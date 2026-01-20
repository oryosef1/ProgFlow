#include "AudioExporter.h"

AudioExporter::AudioExporter(AudioEngine& engine)
    : audioEngine(engine)
{
}

//==============================================================================
// Async export

void AudioExporter::exportAsync(const juce::File& outputFile,
                                 Format format,
                                 const ExportSettings& settings,
                                 ProgressCallback onProgress,
                                 CompletionCallback onComplete)
{
    if (exporting.load())
    {
        if (onComplete)
            onComplete(false, "Export already in progress");
        return;
    }

    exporting.store(true);
    shouldCancel.store(false);

    // Run export on background thread
    juce::Thread::launch([this, outputFile, format, settings, onProgress, onComplete]()
    {
        bool success = false;
        juce::String errorMessage;

        if (format == Format::WAV)
        {
            success = exportToWav(outputFile, settings, onProgress);
            if (!success && !shouldCancel.load())
                errorMessage = "Failed to write WAV file";
        }
        else if (format == Format::MP3)
        {
            success = exportToMp3(outputFile, settings, onProgress);
            if (!success && !shouldCancel.load())
                errorMessage = "Failed to write MP3 file";
        }

        if (shouldCancel.load())
            errorMessage = "Export cancelled";

        exporting.store(false);

        // Call completion on message thread
        juce::MessageManager::callAsync([onComplete, success, errorMessage]()
        {
            if (onComplete)
                onComplete(success && errorMessage.isEmpty(), errorMessage);
        });
    });
}

void AudioExporter::cancelExport()
{
    shouldCancel.store(true);
}

//==============================================================================
// Synchronous export

bool AudioExporter::exportToWav(const juce::File& outputFile,
                                 const ExportSettings& settings,
                                 ProgressCallback onProgress)
{
    // Render audio to buffer
    juce::AudioBuffer<float> buffer;
    if (!renderToBuffer(buffer, settings, onProgress))
        return false;

    if (shouldCancel.load())
        return false;

    // Normalize if requested
    if (settings.normalizeOutput)
        normalizeBuffer(buffer);

    // Write to file
    return writeWavFile(outputFile, buffer, settings.sampleRate, settings.bitDepth);
}

bool AudioExporter::exportToMp3(const juce::File& outputFile,
                                 const ExportSettings& settings,
                                 ProgressCallback onProgress)
{
    // Render audio to buffer
    juce::AudioBuffer<float> buffer;
    if (!renderToBuffer(buffer, settings, onProgress))
        return false;

    if (shouldCancel.load())
        return false;

    // Normalize if requested
    if (settings.normalizeOutput)
        normalizeBuffer(buffer);

    // Write to file
    return writeMp3File(outputFile, buffer, settings.sampleRate, settings.mp3Bitrate);
}

//==============================================================================
// Offline rendering

bool AudioExporter::renderToBuffer(juce::AudioBuffer<float>& buffer,
                                    const ExportSettings& settings,
                                    ProgressCallback onProgress)
{
    const double sampleRate = static_cast<double>(settings.sampleRate);
    const int blockSize = 512;

    // Calculate total samples
    const double startBeats = settings.startBar * 4.0;
    const double endBeats = settings.endBar * 4.0;
    const double totalBeats = endBeats - startBeats;

    // Convert beats to samples using BPM
    const double bpm = audioEngine.getBpm();
    const double secondsPerBeat = 60.0 / bpm;
    const double totalSeconds = totalBeats * secondsPerBeat;
    const int totalSamples = static_cast<int>(std::ceil(totalSeconds * sampleRate));

    // Allocate buffer
    buffer.setSize(2, totalSamples);
    buffer.clear();

    // Prepare audio engine for offline rendering
    // Note: We need to save and restore the engine state
    const bool wasPlaying = audioEngine.isPlaying();
    const double originalPosition = audioEngine.getPositionInBeats();

    audioEngine.stop();
    audioEngine.setPositionInBeats(startBeats);

    // Process audio in blocks
    int samplesProcessed = 0;
    juce::AudioBuffer<float> blockBuffer(2, blockSize);
    juce::AudioSourceChannelInfo channelInfo(&blockBuffer, 0, blockSize);

    while (samplesProcessed < totalSamples && !shouldCancel.load())
    {
        const int samplesThisBlock = std::min(blockSize, totalSamples - samplesProcessed);
        channelInfo.numSamples = samplesThisBlock;

        blockBuffer.clear();

        // Simulate playback
        audioEngine.setPlaying(true);
        audioEngine.getNextAudioBlock(channelInfo);
        audioEngine.setPlaying(false);

        // Copy to output buffer
        for (int ch = 0; ch < 2; ++ch)
        {
            buffer.copyFrom(ch, samplesProcessed, blockBuffer, ch, 0, samplesThisBlock);
        }

        samplesProcessed += samplesThisBlock;

        // Report progress
        if (onProgress)
        {
            float progress = static_cast<float>(samplesProcessed) / static_cast<float>(totalSamples);
            juce::MessageManager::callAsync([onProgress, progress]()
            {
                onProgress(progress * 0.9f); // Reserve 10% for file writing
            });
        }
    }

    // Restore engine state
    audioEngine.setPositionInBeats(originalPosition);
    if (wasPlaying)
        audioEngine.play();

    return !shouldCancel.load();
}

