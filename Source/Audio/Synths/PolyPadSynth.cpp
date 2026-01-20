#include "PolyPadSynth.h"
#include <cmath>

//==============================================================================
// Waveform generation
//==============================================================================

float PolyPadSynthVoice::generateWave(PadWaveType type, double phase)
{
    switch (type)
    {
        case PadWaveType::Sine:
            return std::sin(phase * juce::MathConstants<double>::twoPi);

        case PadWaveType::Triangle:
        {
            double t = std::fmod(phase, 1.0);
            if (t < 0.25)
                return static_cast<float>(t * 4.0);
            else if (t < 0.75)
                return static_cast<float>(2.0 - t * 4.0);
            else
                return static_cast<float>(t * 4.0 - 4.0);
        }

        case PadWaveType::Sawtooth:
        {
            double t = std::fmod(phase, 1.0);
            return static_cast<float>(2.0 * t - 1.0);
        }

        case PadWaveType::Square:
        {
            double t = std::fmod(phase, 1.0);
            return t < 0.5 ? 1.0f : -1.0f;
        }
    }

    return 0.0f;
}

float PolyPadSynthVoice::Oscillator::generate(double frequency, double sr)
{
    float sample = generateWave(waveType, phase);

    // Advance phase
    double phaseIncrement = frequency / sr;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * level;
}

//==============================================================================
// PolyPadSynthVoice Implementation
//==============================================================================

PolyPadSynthVoice::PolyPadSynthVoice()
{
    // Initialize 4 oscillators
    // Oscillators 0, 2 are osc1 type
    osc[0].waveType = PadWaveType::Sawtooth;
    osc[0].detuneCents = 0.0f;
    osc[2].waveType = PadWaveType::Sawtooth;
    osc[2].detuneCents = -7.0f; // Opposite detune for width

    // Oscillators 1, 3 are osc2 type
    osc[1].waveType = PadWaveType::Sawtooth;
    osc[1].detuneCents = 7.0f;
    osc[3].waveType = PadWaveType::Sawtooth;
    osc[3].detuneCents = 14.0f; // Double detune

    // Set initial crossfade levels
    setOscMix(0.5f);

    filterEnvelope.setParameters(filterEnvParams);
}

void PolyPadSynthVoice::prepareToPlay(double sr, int blockSize)
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

    // Prepare chorus
    chorus.prepare(spec);
    chorus.setRate(chorusRate);
    chorus.setDepth(chorusDepth);
    chorus.setMix(chorusWet);
    chorus.setCentreDelay(2.5f); // 2.5ms delay time
    chorus.setFeedback(0.0f);
    chorus.reset();
}

void PolyPadSynthVoice::reset()
{
    SynthVoice::reset();

    for (auto& oscillator : osc)
        oscillator.reset();

    filter.reset();
    filterEnvelope.reset();
    chorus.reset();
}

void PolyPadSynthVoice::onNoteStart()
{
    // Reset oscillator phases for consistent attack
    for (auto& oscillator : osc)
        oscillator.phase = 0.0;

    filterEnvelope.noteOn();
}

void PolyPadSynthVoice::onNoteStop()
{
    filterEnvelope.noteOff();
}

double PolyPadSynthVoice::getOscFrequency(const Oscillator& oscillator, float baseFreq)
{
    // Apply detune in cents
    return baseFreq * std::pow(2.0, oscillator.detuneCents / 1200.0);
}

void PolyPadSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                        int startSample, int numSamples)
{
    if (!isActive())
        return;

    // Temporary mono buffer for chorus processing
    juce::AudioBuffer<float> monoBuffer(1, numSamples);
    monoBuffer.clear();
    auto* mono = monoBuffer.getWritePointer(0);

    auto* outputL = buffer.getWritePointer(0, startSample);
    auto* outputR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // Update portamento
        float baseFreq = getNextFrequency();

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

        // Generate all 4 oscillators
        float osc0Sample = osc[0].generate(getOscFrequency(osc[0], baseFreq), sampleRate);
        float osc1Sample = osc[1].generate(getOscFrequency(osc[1], baseFreq), sampleRate);
        float osc2Sample = osc[2].generate(getOscFrequency(osc[2], baseFreq), sampleRate);
        float osc3Sample = osc[3].generate(getOscFrequency(osc[3], baseFreq), sampleRate);

        // Mix oscillators (levels already applied in generate())
        float mixed = osc0Sample + osc1Sample + osc2Sample + osc3Sample;

        // Normalize to prevent clipping
        mixed *= 0.5f;

        // Calculate filter cutoff with envelope modulation
        float modulatedCutoff = filterCutoff;

        // Apply filter envelope amount
        // Convert from frequency offset to proper modulation
        float envModulation = filterEnvAmount * filterEnv;
        modulatedCutoff += envModulation;
        modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);

        // Update filter
        filter.setCutoffFrequency(modulatedCutoff);

        // Process through filter
        float filtered = filter.processSample(0, mixed);

        // Apply amp envelope and velocity
        float output = filtered * ampEnv * velocity;

        // Write to mono buffer for chorus
        mono[i] = output;

        // Update voice age
        incrementAge(1);
    }

    // Process chorus on mono buffer
    juce::dsp::AudioBlock<float> block(monoBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chorus.process(context);

    // Mix chorus output to stereo buffer
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = mono[i] * 0.25f; // Additional gain reduction for pad layering
        outputL[i] += sample;
        if (outputR != nullptr)
            outputR[i] += sample;
    }
}

//==============================================================================
// Voice parameter setters

void PolyPadSynthVoice::setOsc1WaveType(PadWaveType type)
{
    osc[0].waveType = type;
    osc[2].waveType = type;
}

void PolyPadSynthVoice::setOsc2WaveType(PadWaveType type)
{
    osc[1].waveType = type;
    osc[3].waveType = type;
}

void PolyPadSynthVoice::setOsc2Detune(float cents)
{
    cents = juce::jlimit(-50.0f, 50.0f, cents);
    osc[1].detuneCents = cents;
    osc[2].detuneCents = -cents; // Opposite for width
    osc[3].detuneCents = cents * 2.0f; // Double for extra width
}

void PolyPadSynthVoice::setOscMix(float mix)
{
    oscMix = juce::jlimit(0.0f, 1.0f, mix);

    // Equal-power crossfade for smooth transition
    float osc1Gain = std::sqrt(1.0f - oscMix) * 0.5f;
    float osc2Gain = std::sqrt(oscMix) * 0.5f;

    // Oscillators 0, 2 are osc1 pair
    osc[0].level = osc1Gain;
    osc[2].level = osc1Gain;

    // Oscillators 1, 3 are osc2 pair
    osc[1].level = osc2Gain;
    osc[3].level = osc2Gain;
}

void PolyPadSynthVoice::setFilterCutoff(float frequency)
{
    filterCutoff = juce::jlimit(20.0f, 20000.0f, frequency);
}

void PolyPadSynthVoice::setFilterResonance(float resonance)
{
    // Map 0.1-20 (TypeScript) to 0-1 (JUCE)
    // Use logarithmic mapping
    float q = juce::jlimit(0.1f, 20.0f, resonance);
    filterResonance = juce::jmap(std::log10(q), std::log10(0.1f), std::log10(20.0f), 0.0f, 1.0f);
    filter.setResonance(filterResonance);
}

void PolyPadSynthVoice::setFilterType(PadFilterType type)
{
    filterType = type;
    switch (type)
    {
        case PadFilterType::LowPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case PadFilterType::HighPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case PadFilterType::BandPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }
}

void PolyPadSynthVoice::setFilterEnvAmount(float amount)
{
    filterEnvAmount = juce::jlimit(-10000.0f, 10000.0f, amount);
}

void PolyPadSynthVoice::setFilterEnvelope(float attack, float decay, float sustain, float release)
{
    filterEnvParams.attack = juce::jmax(0.001f, attack);
    filterEnvParams.decay = juce::jmax(0.001f, decay);
    filterEnvParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    filterEnvParams.release = juce::jmax(0.001f, release);
    filterEnvelope.setParameters(filterEnvParams);
}

void PolyPadSynthVoice::setChorusRate(float rate)
{
    chorusRate = juce::jlimit(0.1f, 10.0f, rate);
    chorus.setRate(chorusRate);
}

void PolyPadSynthVoice::setChorusDepth(float depth)
{
    chorusDepth = juce::jlimit(0.0f, 1.0f, depth);
    chorus.setDepth(chorusDepth);
}

void PolyPadSynthVoice::setChorusWet(float wet)
{
    chorusWet = juce::jlimit(0.0f, 1.0f, wet);
    // Per-voice chorus uses half the wet amount
    chorus.setMix(chorusWet * 0.5f);
}

