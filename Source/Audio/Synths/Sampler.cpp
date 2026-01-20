#include "Sampler.h"
#include <cmath>

//==============================================================================
// SamplerVoice Implementation
//==============================================================================

SamplerVoice::SamplerVoice()
{
    filterEnvelope.setParameters(filterEnvParams);
}

void SamplerVoice::prepareToPlay(double sr, int blockSize)
{
    SynthVoice::prepareToPlay(sr, blockSize);

    // Prepare filter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(filterCutoff);
    filter.setResonance(filterResonance);

    filterEnvelope.setSampleRate(sr);
}

void SamplerVoice::reset()
{
    SynthVoice::reset();

    samplePosition = 0.0;
    playbackRate = 1.0;
    filter.reset();
    filterEnvelope.reset();
}

void SamplerVoice::onNoteStart()
{
    // Reset sample position to start position
    if (currentZone && currentZone->sampleData.getNumSamples() > 0)
    {
        samplePosition = startPosition * currentZone->sampleData.getNumSamples();
    }
    else
    {
        samplePosition = 0.0;
    }

    // Calculate playback rate for pitch shifting
    playbackRate = calculatePlaybackRate(currentNote);

    filterEnvelope.noteOn();
}

void SamplerVoice::onNoteStop()
{
    filterEnvelope.noteOff();
}

double SamplerVoice::calculatePlaybackRate(int targetNote)
{
    if (!currentZone)
        return 1.0;

    // Calculate semitone difference
    int semitones = (targetNote - currentZone->rootNote) + transpose;
    float cents = fineTune;

    // Convert to playback rate: 2^(semitones/12)
    double rate = std::pow(2.0, (semitones + cents / 100.0) / 12.0);

    // Adjust for sample rate difference if needed
    if (currentZone->sampleRate > 0.0 && currentZone->sampleRate != sampleRate)
    {
        rate *= currentZone->sampleRate / sampleRate;
    }

    return rate;
}

float SamplerVoice::getInterpolatedSample(double position)
{
    if (!currentZone || currentZone->sampleData.getNumSamples() == 0)
        return 0.0f;

    const auto& buffer = currentZone->sampleData;
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // Clamp position
    if (position < 0.0)
        position = 0.0;
    if (position >= numSamples)
        return 0.0f;

    // Use linear interpolation for simplicity and quality
    int index0 = static_cast<int>(position);
    int index1 = index0 + 1;
    float fraction = static_cast<float>(position - index0);

    // Clamp indices
    index0 = juce::jlimit(0, numSamples - 1, index0);
    index1 = juce::jlimit(0, numSamples - 1, index1);

    // Mix all channels (mono sum)
    float output = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        float sample0 = channelData[index0];
        float sample1 = channelData[index1];
        output += sample0 + fraction * (sample1 - sample0);
    }

    // Average channels
    if (numChannels > 0)
        output /= static_cast<float>(numChannels);

    return output;
}

void SamplerVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                   int startSample, int numSamples)
{
    if (!isActive() || !currentZone || currentZone->sampleData.getNumSamples() == 0)
        return;

    auto* outputL = buffer.getWritePointer(0, startSample);
    auto* outputR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;

    int sampleLength = currentZone->sampleData.getNumSamples();
    int loopStart = currentZone->loopStart;
    int loopEnd = currentZone->loopEnd >= 0 ? currentZone->loopEnd : sampleLength;

    for (int i = 0; i < numSamples; ++i)
    {
        // Get amp envelope value
        float ampEnv = ampEnvelope.getNextSample();

        // Get filter envelope value
        float filterEnv = filterEnvelope.getNextSample();

        // Check if voice should go idle
        if (state == VoiceState::Release && ampEnv < 0.0001f)
        {
            state = VoiceState::Idle;
            currentNote = -1;
            break;
        }

        // Check if playback finished (for non-looping samples)
        if (!looping && samplePosition >= sampleLength)
        {
            state = VoiceState::Idle;
            currentNote = -1;
            break;
        }

        // Read interpolated sample
        float sample = getInterpolatedSample(samplePosition);

        // Apply zone volume
        if (currentZone->volumeDb != 0.0f)
            sample *= std::pow(10.0f, currentZone->volumeDb / 20.0f);

        // Calculate filter cutoff with envelope modulation
        float modulatedCutoff = filterCutoff + (filterEnvAmount * filterEnv);
        modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);

        // Update filter
        filter.setCutoffFrequency(modulatedCutoff);

        // Process through filter
        float filtered = filter.processSample(0, sample);

        // Apply amp envelope and velocity
        float output = filtered * ampEnv * velocity * 0.7f;

        // Write to buffer
        outputL[i] += output;
        if (outputR != nullptr)
            outputR[i] += output;

        // Advance sample position
        samplePosition += playbackRate;

        // Handle looping
        if (looping && samplePosition >= loopEnd)
        {
            samplePosition = loopStart + (samplePosition - loopEnd);
        }

        // Update voice age
        incrementAge(1);
    }
}

