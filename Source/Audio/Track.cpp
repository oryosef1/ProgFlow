#include "Track.h"
#include <algorithm>

Track::Track(const juce::String& trackName)
    : name(trackName)
{
    // Create default synth (AnalogSynth)
    synth = SynthFactory::createSynth(SynthType::Analog);
}

//==============================================================================
// Clip Management

MidiClip* Track::addClip(double startBar, double durationBars)
{
    juce::ScopedLock lock(clipLock);

    auto clip = std::make_unique<MidiClip>("Clip " + juce::String(clips.size() + 1));
    clip->setStartBar(startBar);
    clip->setDurationBars(durationBars);
    clip->setColour(colour);  // Inherit track color

    MidiClip* ptr = clip.get();
    clips.push_back(std::move(clip));
    sortClips();

    return ptr;
}

MidiClip* Track::addClip(std::unique_ptr<MidiClip> clip)
{
    if (!clip) return nullptr;

    juce::ScopedLock lock(clipLock);

    MidiClip* ptr = clip.get();
    clips.push_back(std::move(clip));
    sortClips();

    return ptr;
}

void Track::removeClip(const juce::Uuid& clipId)
{
    juce::ScopedLock lock(clipLock);

    clips.erase(
        std::remove_if(clips.begin(), clips.end(),
            [&clipId](const std::unique_ptr<MidiClip>& c) {
                return c->getId() == clipId;
            }),
        clips.end()
    );
}

MidiClip* Track::getClip(const juce::Uuid& clipId)
{
    juce::ScopedLock lock(clipLock);

    for (auto& clip : clips)
    {
        if (clip->getId() == clipId)
            return clip.get();
    }
    return nullptr;
}

const MidiClip* Track::getClip(const juce::Uuid& clipId) const
{
    for (const auto& clip : clips)
    {
        if (clip->getId() == clipId)
            return clip.get();
    }
    return nullptr;
}

MidiClip* Track::getClipAt(double barPosition)
{
    juce::ScopedLock lock(clipLock);

    for (auto& clip : clips)
    {
        if (barPosition >= clip->getStartBar() && barPosition < clip->getEndBar())
            return clip.get();
    }
    return nullptr;
}

void Track::getClipsInRange(double startBar, double endBar,
                             std::vector<MidiClip*>& result)
{
    juce::ScopedLock lock(clipLock);
    result.clear();

    for (auto& clip : clips)
    {
        // Check if clip overlaps with the range
        if (clip->getEndBar() > startBar && clip->getStartBar() < endBar)
        {
            result.push_back(clip.get());
        }
    }
}

void Track::sortClips()
{
    std::stable_sort(clips.begin(), clips.end(),
        [](const std::unique_ptr<MidiClip>& a, const std::unique_ptr<MidiClip>& b) {
            return a->getStartBar() < b->getStartBar();
        });
}

//==============================================================================
// Recording (with overflow support)

MidiClip* Track::getOrCreateClipForRecording(double barPosition,
                                              double minDuration,
                                              bool extendExisting)
{
    juce::ScopedLock lock(clipLock);

    // First, check if there's a clip at this position
    MidiClip* existingClip = nullptr;
    for (auto& clip : clips)
    {
        if (barPosition >= clip->getStartBar() && barPosition < clip->getEndBar())
        {
            existingClip = clip.get();
            break;
        }
    }

    if (existingClip)
        return existingClip;

    // No clip at this position - check if we should extend a previous clip
    if (extendExisting)
    {
        // Find the most recent clip that ends before or at this position
        MidiClip* previousClip = nullptr;
        for (auto& clip : clips)
        {
            if (clip->getEndBar() <= barPosition + 0.001)  // Small tolerance
            {
                if (!previousClip || clip->getEndBar() > previousClip->getEndBar())
                {
                    previousClip = clip.get();
                }
            }
        }

        // If there's a recent clip within one bar, extend it (recording overflow)
        if (previousClip && (barPosition - previousClip->getEndBar()) < 1.0)
        {
            double newDuration = barPosition - previousClip->getStartBar() + minDuration;
            previousClip->setDurationBars(newDuration);
            return previousClip;
        }
    }

    // Create a new clip at this position
    // Align to bar boundary for cleaner arrangement
    double clipStart = std::floor(barPosition);
    auto newClip = std::make_unique<MidiClip>("Clip " + juce::String(clips.size() + 1));
    newClip->setStartBar(clipStart);
    newClip->setDurationBars(minDuration);
    newClip->setColour(colour);

    MidiClip* ptr = newClip.get();
    clips.push_back(std::move(newClip));
    sortClips();

    return ptr;
}