//==============================================================================
// PolyPadSynth Implementation
//==============================================================================

PolyPadSynth::PolyPadSynth()
{
    initializeParameters();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<PolyPadSynthVoice>();
    }
}

PolyPadSynth::~PolyPadSynth()
{
    killAllNotes();
}

void PolyPadSynth::initializeParameters()
{
    // Oscillator settings
    addEnumParameter("osc1_wave", "Osc 1 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 2);
    addEnumParameter("osc2_wave", "Osc 2 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 2);
    addParameter("osc2_detune", "Osc 2 Detune", 7.0f, -50.0f, 50.0f, 1.0f);
    addParameter("osc_mix", "Oscillator Mix", 0.5f, 0.0f, 1.0f);

    // Filter
    addParameter("filter_cutoff", "Filter Cutoff", 2000.0f, 20.0f, 20000.0f);
    addParameter("filter_resonance", "Filter Resonance", 1.0f, 0.1f, 20.0f);
    addEnumParameter("filter_type", "Filter Type", {"LowPass", "HighPass", "BandPass"}, 0);
    addParameter("filter_env_amount", "Filter Env Amount", 2000.0f, -10000.0f, 10000.0f);

    // Filter Envelope
    addParameter("filter_attack", "Filter Attack", 0.3f, 0.001f, 4.0f);
    addParameter("filter_decay", "Filter Decay", 0.5f, 0.001f, 4.0f);
    addParameter("filter_sustain", "Filter Sustain", 0.5f, 0.0f, 1.0f);
    addParameter("filter_release", "Filter Release", 1.0f, 0.001f, 8.0f);

    // Amp Envelope
    addParameter("amp_attack", "Amp Attack", 0.5f, 0.001f, 4.0f);
    addParameter("amp_decay", "Amp Decay", 0.5f, 0.001f, 4.0f);
    addParameter("amp_sustain", "Amp Sustain", 0.7f, 0.0f, 1.0f);
    addParameter("amp_release", "Amp Release", 2.0f, 0.001f, 10.0f);

    // Chorus settings
    addParameter("chorus_rate", "Chorus Rate", 0.8f, 0.1f, 10.0f);
    addParameter("chorus_depth", "Chorus Depth", 0.7f, 0.0f, 1.0f);
    addParameter("chorus_wet", "Chorus Mix", 0.4f, 0.0f, 1.0f);

    // Master
    addParameter("volume", "Volume", 0.5f, 0.0f, 1.0f);
}

void PolyPadSynth::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    // Prepare all voices
    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    // Prepare master chorus
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 2; // Stereo

    masterChorus.prepare(spec);
    masterChorus.setRate(getParameter("chorus_rate"));
    masterChorus.setDepth(getParameter("chorus_depth"));
    masterChorus.setMix(getParameter("chorus_wet"));
    masterChorus.setCentreDelay(3.5f); // 3.5ms delay time
    masterChorus.setFeedback(0.0f);
    masterChorus.reset();

    updateVoiceParameters();
}

void PolyPadSynth::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }
}

void PolyPadSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear buffer
    buffer.clear();

    // Process MIDI
    processMidiMessages(midiMessages);

    // Render all active voices
    for (auto& voice : voices)
    {
        if (voice->isActive())
        {
            voice->renderNextBlock(buffer, 0, buffer.getNumSamples());
        }
    }

    // Apply master chorus
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    masterChorus.process(context);

    // Apply master volume
    float masterVol = getParameter("volume");
    buffer.applyGain(masterVol);
}

//==============================================================================
// Voice management

PolyPadSynthVoice* PolyPadSynth::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

PolyPadSynthVoice* PolyPadSynth::findVoiceToSteal()
{
    // Find oldest voice in release, or oldest overall
    PolyPadSynthVoice* oldestRelease = nullptr;
    PolyPadSynthVoice* oldest = nullptr;
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

PolyPadSynthVoice* PolyPadSynth::findVoicePlayingNote(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice->isActive() && voice->getCurrentNote() == midiNote)
            return voice.get();
    }
    return nullptr;
}

void PolyPadSynth::noteOn(int midiNote, float velocity, int /* sampleOffset */)
{
    PolyPadSynthVoice* voice = findFreeVoice();
    if (!voice)
        voice = findVoiceToSteal();

    if (voice)
    {
        voice->startNote(midiNote, velocity);
    }

    activeNotes.insert(midiNote);
}