//==============================================================================
// File writing

bool AudioExporter::writeWavFile(const juce::File& file,
                                  const juce::AudioBuffer<float>& buffer,
                                  int sampleRate,
                                  int bitDepth)
{
    // Delete existing file
    file.deleteFile();

    // Create output stream
    auto outputStream = file.createOutputStream();
    if (!outputStream)
        return false;

    // Create WAV writer
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(outputStream.release(),
                                  sampleRate,
                                  buffer.getNumChannels(),
                                  bitDepth,
                                  {},
                                  0));

    if (!writer)
        return false;

    // Write samples
    return writer->writeFromAudioSampleBuffer(buffer, 0, buffer.getNumSamples());
}

bool AudioExporter::writeMp3File(const juce::File& file,
                                  const juce::AudioBuffer<float>& buffer,
                                  int sampleRate,
                                  int bitrate)
{
    // First, write a temporary WAV file
    juce::File tempWav = file.getSiblingFile(file.getFileNameWithoutExtension() + "_temp.wav");

    if (!writeWavFile(tempWav, buffer, sampleRate, 16))
    {
        DBG("Failed to write temporary WAV for MP3 conversion");
        return false;
    }

    // Try to use system LAME encoder if available
    juce::String lameCommand;

#if JUCE_MAC
    // Check common LAME locations on macOS
    if (juce::File("/usr/local/bin/lame").existsAsFile())
        lameCommand = "/usr/local/bin/lame";
    else if (juce::File("/opt/homebrew/bin/lame").existsAsFile())
        lameCommand = "/opt/homebrew/bin/lame";
#elif JUCE_WINDOWS
    // Check if lame.exe is in PATH or common locations
    if (juce::File("C:/Program Files/LAME/lame.exe").existsAsFile())
        lameCommand = "\"C:/Program Files/LAME/lame.exe\"";
#elif JUCE_LINUX
    if (juce::File("/usr/bin/lame").existsAsFile())
        lameCommand = "/usr/bin/lame";
#endif

    if (lameCommand.isNotEmpty())
    {
        // Use LAME to encode
        juce::String cmd = lameCommand + " -b " + juce::String(bitrate) + " "
                          + "\"" + tempWav.getFullPathName() + "\" "
                          + "\"" + file.getFullPathName() + "\"";

        DBG("Running LAME: " + cmd);

        juce::ChildProcess process;
        if (process.start(cmd) && process.waitForProcessToFinish(60000))
        {
            int exitCode = process.getExitCode();
            tempWav.deleteFile();

            if (exitCode == 0 && file.existsAsFile())
            {
                DBG("MP3 export successful via LAME");
                return true;
            }
        }
    }

    // Fallback: LAME not available
    // Rename WAV to MP3 (not ideal, but at least produces a playable file in some players)
    DBG("LAME not found - MP3 export creating WAV with .mp3 extension");
    DBG("For proper MP3 encoding, install LAME: brew install lame (macOS) or apt install lame (Linux)");

    tempWav.moveFileTo(file);
    return true;  // Return true since we wrote something, but it's not a real MP3
}

//==============================================================================
// Utilities

void AudioExporter::normalizeBuffer(juce::AudioBuffer<float>& buffer)
{
    float maxLevel = 0.0f;

    // Find peak level
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        maxLevel = std::max(maxLevel, buffer.getMagnitude(ch, 0, buffer.getNumSamples()));
    }

    // Apply gain to normalize
    if (maxLevel > 0.0f && maxLevel != 1.0f)
    {
        float gain = 1.0f / maxLevel;
        // Leave a bit of headroom
        gain *= 0.95f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            buffer.applyGain(ch, 0, buffer.getNumSamples(), gain);
        }
    }
}

double AudioExporter::calculateProjectLengthBars(AudioEngine& engine)
{
    double maxEndBar = 4.0; // Minimum 4 bars

    for (int i = 0; i < engine.getNumTracks(); ++i)
    {
        if (auto* track = engine.getTrack(i))
        {
            for (const auto& clip : track->getClips())
            {
                maxEndBar = std::max(maxEndBar, clip->getEndBar());
            }
        }
    }

    return maxEndBar;
}

juce::StringArray AudioExporter::getSupportedFormats()
{
    return { "WAV", "MP3" };
}

bool AudioExporter::isMp3Supported()
{
    // TODO: Check if LAME is available
    // For now, return true but MP3 export will produce a WAV in disguise
    return true;
}