MidiClip* Track::recordNoteAtPosition(double barPosition, int midiNote,
                                       double durationBars, float velocity,
                                       bool extendClip)
{
    // Get or create a clip at this position
    MidiClip* clip = getOrCreateClipForRecording(barPosition, 4.0, extendClip);
    if (!clip)
        return nullptr;

    // Calculate note position relative to clip start
    double noteStartInClip = (barPosition - clip->getStartBar()) * 4.0;  // Convert to beats

    // Check if note extends past clip end
    double noteEndBar = barPosition + durationBars;
    if (noteEndBar > clip->getEndBar())
    {
        if (extendClip)
        {
            // Extend the clip to accommodate the note
            double newDuration = noteEndBar - clip->getStartBar();
            clip->setDurationBars(newDuration);
        }
        else
        {
            // Truncate the note to fit within clip
            durationBars = clip->getEndBar() - barPosition;
        }
    }

    // Add the note to the clip
    clip->addNote(midiNote, noteStartInClip, durationBars * 4.0, velocity);

    return clip;
}

//==============================================================================
// Audio Clip Management

AudioClip* Track::addAudioClip(double startBeat)
{
    juce::ScopedLock lock(audioClipLock);

    auto clip = std::make_unique<AudioClip>();
    clip->setStartBeat(startBeat);
    clip->setName("Audio " + juce::String(audioClips.size() + 1));

    AudioClip* ptr = clip.get();
    audioClips.push_back(std::move(clip));
    sortAudioClips();

    return ptr;
}

AudioClip* Track::addAudioClip(std::unique_ptr<AudioClip> clip)
{
    if (!clip) return nullptr;

    juce::ScopedLock lock(audioClipLock);

    AudioClip* ptr = clip.get();
    audioClips.push_back(std::move(clip));
    sortAudioClips();

    return ptr;
}

void Track::removeAudioClip(const juce::String& clipId)
{
    juce::ScopedLock lock(audioClipLock);

    audioClips.erase(
        std::remove_if(audioClips.begin(), audioClips.end(),
            [&clipId](const std::unique_ptr<AudioClip>& c) {
                return c->getId() == clipId;
            }),
        audioClips.end()
    );
}

AudioClip* Track::getAudioClip(const juce::String& clipId)
{
    juce::ScopedLock lock(audioClipLock);

    for (auto& clip : audioClips)
    {
        if (clip->getId() == clipId)
            return clip.get();
    }
    return nullptr;
}

const AudioClip* Track::getAudioClip(const juce::String& clipId) const
{
    for (const auto& clip : audioClips)
    {
        if (clip->getId() == clipId)
            return clip.get();
    }
    return nullptr;
}

AudioClip* Track::getAudioClipAt(double beatPosition)
{
    juce::ScopedLock lock(audioClipLock);

    // Current BPM needed for duration calculation
    // For now, assume 120 BPM if we don't have it
    const double bpm = 120.0;

    for (auto& clip : audioClips)
    {
        double startBeat = clip->getStartBeat();
        double endBeat = startBeat + clip->getDurationInBeats(bpm);

        if (beatPosition >= startBeat && beatPosition < endBeat)
            return clip.get();
    }
    return nullptr;
}

void Track::getAudioClipsInRange(double startBeat, double endBeat, double bpm,
                                  std::vector<AudioClip*>& result)
{
    juce::ScopedLock lock(audioClipLock);
    result.clear();

    for (auto& clip : audioClips)
    {
        double clipStart = clip->getStartBeat();
        double clipEnd = clipStart + clip->getDurationInBeats(bpm);

        // Check if clip overlaps with the range
        if (clipEnd > startBeat && clipStart < endBeat)
        {
            result.push_back(clip.get());
        }
    }
}