void PolyPadSynth::noteOff(int midiNote, int /* sampleOffset */)
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

void PolyPadSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void PolyPadSynth::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Parameter updates

void PolyPadSynth::updateVoiceParameters()
{
    for (auto& voice : voices)
    {
        // Oscillators
        voice->setOsc1WaveType(static_cast<PadWaveType>(getParameterEnum("osc1_wave")));
        voice->setOsc2WaveType(static_cast<PadWaveType>(getParameterEnum("osc2_wave")));
        voice->setOsc2Detune(getParameter("osc2_detune"));
        voice->setOscMix(getParameter("osc_mix"));

        // Filter
        voice->setFilterCutoff(getParameter("filter_cutoff"));
        voice->setFilterResonance(getParameter("filter_resonance"));
        voice->setFilterType(static_cast<PadFilterType>(getParameterEnum("filter_type")));
        voice->setFilterEnvAmount(getParameter("filter_env_amount"));
        voice->setFilterEnvelope(
            getParameter("filter_attack"),
            getParameter("filter_decay"),
            getParameter("filter_sustain"),
            getParameter("filter_release")
        );

        // Amp envelope
        voice->setAmpEnvelope(
            getParameter("amp_attack"),
            getParameter("amp_decay"),
            getParameter("amp_sustain"),
            getParameter("amp_release")
        );

        // Chorus
        voice->setChorusRate(getParameter("chorus_rate"));
        voice->setChorusDepth(getParameter("chorus_depth"));
        voice->setChorusWet(getParameter("chorus_wet"));
    }

    // Update master chorus
    masterChorus.setRate(getParameter("chorus_rate"));
    masterChorus.setDepth(getParameter("chorus_depth"));
    masterChorus.setMix(getParameter("chorus_wet"));
}

void PolyPadSynth::onParameterChanged(const juce::String& name, float value)
{
    updateVoiceParameters();
}

void PolyPadSynth::onParameterEnumChanged(const juce::String& name, int index)
{
    updateVoiceParameters();
}

//==============================================================================
// Presets
//==============================================================================

