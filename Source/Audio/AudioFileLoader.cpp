#include "AudioFileLoader.h"
#include <juce_dsp/juce_dsp.h>

AudioFileLoader::AudioFileLoader()
{
    // Register all basic audio formats
    formatManager.registerBasicFormats();
}

std::unique_ptr<AudioClip> AudioFileLoader::loadFile(const juce::File& file, double targetSampleRate)
{
    auto clip = std::make_unique<AudioClip>();

    if (!loadIntoClip(file, *clip, targetSampleRate))
        return nullptr;

    return clip;
}

bool AudioFileLoader::loadIntoClip(const juce::File& file, AudioClip& clip, double targetSampleRate)
{
    if (!file.existsAsFile())
        return false;

    // Create a reader for the file
    std::unique_ptr<juce::AudioFormatReader> reader(
        formatManager.createReaderFor(file));

    if (reader == nullptr)
        return false;

    // Read the audio data
    juce::AudioBuffer<float> buffer(static_cast<int>(reader->numChannels),
                                     static_cast<int>(reader->lengthInSamples));

    reader->read(&buffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    double sourceSampleRate = reader->sampleRate;

    // Resample if needed
    if (targetSampleRate > 0 && std::abs(targetSampleRate - sourceSampleRate) > 0.1)
    {
        resample(buffer, sourceSampleRate, targetSampleRate);
        sourceSampleRate = targetSampleRate;
    }

    // Set the clip data
    clip.setAudioBuffer(std::move(buffer), sourceSampleRate);
    clip.setFilePath(file.getFullPathName());
    clip.setName(file.getFileNameWithoutExtension());

    return true;
}

juce::String AudioFileLoader::getSupportedFormatsWildcard() const
{
    // Build wildcard from registered formats
    juce::StringArray wildcards;

    for (int i = 0; i < formatManager.getNumKnownFormats(); ++i)
    {
        auto* format = formatManager.getKnownFormat(i);
        wildcards.addArray(format->getFileExtensions());
    }

    juce::String result;
    for (const auto& ext : wildcards)
    {
        if (result.isNotEmpty())
            result += ";";
        result += "*" + ext;
    }

    return result;
}

bool AudioFileLoader::isFormatSupported(const juce::String& extension) const
{
    juce::String ext = extension.toLowerCase();
    if (!ext.startsWith("."))
        ext = "." + ext;

    for (int i = 0; i < formatManager.getNumKnownFormats(); ++i)
    {
        auto* format = formatManager.getKnownFormat(i);
        if (format->getFileExtensions().contains(ext))
            return true;
    }

    return false;
}

void AudioFileLoader::resample(juce::AudioBuffer<float>& buffer, double sourceSampleRate,
                                double targetSampleRate)
{
    if (sourceSampleRate <= 0 || targetSampleRate <= 0)
        return;

    double ratio = targetSampleRate / sourceSampleRate;
    int newLength = static_cast<int>(buffer.getNumSamples() * ratio);

    juce::AudioBuffer<float> resampled(buffer.getNumChannels(), newLength);

    // Use Lagrange interpolation for resampling
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const float* srcData = buffer.getReadPointer(channel);
        float* dstData = resampled.getWritePointer(channel);

        for (int i = 0; i < newLength; ++i)
        {
            double srcPosition = i / ratio;
            int srcIndex = static_cast<int>(srcPosition);
            float frac = static_cast<float>(srcPosition - srcIndex);

            // Linear interpolation
            float sample1 = (srcIndex < buffer.getNumSamples()) ? srcData[srcIndex] : 0.0f;
            float sample2 = (srcIndex + 1 < buffer.getNumSamples()) ? srcData[srcIndex + 1] : sample1;

            dstData[i] = sample1 + frac * (sample2 - sample1);
        }
    }

    // Replace the buffer with resampled version
    buffer = std::move(resampled);
}

//==============================================================================
// Global singleton

static std::unique_ptr<AudioFileLoader> globalAudioFileLoader;
static juce::SpinLock globalLoaderLock;

AudioFileLoader& getAudioFileLoader()
{
    juce::SpinLock::ScopedLockType lock(globalLoaderLock);

    if (globalAudioFileLoader == nullptr)
        globalAudioFileLoader = std::make_unique<AudioFileLoader>();

    return *globalAudioFileLoader;
}