//==============================================================================
// Voice setters

void SamplerVoice::setSample(const SampleZone* zone)
{
    currentZone = zone;
}

void SamplerVoice::setLoopMode(bool enabled)
{
    looping = enabled;
}

void SamplerVoice::setStartPosition(float normalizedPosition)
{
    startPosition = juce::jlimit(0.0f, 1.0f, normalizedPosition);
}

void SamplerVoice::setTranspose(int semitones)
{
    transpose = juce::jlimit(-24, 24, semitones);
}

void SamplerVoice::setFineTune(float cents)
{
    fineTune = juce::jlimit(-100.0f, 100.0f, cents);
}

void SamplerVoice::setFilterCutoff(float frequency)
{
    filterCutoff = juce::jlimit(20.0f, 20000.0f, frequency);
}

void SamplerVoice::setFilterResonance(float resonance)
{
    filterResonance = juce::jlimit(0.0f, 1.0f, resonance);
    filter.setResonance(filterResonance);
}

void SamplerVoice::setFilterEnvAmount(float amount)
{
    filterEnvAmount = juce::jlimit(-10000.0f, 10000.0f, amount);
}

void SamplerVoice::setFilterEnvelope(float attack, float decay, float sustain, float release)
{
    filterEnvParams.attack = juce::jmax(0.001f, attack);
    filterEnvParams.decay = juce::jmax(0.001f, decay);
    filterEnvParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    filterEnvParams.release = juce::jmax(0.001f, release);
    filterEnvelope.setParameters(filterEnvParams);
}

//==============================================================================
// Sampler Implementation
//==============================================================================

Sampler::Sampler()
{
    initializeParameters();

    // Register audio formats
    formatManager.registerBasicFormats();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<SamplerVoice>();
    }
}

Sampler::~Sampler()
{
    killAllNotes();
    clearAllSamples();
}

void Sampler::initializeParameters()
{
    // Amp Envelope
    addParameter("amp_attack", "Attack", 0.005f, 0.0f, 2.0f);
    addParameter("amp_decay", "Decay", 0.1f, 0.001f, 2.0f);
    addParameter("amp_sustain", "Sustain", 1.0f, 0.0f, 1.0f);
    addParameter("amp_release", "Release", 0.3f, 0.01f, 5.0f);

    // Filter
    addParameter("filter_cutoff", "Filter Cutoff", 20000.0f, 20.0f, 20000.0f);
    addParameter("filter_resonance", "Filter Resonance", 0.1f, 0.0f, 1.0f);
    addParameter("filter_env_amount", "Filter Env Amount", 0.0f, -10000.0f, 10000.0f);

    // Filter Envelope
    addParameter("filter_attack", "Filter Attack", 0.01f, 0.001f, 2.0f);
    addParameter("filter_decay", "Filter Decay", 0.1f, 0.001f, 2.0f);
    addParameter("filter_sustain", "Filter Sustain", 1.0f, 0.0f, 1.0f);
    addParameter("filter_release", "Filter Release", 0.3f, 0.001f, 5.0f);

    // Playback
    addParameter("transpose", "Transpose", 0.0f, -24.0f, 24.0f, 1.0f);
    addParameter("fine_tune", "Fine Tune", 0.0f, -100.0f, 100.0f, 1.0f);
    addParameter("start", "Start Position", 0.0f, 0.0f, 1.0f);
    addEnumParameter("loop_mode", "Loop Mode", {"Off", "Forward"}, 0);

    // Master
    addParameter("master_volume", "Volume", 0.7f, 0.0f, 1.0f);
}

void Sampler::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    updateVoiceParameters();
}

void Sampler::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }
}

void Sampler::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear buffer
    buffer.clear();

    // Process MIDI
    processMidiMessages(midiMessages);

    // Process all active voices
    int numSamples = buffer.getNumSamples();

    for (auto& voice : voices)
    {
        if (voice->isActive())
        {
            voice->renderNextBlock(buffer, 0, numSamples);
        }
    }

    // Apply master volume
    float masterVol = getParameter("master_volume");
    buffer.applyGain(masterVol);
}

//==============================================================================
// Voice management

SamplerVoice* Sampler::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