void Track::sortAudioClips()
{
    std::stable_sort(audioClips.begin(), audioClips.end(),
        [](const std::unique_ptr<AudioClip>& a, const std::unique_ptr<AudioClip>& b) {
            return a->getStartBeat() < b->getStartBeat();
        });
}

void Track::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    // Prepare synth
    if (synth)
        synth->prepareToPlay(sampleRate, samplesPerBlock);

    // Prepare plugin instrument
    if (pluginInstrument)
        pluginInstrument->prepareToPlay(sampleRate, samplesPerBlock);

    // Prepare plugin effects
    for (auto& effect : pluginEffects)
    {
        if (effect)
            effect->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void Track::processBlock(juce::AudioBuffer<float>& buffer, int numSamples,
                         double positionInBeats, double bpm)
{
    // Skip if muted
    if (muted.load())
    {
        meterLevel.store(0.0f);
        return;
    }

    // Apply automation if in a reading mode
    if (automationMode != AutomationMode::Off)
    {
        applyAutomation(positionInBeats);
    }

    // Process audio clips first (they contribute audio directly)
    if (!audioClips.empty())
    {
        processAudioClips(buffer, numSamples, positionInBeats, bpm);
    }

    juce::ScopedLock lock(synthLock);

    // Process instrument (either plugin or built-in synth)
    if (usePluginInstrument && pluginInstrument)
    {
        // Use plugin instrument
        pluginInstrument->processBlock(buffer, synthMidiBuffer);
    }
    else if (synth)
    {
        // Update BPM for tempo-synced features (LFOs, etc.)
        synth->setBpm(bpm);
        // Use built-in synth
        synth->processBlock(buffer, synthMidiBuffer);
    }

    synthMidiBuffer.clear();

    // Process plugin effects chain
    juce::MidiBuffer emptyMidi;  // Effects don't need MIDI
    for (auto& effect : pluginEffects)
    {
        if (effect)
        {
            effect->processBlock(buffer, emptyMidi);
        }
    }

    // Apply gain and pan
    applyGainAndPan(buffer);
    updateMeter(buffer);
}

void Track::releaseResources()
{
    if (synth)
        synth->releaseResources();

    if (pluginInstrument)
        pluginInstrument->releaseResources();

    for (auto& effect : pluginEffects)
    {
        if (effect)
            effect->releaseResources();
    }
}

//==============================================================================
// Synth Management

void Track::setSynthType(SynthType type)
{
    if (type == synthType && synth != nullptr)
        return;

    juce::ScopedLock lock(synthLock);

    // Create new synth
    auto newSynth = SynthFactory::createSynth(type);

    // Prepare it if we have valid audio settings
    if (sampleRate > 0 && samplesPerBlock > 0)
        newSynth->prepareToPlay(sampleRate, samplesPerBlock);

    // Swap
    synth = std::move(newSynth);
    synthType = type;
}

void Track::synthNoteOn(int midiNote, float velocity)
{
    juce::ScopedLock lock(synthLock);
    if (synth)
    {
        // Add to MIDI buffer for next processBlock
        int velocityInt = static_cast<int>(velocity * 127.0f);
        synthMidiBuffer.addEvent(juce::MidiMessage::noteOn(1, midiNote, static_cast<juce::uint8>(velocityInt)), 0);
    }
}

void Track::synthNoteOff(int midiNote)
{
    juce::ScopedLock lock(synthLock);
    if (synth)
    {
        synthMidiBuffer.addEvent(juce::MidiMessage::noteOff(1, midiNote), 0);
    }
}

void Track::synthAllNotesOff()
{
    juce::ScopedLock lock(synthLock);
    if (synth)
    {
        synth->allNotesOff();
        synthMidiBuffer.clear();
    }
}

void Track::applyGainAndPan(juce::AudioBuffer<float>& buffer)
{
    const float vol = volume.load();
    const float p = pan.load();

    // Calculate left/right gains based on pan (equal power panning)
    const float leftGain = vol * std::cos((p + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f);
    const float rightGain = vol * std::sin((p + 1.0f) * juce::MathConstants<float>::halfPi * 0.5f);

    if (buffer.getNumChannels() >= 2)
    {
        buffer.applyGain(0, 0, buffer.getNumSamples(), leftGain);
        buffer.applyGain(1, 0, buffer.getNumSamples(), rightGain);
    }
    else if (buffer.getNumChannels() == 1)
    {
        buffer.applyGain(vol);
    }
}

void Track::updateMeter(const juce::AudioBuffer<float>& buffer)
{
    // Calculate RMS level
    float rms = 0.0f;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        rms += buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
    }

    rms /= static_cast<float>(buffer.getNumChannels());

    // Smooth the meter value
    const float smoothing = 0.8f;
    meterLevel.store(meterLevel.load() * smoothing + rms * (1.0f - smoothing));
}

//==============================================================================
// Plugin Instrument Management

void Track::setPluginInstrument(std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    juce::ScopedLock lock(synthLock);

    pluginInstrument = std::move(plugin);

    if (pluginInstrument)
    {
        // Store the plugin description
        pluginInstrumentDesc = std::make_unique<juce::PluginDescription>();
        pluginInstrument->fillInPluginDescription(*pluginInstrumentDesc);

        // Prepare plugin if we have valid audio settings
        if (sampleRate > 0 && samplesPerBlock > 0)
        {
            pluginInstrument->prepareToPlay(sampleRate, samplesPerBlock);
        }

        usePluginInstrument = true;
    }
    else
    {
        pluginInstrumentDesc.reset();
        usePluginInstrument = false;
    }
}

void Track::clearPluginInstrument()
{
    juce::ScopedLock lock(synthLock);

    if (pluginInstrument)
    {
        pluginInstrument->releaseResources();
        pluginInstrument.reset();
    }
    pluginInstrumentDesc.reset();
    usePluginInstrument = false;
}

const juce::PluginDescription* Track::getPluginInstrumentDescription() const
{
    return pluginInstrumentDesc.get();
}

//==============================================================================
// Plugin Effect Management

juce::AudioPluginInstance* Track::getPluginEffect(int slot)
{
    if (slot < 0 || slot >= MAX_PLUGIN_EFFECTS)
        return nullptr;
    return pluginEffects[static_cast<size_t>(slot)].get();
}

const juce::AudioPluginInstance* Track::getPluginEffect(int slot) const
{
    if (slot < 0 || slot >= MAX_PLUGIN_EFFECTS)
        return nullptr;
    return pluginEffects[static_cast<size_t>(slot)].get();
}

void Track::setPluginEffect(int slot, std::unique_ptr<juce::AudioPluginInstance> plugin)
{
    if (slot < 0 || slot >= MAX_PLUGIN_EFFECTS)
        return;

    juce::ScopedLock lock(synthLock);

    // Release old plugin
    if (pluginEffects[static_cast<size_t>(slot)])
    {
        pluginEffects[static_cast<size_t>(slot)]->releaseResources();
    }

    pluginEffects[static_cast<size_t>(slot)] = std::move(plugin);

    if (pluginEffects[static_cast<size_t>(slot)])
    {
        // Store the plugin description
        pluginEffectDescs[static_cast<size_t>(slot)] = std::make_unique<juce::PluginDescription>();
        pluginEffects[static_cast<size_t>(slot)]->fillInPluginDescription(*pluginEffectDescs[static_cast<size_t>(slot)]);

        // Prepare plugin if we have valid audio settings
        if (sampleRate > 0 && samplesPerBlock > 0)
        {
            pluginEffects[static_cast<size_t>(slot)]->prepareToPlay(sampleRate, samplesPerBlock);
        }
    }
    else
    {
        pluginEffectDescs[static_cast<size_t>(slot)].reset();
    }
}

void Track::clearPluginEffect(int slot)
{
    if (slot < 0 || slot >= MAX_PLUGIN_EFFECTS)
        return;

    juce::ScopedLock lock(synthLock);

    if (pluginEffects[static_cast<size_t>(slot)])
    {
        pluginEffects[static_cast<size_t>(slot)]->releaseResources();
        pluginEffects[static_cast<size_t>(slot)].reset();
    }
    pluginEffectDescs[static_cast<size_t>(slot)].reset();
}

int Track::getNumPluginEffects() const
{
    int count = 0;
    for (const auto& effect : pluginEffects)
    {
        if (effect != nullptr)
            count++;
    }
    return count;
}

const juce::PluginDescription* Track::getPluginEffectDescription(int slot) const
{
    if (slot < 0 || slot >= MAX_PLUGIN_EFFECTS)
        return nullptr;
    return pluginEffectDescs[static_cast<size_t>(slot)].get();
}

//==============================================================================
// Automation

AutomationLane* Track::getAutomationLane(const juce::String& parameterId)
{
    for (auto& lane : automationLanes)
    {
        if (lane->getParameterId() == parameterId)
            return lane.get();
    }
    return nullptr;
}

const AutomationLane* Track::getAutomationLane(const juce::String& parameterId) const
{
    for (const auto& lane : automationLanes)
    {
        if (lane->getParameterId() == parameterId)
            return lane.get();
    }
    return nullptr;
}

AutomationLane* Track::getOrCreateAutomationLane(const juce::String& parameterId)
{
    // Check if lane already exists
    if (auto* existing = getAutomationLane(parameterId))
        return existing;

    // Create new lane
    auto lane = std::make_unique<AutomationLane>(parameterId);
    AutomationLane* ptr = lane.get();
    automationLanes.push_back(std::move(lane));
    return ptr;
}

void Track::removeAutomationLane(const juce::String& parameterId)
{
    automationLanes.erase(
        std::remove_if(automationLanes.begin(), automationLanes.end(),
            [&parameterId](const std::unique_ptr<AutomationLane>& lane) {
                return lane->getParameterId() == parameterId;
            }),
        automationLanes.end()
    );
}

std::vector<juce::String> Track::getAutomatableParameters() const
{
    std::vector<juce::String> params;

    // Track-level parameters
    params.push_back("volume");
    params.push_back("pan");

    // Synth parameters (prefixed with "synth.")
    if (synth)
    {
        for (const auto& paramName : synth->getParameterNames())
        {
            params.push_back("synth." + paramName);
        }
    }

    return params;
}

void Track::applyAutomation(double positionInBeats)
{
    for (const auto& lane : automationLanes)
    {
        if (lane->getNumPoints() == 0)
            continue;

        float normalizedValue = lane->getValueAtTime(positionInBeats);
        const auto& paramId = lane->getParameterId();

        if (paramId == "volume")
        {
            // Normalized 0-1 maps to actual 0-2
            setVolume(normalizedValue * 2.0f);
        }
        else if (paramId == "pan")
        {
            // Normalized 0-1 maps to actual -1 to 1
            setPan(normalizedValue * 2.0f - 1.0f);
        }
        else if (paramId.startsWith("synth.") && synth != nullptr)
        {
            // Forward to synth parameter system
            auto synthParamId = paramId.substring(6);  // Remove "synth." prefix
            auto* paramInfo = synth->getParameterInfo(synthParamId);
            if (paramInfo != nullptr)
            {
                // Convert normalized to actual range
                float actualValue = paramInfo->minValue +
                    normalizedValue * (paramInfo->maxValue - paramInfo->minValue);
                synth->setParameter(synthParamId, actualValue);
            }
        }
    }
}

//==============================================================================
// Audio Clip Processing

void Track::processAudioClips(juce::AudioBuffer<float>& buffer, int numSamples,
                               double positionInBeats, double bpm)
{
    if (bpm <= 0 || sampleRate <= 0)
        return;

    juce::ScopedLock lock(audioClipLock);

    // Calculate time values
    double beatsPerSecond = bpm / 60.0;
    double secondsPerBeat = 60.0 / bpm;
    double blockDurationInBeats = (numSamples / sampleRate) * beatsPerSecond;
    double blockEndBeat = positionInBeats + blockDurationInBeats;

    // Find clips that overlap with this block
    for (auto& clip : audioClips)
    {
        if (!clip->hasAudio())
            continue;

        double clipStartBeat = clip->getStartBeat();
        double clipDurationBeats = clip->getDurationInBeats(bpm);
        double clipEndBeat = clipStartBeat + clipDurationBeats;

        // Check if clip overlaps with the current block
        if (clipEndBeat <= positionInBeats || clipStartBeat >= blockEndBeat)
            continue;

        // Calculate clip's sample rate ratio for resampling
        double clipSampleRate = clip->getSampleRate();
        double playbackRate = clip->getPlaybackRate();
        double sampleRateRatio = (clipSampleRate / sampleRate) * playbackRate;

        // Calculate the position within the clip at the start of this block
        double beatOffsetInClip = positionInBeats - clipStartBeat;
        double secondsIntoClip = beatOffsetInClip * secondsPerBeat;

        // If we're before the clip start, adjust
        int sampleOffset = 0;
        if (beatOffsetInClip < 0)
        {
            // Block starts before clip - calculate how many samples to skip
            sampleOffset = static_cast<int>((-beatOffsetInClip * secondsPerBeat) * sampleRate);
            secondsIntoClip = 0;
        }

        // Get trim points
        juce::int64 trimStart = clip->getTrimStartSample();
        juce::int64 trimEnd = clip->getTrimEndSample();
        juce::int64 trimmedDuration = trimEnd - trimStart;

        // Calculate starting sample position in clip (accounting for playback rate)
        double clipSamplePos = trimStart + (secondsIntoClip * clipSampleRate / playbackRate);

        // Get clip gain and fade info
        float clipGain = clip->getGain();
        juce::int64 fadeInSamples = clip->getFadeInSamples();
        juce::int64 fadeOutSamples = clip->getFadeOutSamples();

        const auto& audioBuffer = clip->getAudioBuffer();
        int clipChannels = audioBuffer.getNumChannels();
        int bufferChannels = buffer.getNumChannels();

        // Process each sample in the block
        for (int i = sampleOffset; i < numSamples; ++i)
        {
            // Calculate current position in clip samples
            double currentClipSample = clipSamplePos + (i - sampleOffset) * sampleRateRatio;

            // Check if we're past the trimmed end
            if (currentClipSample >= trimEnd)
                break;

            // Skip if before trim start (shouldn't happen, but be safe)
            if (currentClipSample < trimStart)
                continue;

            // Calculate sample position relative to trim start (for fades)
            juce::int64 sampleInTrimmedClip = static_cast<juce::int64>(currentClipSample) - trimStart;

            // Calculate fade envelope
            float fadeGain = 1.0f;

            // Fade in
            if (fadeInSamples > 0 && sampleInTrimmedClip < fadeInSamples)
            {
                fadeGain = static_cast<float>(sampleInTrimmedClip) / static_cast<float>(fadeInSamples);
            }

            // Fade out
            juce::int64 samplesFromEnd = trimmedDuration - sampleInTrimmedClip;
            if (fadeOutSamples > 0 && samplesFromEnd < fadeOutSamples)
            {
                fadeGain *= static_cast<float>(samplesFromEnd) / static_cast<float>(fadeOutSamples);
            }

            // Linear interpolation for resampling
            int sampleIndex = static_cast<int>(currentClipSample);
            float frac = static_cast<float>(currentClipSample - sampleIndex);

            // Get interpolated sample for each channel
            for (int ch = 0; ch < bufferChannels; ++ch)
            {
                // Map buffer channel to clip channel (mono clips play on both channels)
                int clipCh = (clipChannels == 1) ? 0 : std::min(ch, clipChannels - 1);

                float sample1 = clip->getSample(clipCh, sampleIndex);
                float sample2 = clip->getSample(clipCh, sampleIndex + 1);
                float interpolatedSample = sample1 + frac * (sample2 - sample1);

                // Apply gain and fade, add to buffer
                buffer.addSample(ch, i, interpolatedSample * clipGain * fadeGain);
            }
        }
    }
}