std::vector<SynthPreset> PolyPadSynth::getPresets() const
{
    std::vector<SynthPreset> presets;

    // Warm Pad
    {
        SynthPreset p;
        p.name = "Warm Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 7.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 1500.0f;
        p.values["filter_resonance"] = 1.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 1000.0f;
        p.values["filter_attack"] = 0.5f;
        p.values["filter_decay"] = 0.5f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.8f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 2.0f;
        p.values["chorus_rate"] = 0.8f;
        p.values["chorus_depth"] = 0.7f;
        p.values["chorus_wet"] = 0.4f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // String Pad
    {
        SynthPreset p;
        p.name = "String Pad";
        p.category = "Strings";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 12.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 3000.0f;
        p.values["filter_resonance"] = 0.5f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 500.0f;
        p.values["filter_attack"] = 0.3f;
        p.values["filter_decay"] = 0.3f;
        p.values["filter_sustain"] = 0.8f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.4f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 1.5f;
        p.values["chorus_rate"] = 1.2f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.5f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Ethereal
    {
        SynthPreset p;
        p.name = "Ethereal";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 0; // Sine
        p.enumValues["osc2_wave"] = 1; // Triangle
        p.values["osc2_detune"] = 5.0f;
        p.values["osc_mix"] = 0.4f;
        p.values["filter_cutoff"] = 4000.0f;
        p.values["filter_resonance"] = 2.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 1.0f;
        p.values["filter_decay"] = 1.0f;
        p.values["filter_sustain"] = 0.3f;
        p.values["filter_release"] = 3.0f;
        p.values["amp_attack"] = 1.5f;
        p.values["amp_decay"] = 1.0f;
        p.values["amp_sustain"] = 0.6f;
        p.values["amp_release"] = 4.0f;
        p.values["chorus_rate"] = 0.5f;
        p.values["chorus_depth"] = 0.8f;
        p.values["chorus_wet"] = 0.6f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Dark Pad
    {
        SynthPreset p;
        p.name = "Dark Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 3; // Square
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 10.0f;
        p.values["osc_mix"] = 0.6f;
        p.values["filter_cutoff"] = 800.0f;
        p.values["filter_resonance"] = 3.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 500.0f;
        p.values["filter_attack"] = 0.8f;
        p.values["filter_decay"] = 0.8f;
        p.values["filter_sustain"] = 0.4f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.6f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.6f;
        p.values["amp_release"] = 2.5f;
        p.values["chorus_rate"] = 0.4f;
        p.values["chorus_depth"] = 0.9f;
        p.values["chorus_wet"] = 0.5f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Analog Pad
    {
        SynthPreset p;
        p.name = "Analog Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 3; // Square
        p.values["osc2_detune"] = 8.0f;
        p.values["osc_mix"] = 0.4f;
        p.values["filter_cutoff"] = 2500.0f;
        p.values["filter_resonance"] = 4.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 3000.0f;
        p.values["filter_attack"] = 0.4f;
        p.values["filter_decay"] = 0.6f;
        p.values["filter_sustain"] = 0.3f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.3f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 1.5f;
        p.values["chorus_rate"] = 0.6f;
        p.values["chorus_depth"] = 0.6f;
        p.values["chorus_wet"] = 0.35f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Choir
    {
        SynthPreset p;
        p.name = "Choir";
        p.category = "Vocal";
        p.enumValues["osc1_wave"] = 0; // Sine
        p.enumValues["osc2_wave"] = 0; // Sine
        p.values["osc2_detune"] = 3.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 2000.0f;
        p.values["filter_resonance"] = 5.0f;
        p.enumValues["filter_type"] = 2; // BandPass
        p.values["filter_env_amount"] = 1000.0f;
        p.values["filter_attack"] = 0.6f;
        p.values["filter_decay"] = 0.4f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 1.2f;
        p.values["amp_attack"] = 0.5f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 1.5f;
        p.values["chorus_rate"] = 1.0f;
        p.values["chorus_depth"] = 0.4f;
        p.values["chorus_wet"] = 0.3f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Bright Pad
    {
        SynthPreset p;
        p.name = "Bright Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 15.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 6000.0f;
        p.values["filter_resonance"] = 1.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.2f;
        p.values["filter_decay"] = 0.4f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.2f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 1.0f;
        p.values["chorus_rate"] = 1.5f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.4f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Glass Pad
    {
        SynthPreset p;
        p.name = "Glass Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 1; // Triangle
        p.enumValues["osc2_wave"] = 0; // Sine
        p.values["osc2_detune"] = 2.0f;
        p.values["osc_mix"] = 0.6f;
        p.values["filter_cutoff"] = 8000.0f;
        p.values["filter_resonance"] = 6.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 3000.0f;
        p.values["filter_attack"] = 0.1f;
        p.values["filter_decay"] = 1.0f;
        p.values["filter_sustain"] = 0.2f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.3f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.5f;
        p.values["amp_release"] = 2.5f;
        p.values["chorus_rate"] = 2.0f;
        p.values["chorus_depth"] = 0.3f;
        p.values["chorus_wet"] = 0.4f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Lush Pad
    {
        SynthPreset p;
        p.name = "Lush Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 10.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 2000.0f;
        p.values["filter_resonance"] = 1.5f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 1500.0f;
        p.values["filter_attack"] = 0.8f;
        p.values["filter_decay"] = 0.6f;
        p.values["filter_sustain"] = 0.6f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 1.0f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 2.5f;
        p.values["chorus_rate"] = 0.7f;
        p.values["chorus_depth"] = 0.8f;
        p.values["chorus_wet"] = 0.5f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // Shimmer Pad
    {
        SynthPreset p;
        p.name = "Shimmer Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 1; // Triangle
        p.values["osc2_detune"] = 6.0f;
        p.values["osc_mix"] = 0.45f;
        p.values["filter_cutoff"] = 5000.0f;
        p.values["filter_resonance"] = 2.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 2500.0f;
        p.values["filter_attack"] = 0.5f;
        p.values["filter_decay"] = 0.7f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.6f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 3.0f;
        p.values["chorus_rate"] = 1.2f;
        p.values["chorus_depth"] = 0.6f;
        p.values["chorus_wet"] = 0.55f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // Dreamy Pad
    {
        SynthPreset p;
        p.name = "Dreamy Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 0; // Sine
        p.enumValues["osc2_wave"] = 1; // Triangle
        p.values["osc2_detune"] = 8.0f;
        p.values["osc_mix"] = 0.4f;
        p.values["filter_cutoff"] = 3000.0f;
        p.values["filter_resonance"] = 1.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 1.2f;
        p.values["filter_decay"] = 1.0f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 3.0f;
        p.values["amp_attack"] = 1.5f;
        p.values["amp_decay"] = 0.8f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 4.0f;
        p.values["chorus_rate"] = 0.4f;
        p.values["chorus_depth"] = 0.9f;
        p.values["chorus_wet"] = 0.6f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Ambient Pad
    {
        SynthPreset p;
        p.name = "Ambient Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 0; // Sine
        p.enumValues["osc2_wave"] = 0; // Sine
        p.values["osc2_detune"] = 4.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 2500.0f;
        p.values["filter_resonance"] = 1.5f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 1000.0f;
        p.values["filter_attack"] = 2.0f;
        p.values["filter_decay"] = 1.0f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 4.0f;
        p.values["amp_attack"] = 2.5f;
        p.values["amp_decay"] = 1.0f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 5.0f;
        p.values["chorus_rate"] = 0.3f;
        p.values["chorus_depth"] = 0.7f;
        p.values["chorus_wet"] = 0.5f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Trance Pad
    {
        SynthPreset p;
        p.name = "Trance Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 6.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 3500.0f;
        p.values["filter_resonance"] = 3.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 2500.0f;
        p.values["filter_attack"] = 0.3f;
        p.values["filter_decay"] = 0.5f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.25f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 1.0f;
        p.values["chorus_rate"] = 1.0f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.4f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Sweeping Pad
    {
        SynthPreset p;
        p.name = "Sweeping Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 3; // Square
        p.values["osc2_detune"] = 9.0f;
        p.values["osc_mix"] = 0.55f;
        p.values["filter_cutoff"] = 1000.0f;
        p.values["filter_resonance"] = 5.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 4000.0f;
        p.values["filter_attack"] = 2.5f;
        p.values["filter_decay"] = 1.5f;
        p.values["filter_sustain"] = 0.4f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.8f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 2.0f;
        p.values["chorus_rate"] = 0.6f;
        p.values["chorus_depth"] = 0.7f;
        p.values["chorus_wet"] = 0.45f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Vintage Pad
    {
        SynthPreset p;
        p.name = "Vintage Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 3; // Square
        p.values["osc2_detune"] = 12.0f;
        p.values["osc_mix"] = 0.45f;
        p.values["filter_cutoff"] = 1800.0f;
        p.values["filter_resonance"] = 3.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 1200.0f;
        p.values["filter_attack"] = 0.6f;
        p.values["filter_decay"] = 0.7f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.5f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 1.5f;
        p.values["chorus_rate"] = 0.5f;
        p.values["chorus_depth"] = 0.8f;
        p.values["chorus_wet"] = 0.5f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Cinematic Pad
    {
        SynthPreset p;
        p.name = "Cinematic Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 15.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 2000.0f;
        p.values["filter_resonance"] = 2.0f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 3000.0f;
        p.values["filter_attack"] = 1.5f;
        p.values["filter_decay"] = 1.0f;
        p.values["filter_sustain"] = 0.5f;
        p.values["filter_release"] = 3.0f;
        p.values["amp_attack"] = 2.0f;
        p.values["amp_decay"] = 1.0f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 4.0f;
        p.values["chorus_rate"] = 0.4f;
        p.values["chorus_depth"] = 0.85f;
        p.values["chorus_wet"] = 0.55f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // Soft Pad
    {
        SynthPreset p;
        p.name = "Soft Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 1; // Triangle
        p.enumValues["osc2_wave"] = 0; // Sine
        p.values["osc2_detune"] = 5.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 2500.0f;
        p.values["filter_resonance"] = 0.8f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 800.0f;
        p.values["filter_attack"] = 0.7f;
        p.values["filter_decay"] = 0.5f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.6f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 1.8f;
        p.values["chorus_rate"] = 0.8f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.4f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Rich Pad
    {
        SynthPreset p;
        p.name = "Rich Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2; // Sawtooth
        p.values["osc2_detune"] = 18.0f;
        p.values["osc_mix"] = 0.5f;
        p.values["filter_cutoff"] = 3500.0f;
        p.values["filter_resonance"] = 1.5f;
        p.enumValues["filter_type"] = 0; // LowPass
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.5f;
        p.values["filter_decay"] = 0.6f;
        p.values["filter_sustain"] = 0.6f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.6f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 2.0f;
        p.values["chorus_rate"] = 0.9f;
        p.values["chorus_depth"] = 0.75f;
        p.values["chorus_wet"] = 0.5f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    return presets;
}