SamplerVoice* Sampler::findVoiceToSteal()
{
    // Find oldest voice in release, or oldest overall
    SamplerVoice* oldestRelease = nullptr;
    SamplerVoice* oldest = nullptr;
    float oldestReleaseAge = 0.0f;
    float oldestAge = 0.0f;

    for (auto& voice : voices)
    {
        if (voice->getState() == VoiceState::Release)
        {
            if (voice->getAge() > oldestReleaseAge)
            {
                oldestReleaseAge = voice->getAge();
                oldestRelease = voice.get();
            }
        }
        if (voice->getAge() > oldestAge)
        {
            oldestAge = voice->getAge();
            oldest = voice.get();
        }
    }

    return oldestRelease ? oldestRelease : oldest;
}

SamplerVoice* Sampler::findVoicePlayingNote(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice->isActive() && voice->getCurrentNote() == midiNote)
            return voice.get();
    }
    return nullptr;
}

void Sampler::noteOn(int midiNote, float velocity, int /* sampleOffset */)
{
    // Find appropriate zone for this note
    const SampleZone* zone = findZoneForNote(midiNote);
    if (!zone)
        return; // No sample loaded for this note

    SamplerVoice* voice = findFreeVoice();
    if (!voice)
        voice = findVoiceToSteal();

    if (voice)
    {
        voice->setSample(zone);
        voice->startNote(midiNote, velocity, false);
    }

    activeNotes.insert(midiNote);
}

void Sampler::noteOff(int midiNote, int /* sampleOffset */)
{
    // Release all voices playing this note
    for (auto& voice : voices)
    {
        if (voice->isActive() && voice->getCurrentNote() == midiNote)
        {
            voice->stopNote(true);
        }
    }

    activeNotes.erase(midiNote);
}

void Sampler::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void Sampler::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Sample management

bool Sampler::loadSample(const juce::File& file, int rootNote, int lowNote, int highNote)
{
    if (!file.existsAsFile())
        return false;

    // Create reader
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader)
        return false;

    // Load into buffer
    juce::AudioBuffer<float> sampleBuffer(static_cast<int>(reader->numChannels),
                                         static_cast<int>(reader->lengthInSamples));
    reader->read(&sampleBuffer, 0, static_cast<int>(reader->lengthInSamples), 0, true, true);

    // Generate unique ID from filename and root note
    juce::String zoneId = file.getFileNameWithoutExtension() + "_" + juce::String(rootNote);
    juce::String zoneName = file.getFileNameWithoutExtension();

    // Use rootNote for bounds if not specified
    if (lowNote < 0) lowNote = rootNote;
    if (highNote < 0) highNote = rootNote;

    return loadSample(zoneId, zoneName, sampleBuffer, reader->sampleRate,
                     rootNote, lowNote, highNote);
}

bool Sampler::loadSample(const juce::String& zoneId, const juce::String& name,
                        const juce::AudioBuffer<float>& samples, double sr,
                        int rootNote, int lowNote, int highNote)
{
    // Create new zone
    auto zone = std::make_unique<SampleZone>(zoneId, name, rootNote);
    zone->sampleData = samples;
    zone->sampleRate = sr;
    zone->lowNote = lowNote >= 0 ? lowNote : rootNote;
    zone->highNote = highNote >= 0 ? highNote : rootNote;
    zone->loopStart = 0;
    zone->loopEnd = samples.getNumSamples();

    zones.push_back(std::move(zone));

    return true;
}

void Sampler::removeSample(const juce::String& zoneId)
{
    zones.erase(std::remove_if(zones.begin(), zones.end(),
        [&zoneId](const std::unique_ptr<SampleZone>& zone)
        {
            return zone->id == zoneId;
        }), zones.end());
}

void Sampler::clearAllSamples()
{
    allNotesOff();
    zones.clear();
}

std::vector<SampleZone*> Sampler::getZones()
{
    std::vector<SampleZone*> result;
    for (auto& zone : zones)
    {
        result.push_back(zone.get());
    }
    return result;
}

const SampleZone* Sampler::findZoneForNote(int midiNote) const
{
    // Look for exact match or zone that contains this note
    for (const auto& zone : zones)
    {
        if (midiNote >= zone->lowNote && midiNote <= zone->highNote)
        {
            return zone.get();
        }
    }

    // If no exact zone, find closest zone
    const SampleZone* closestZone = nullptr;
    int closestDistance = 128;

    for (const auto& zone : zones)
    {
        int distance = std::abs(midiNote - zone->rootNote);
        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestZone = zone.get();
        }
    }

    return closestZone;
}

//==============================================================================
// Parameter updates

void Sampler::updateVoiceParameters()
{
    for (auto& voice : voices)
    {
        // Amp envelope
        voice->setAmpEnvelope(
            getParameter("amp_attack"),
            getParameter("amp_decay"),
            getParameter("amp_sustain"),
            getParameter("amp_release")
        );

        // Filter
        voice->setFilterCutoff(getParameter("filter_cutoff"));
        voice->setFilterResonance(getParameter("filter_resonance"));
        voice->setFilterEnvAmount(getParameter("filter_env_amount"));
        voice->setFilterEnvelope(
            getParameter("filter_attack"),
            getParameter("filter_decay"),
            getParameter("filter_sustain"),
            getParameter("filter_release")
        );

        // Playback
        voice->setTranspose(static_cast<int>(getParameter("transpose")));
        voice->setFineTune(getParameter("fine_tune"));
        voice->setStartPosition(getParameter("start"));
        voice->setLoopMode(getParameterEnum("loop_mode") == 1); // 0=Off, 1=Forward
    }
}

void Sampler::onParameterChanged(const juce::String& name, float value)
{
    updateVoiceParameters();
}

void Sampler::onParameterEnumChanged(const juce::String& name, int index)
{
    updateVoiceParameters();
}

//==============================================================================
// Presets

std::vector<SynthPreset> Sampler::getPresets() const
{
    std::vector<SynthPreset> presets;

    // Default
    {
        SynthPreset p;
        p.name = "Default";
        p.category = "Basic";
        p.values["amp_attack"] = 0.005f;
        p.values["amp_release"] = 0.3f;
        p.values["transpose"] = 0.0f;
        p.values["fine_tune"] = 0.0f;
        p.enumValues["loop_mode"] = 0; // Off
        p.values["start"] = 0.0f;
        p.values["master_volume"] = 0.7f;
        p.values["filter_cutoff"] = 20000.0f;
        p.values["filter_resonance"] = 0.1f;
        presets.push_back(p);
    }

    // Pad
    {
        SynthPreset p;
        p.name = "Pad";
        p.category = "Ambient";
        p.values["amp_attack"] = 0.5f;
        p.values["amp_release"] = 2.0f;
        p.values["transpose"] = 0.0f;
        p.values["fine_tune"] = 0.0f;
        p.enumValues["loop_mode"] = 1; // Forward
        p.values["start"] = 0.0f;
        p.values["master_volume"] = 0.5f;
        p.values["filter_cutoff"] = 5000.0f;
        p.values["filter_resonance"] = 0.2f;
        presets.push_back(p);
    }

    // Pluck
    {
        SynthPreset p;
        p.name = "Pluck";
        p.category = "Percussive";
        p.values["amp_attack"] = 0.001f;
        p.values["amp_release"] = 0.8f;
        p.values["transpose"] = 0.0f;
        p.values["fine_tune"] = 0.0f;
        p.enumValues["loop_mode"] = 0; // Off
        p.values["start"] = 0.0f;
        p.values["master_volume"] = 0.8f;
        p.values["filter_cutoff"] = 20000.0f;
        p.values["filter_env_amount"] = 3000.0f;
        p.values["filter_attack"] = 0.001f;
        p.values["filter_decay"] = 0.15f;
        p.values["filter_sustain"] = 0.1f;
        presets.push_back(p);
    }

    // One Shot
    {
        SynthPreset p;
        p.name = "One Shot";
        p.category = "FX";
        p.values["amp_attack"] = 0.0f;
        p.values["amp_sustain"] = 1.0f;
        p.values["amp_release"] = 5.0f;
        p.values["transpose"] = 0.0f;
        p.values["fine_tune"] = 0.0f;
        p.enumValues["loop_mode"] = 0; // Off
        p.values["start"] = 0.0f;
        p.values["master_volume"] = 0.7f;
        p.values["filter_cutoff"] = 20000.0f;
        presets.push_back(p);
    }

    // Loop
    {
        SynthPreset p;
        p.name = "Loop";
        p.category = "Loop";
        p.values["amp_attack"] = 0.01f;
        p.values["amp_release"] = 0.5f;
        p.values["transpose"] = 0.0f;
        p.values["fine_tune"] = 0.0f;
        p.enumValues["loop_mode"] = 1; // Forward
        p.values["start"] = 0.0f;
        p.values["master_volume"] = 0.6f;
        p.values["filter_cutoff"] = 20000.0f;
        presets.push_back(p);
    }

    // Reverse (start from end)
    {
        SynthPreset p;
        p.name = "Reverse Effect";
        p.category = "FX";
        p.values["amp_attack"] = 0.5f;
        p.values["amp_release"] = 0.1f;
        p.values["transpose"] = 0.0f;
        p.values["fine_tune"] = 0.0f;
        p.enumValues["loop_mode"] = 0; // Off
        p.values["start"] = 0.9f; // Start near end
        p.values["master_volume"] = 0.7f;
        p.values["filter_cutoff"] = 10000.0f;
        presets.push_back(p);
    }

    return presets;
}
