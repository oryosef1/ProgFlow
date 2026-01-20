#include "FMSynth.h"
#include <cmath>

//==============================================================================
// FMSynthVoice Implementation
//==============================================================================

FMSynthVoice::FMSynthVoice()
{
    carrier.ratio = 1.0f;
    modulator1.ratio = 2.0f;
    modulator2.ratio = 3.0f;

    modEnv1.setParameters(modEnv1Params);
    modEnv2.setParameters(modEnv2Params);
}

void FMSynthVoice::prepareToPlay(double sr, int blockSize)
{
    SynthVoice::prepareToPlay(sr, blockSize);

    modEnv1.setSampleRate(sr);
    modEnv2.setSampleRate(sr);
}

void FMSynthVoice::reset()
{
    SynthVoice::reset();

    carrier.reset();
    modulator1.reset();
    modulator2.reset();

    modEnv1.reset();
    modEnv2.reset();

    feedbackSample = 0.0f;
}

void FMSynthVoice::onNoteStart()
{
    // Reset oscillator phases for consistent attack
    carrier.phase = 0.0;
    modulator1.phase = 0.0;
    modulator2.phase = 0.0;

    modEnv1.noteOn();
    modEnv2.noteOn();

    feedbackSample = 0.0f;
}

void FMSynthVoice::onNoteStop()
{
    modEnv1.noteOff();
    modEnv2.noteOff();
}

float FMSynthVoice::SineOscillator::process(double baseFreq, double sr, float modulation)
{
    // Calculate instantaneous frequency with FM modulation
    double instantFreq = baseFreq * ratio + modulation;

    // Generate sine wave
    float sample = std::sin(phase * juce::MathConstants<double>::twoPi);

    // Advance phase
    double phaseIncrement = instantFreq / sr;
    phase += phaseIncrement;

    // Wrap phase to [0, 1)
    while (phase >= 1.0)
        phase -= 1.0;
    while (phase < 0.0)
        phase += 1.0;

    return sample;
}

float FMSynthVoice::processSample(double baseFreq)
{
    // Get envelope values
    float modEnv1Value = modEnv1.getNextSample();
    float modEnv2Value = modEnv2.getNextSample();

    float mod1Sample = 0.0f;
    float mod2Sample = 0.0f;
    float carrierSample = 0.0f;
    float mod1Modulation = 0.0f;
    float mod2Modulation = 0.0f;
    float carrierModulation = 0.0f;

    // Calculate modulation amounts (index * envelope * base frequency)
    float mod1Depth = mod1Index * modEnv1Value * static_cast<float>(baseFreq);
    float mod2Depth = mod2Index * modEnv2Value * static_cast<float>(baseFreq);

    // Apply feedback to carrier
    if (feedback > 0.0f)
    {
        carrierModulation += feedbackSample * feedback * static_cast<float>(baseFreq);
    }

    // Process based on algorithm
    switch (algorithm)
    {
        case FMAlgorithm::Serial_2_1_C: // mod2 → mod1 → carrier
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            mod1Modulation = mod2Sample * mod2Depth;
            mod1Sample = modulator1.process(baseFreq, sampleRate, mod1Modulation);
            carrierModulation += mod1Sample * mod1Depth;
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 1.0f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.0f;
            break;

        case FMAlgorithm::Parallel_12_C: // (mod1 + mod2) → carrier
            mod1Sample = modulator1.process(baseFreq, sampleRate, 0.0f);
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            carrierModulation += (mod1Sample * mod1Depth) + (mod2Sample * mod2Depth);
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 1.0f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.0f;
            break;

        case FMAlgorithm::Dual_1C_2: // mod1 → carrier, mod2 as 2nd voice
            mod1Sample = modulator1.process(baseFreq, sampleRate, 0.0f);
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            carrierModulation += mod1Sample * mod1Depth;
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 0.7f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.3f * modEnv2Value;
            break;

        case FMAlgorithm::YShape_21C_2: // mod2 → mod1 → carrier + mod2 direct
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            mod1Modulation = mod2Sample * mod2Depth;
            mod1Sample = modulator1.process(baseFreq, sampleRate, mod1Modulation);
            carrierModulation += mod1Sample * mod1Depth;
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 0.8f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.2f * modEnv2Value;
            break;

        case FMAlgorithm::Split_1C_2: // mod1 → carrier + mod2 output
            mod1Sample = modulator1.process(baseFreq, sampleRate, 0.0f);
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            carrierModulation += mod1Sample * mod1Depth;
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 0.7f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.3f * modEnv2Value;
            break;

        case FMAlgorithm::Serial_1_2_C: // mod1 → mod2 → carrier
            mod1Sample = modulator1.process(baseFreq, sampleRate, 0.0f);
            mod2Modulation = mod1Sample * mod1Depth;
            mod2Sample = modulator2.process(baseFreq, sampleRate, mod2Modulation);
            carrierModulation += mod2Sample * mod2Depth;
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 1.0f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.0f;
            break;

        case FMAlgorithm::Parallel_1C_2C: // mod1 → carrier + mod2 → carrier
            mod1Sample = modulator1.process(baseFreq, sampleRate, 0.0f);
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            carrierModulation += (mod1Sample * mod1Depth) + (mod2Sample * mod2Depth);
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 1.0f;
            mod1OutputGain = 0.0f;
            mod2OutputGain = 0.0f;
            break;

        case FMAlgorithm::Additive_C_1_2: // carrier + mod1 + mod2 (pure additive)
            mod1Sample = modulator1.process(baseFreq, sampleRate, 0.0f);
            mod2Sample = modulator2.process(baseFreq, sampleRate, 0.0f);
            carrierSample = carrier.process(baseFreq, sampleRate, carrierModulation);
            carrierGain = 0.4f;
            mod1OutputGain = 0.3f * modEnv1Value;
            mod2OutputGain = 0.3f * modEnv2Value;
            break;
    }

    // Mix outputs based on algorithm
    float output = (carrierSample * carrierGain) +
                   (mod1Sample * mod1OutputGain) +
                   (mod2Sample * mod2OutputGain);

    // Store for feedback
    feedbackSample = carrierSample;

    return output;
}

void FMSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                   int startSample, int numSamples)
{
    if (!isActive())
        return;

    auto* outputL = buffer.getWritePointer(0, startSample);
    auto* outputR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // Update portamento
        float baseFreq = getNextFrequency();

        // Get amp envelope value
        float ampEnv = ampEnvelope.getNextSample();

        // Check if voice should go idle
        if (state == VoiceState::Release && ampEnv < 0.0001f)
        {
            state = VoiceState::Idle;
            currentNote = -1;
            break;
        }

        // Process FM synthesis
        float sample = processSample(static_cast<double>(baseFreq));

        // Apply amp envelope and velocity
        float output = sample * ampEnv * velocity;

        // Write to buffer
        outputL[i] += output;
        if (outputR != nullptr)
            outputR[i] += output;

        // Update voice age
        incrementAge(1);
    }
}

//==============================================================================
// Voice setters

void FMSynthVoice::setAlgorithm(FMAlgorithm alg)
{
    algorithm = alg;
}

void FMSynthVoice::setCarrierRatio(float ratio)
{
    carrier.ratio = juce::jlimit(0.5f, 16.0f, ratio);
}

void FMSynthVoice::setMod1Ratio(float ratio)
{
    modulator1.ratio = juce::jlimit(0.5f, 16.0f, ratio);
}

void FMSynthVoice::setMod2Ratio(float ratio)
{
    modulator2.ratio = juce::jlimit(0.5f, 16.0f, ratio);
}

void FMSynthVoice::setMod1Index(float index)
{
    mod1Index = juce::jlimit(0.0f, 50.0f, index);
}

void FMSynthVoice::setMod2Index(float index)
{
    mod2Index = juce::jlimit(0.0f, 50.0f, index);
}

void FMSynthVoice::setFeedback(float fb)
{
    feedback = juce::jlimit(0.0f, 1.0f, fb);
}

void FMSynthVoice::setModEnvelope1(float attack, float decay, float sustain, float release)
{
    modEnv1Params.attack = juce::jmax(0.001f, attack);
    modEnv1Params.decay = juce::jmax(0.001f, decay);
    modEnv1Params.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    modEnv1Params.release = juce::jmax(0.001f, release);
    modEnv1.setParameters(modEnv1Params);
}

void FMSynthVoice::setModEnvelope2(float attack, float decay, float sustain, float release)
{
    modEnv2Params.attack = juce::jmax(0.001f, attack);
    modEnv2Params.decay = juce::jmax(0.001f, decay);
    modEnv2Params.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    modEnv2Params.release = juce::jmax(0.001f, release);
    modEnv2.setParameters(modEnv2Params);
}

//==============================================================================
// FMSynth Implementation
//==============================================================================

FMSynth::FMSynth()
{
    initializeParameters();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<FMSynthVoice>();
    }
}

FMSynth::~FMSynth()
{
    killAllNotes();
}

void FMSynth::initializeParameters()
{
    // Algorithm selection (1-8)
    addEnumParameter("algorithm", "Algorithm",
        {"Serial 2>1>C", "Parallel (1+2)>C", "Dual 1>C, 2", "Y-Shape 2>1>C+2",
         "Split 1>C+2", "Serial 1>2>C", "Parallel 1>C+2>C", "Additive C+1+2"}, 0);

    // Carrier settings
    addParameter("carrier_ratio", "Carrier Ratio", 1.0f, 0.5f, 16.0f, 0.5f);

    // Modulator 1 settings
    addParameter("mod1_ratio", "Mod 1 Ratio", 2.0f, 0.5f, 16.0f, 0.5f);
    addParameter("mod1_index", "Mod 1 Index", 5.0f, 0.0f, 50.0f, 0.1f);

    // Modulator 2 settings
    addParameter("mod2_ratio", "Mod 2 Ratio", 3.0f, 0.5f, 16.0f, 0.5f);
    addParameter("mod2_index", "Mod 2 Index", 2.0f, 0.0f, 50.0f, 0.1f);

    // Feedback
    addParameter("feedback", "Feedback", 0.0f, 0.0f, 1.0f, 0.01f);

    // Amplitude envelope
    addParameter("amp_attack", "Amp Attack", 0.01f, 0.001f, 2.0f, 0.001f);
    addParameter("amp_decay", "Amp Decay", 0.2f, 0.001f, 2.0f, 0.001f);
    addParameter("amp_sustain", "Amp Sustain", 0.5f, 0.0f, 1.0f, 0.01f);
    addParameter("amp_release", "Amp Release", 0.3f, 0.001f, 5.0f, 0.001f);

    // Modulator 1 envelope
    addParameter("mod1_attack", "Mod 1 Attack", 0.01f, 0.001f, 2.0f, 0.001f);
    addParameter("mod1_decay", "Mod 1 Decay", 0.3f, 0.001f, 2.0f, 0.001f);
    addParameter("mod1_sustain", "Mod 1 Sustain", 0.3f, 0.0f, 1.0f, 0.01f);
    addParameter("mod1_release", "Mod 1 Release", 0.2f, 0.001f, 5.0f, 0.001f);

    // Modulator 2 envelope
    addParameter("mod2_attack", "Mod 2 Attack", 0.01f, 0.001f, 2.0f, 0.001f);
    addParameter("mod2_decay", "Mod 2 Decay", 0.5f, 0.001f, 2.0f, 0.001f);
    addParameter("mod2_sustain", "Mod 2 Sustain", 0.2f, 0.0f, 1.0f, 0.01f);
    addParameter("mod2_release", "Mod 2 Release", 0.3f, 0.001f, 5.0f, 0.001f);

    // Master volume
    addParameter("volume", "Volume", 0.7f, 0.0f, 1.0f, 0.01f);
}

void FMSynth::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    updateVoiceParameters();
}

void FMSynth::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }
}

void FMSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // Apply master volume
    float volume = getParameter("volume");
    buffer.applyGain(volume);
}

//==============================================================================
// Voice management

FMSynthVoice* FMSynth::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

FMSynthVoice* FMSynth::findVoiceToSteal()
{
    // Find oldest voice in release, or oldest overall
    FMSynthVoice* oldestRelease = nullptr;
    FMSynthVoice* oldest = nullptr;
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

void FMSynth::noteOn(int midiNote, float velocity, int /* sampleOffset */)
{
    FMSynthVoice* voice = findFreeVoice();
    if (!voice)
        voice = findVoiceToSteal();

    if (voice)
    {
        voice->startNote(midiNote, velocity);
    }

    activeNotes.insert(midiNote);
}

void FMSynth::noteOff(int midiNote, int /* sampleOffset */)
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

void FMSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void FMSynth::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Parameter updates

void FMSynth::updateVoiceParameters()
{
    for (auto& voice : voices)
    {
        // Algorithm
        voice->setAlgorithm(static_cast<FMAlgorithm>(getParameterEnum("algorithm") + 1));

        // Operator ratios
        voice->setCarrierRatio(getParameter("carrier_ratio"));
        voice->setMod1Ratio(getParameter("mod1_ratio"));
        voice->setMod2Ratio(getParameter("mod2_ratio"));

        // Modulation indices
        voice->setMod1Index(getParameter("mod1_index"));
        voice->setMod2Index(getParameter("mod2_index"));

        // Feedback
        voice->setFeedback(getParameter("feedback"));

        // Amp envelope
        voice->setAmpEnvelope(
            getParameter("amp_attack"),
            getParameter("amp_decay"),
            getParameter("amp_sustain"),
            getParameter("amp_release")
        );

        // Modulator envelopes
        voice->setModEnvelope1(
            getParameter("mod1_attack"),
            getParameter("mod1_decay"),
            getParameter("mod1_sustain"),
            getParameter("mod1_release")
        );

        voice->setModEnvelope2(
            getParameter("mod2_attack"),
            getParameter("mod2_decay"),
            getParameter("mod2_sustain"),
            getParameter("mod2_release")
        );
    }
}

void FMSynth::onParameterChanged(const juce::String& name, float value)
{
    updateVoiceParameters();
}

void FMSynth::onParameterEnumChanged(const juce::String& name, int index)
{
    updateVoiceParameters();
}

//==============================================================================
// Presets

std::vector<SynthPreset> FMSynth::getPresets() const
{
    std::vector<SynthPreset> presets;

    // ============ KEYS ============
    {
        SynthPreset p;
        p.name = "Electric Piano";
        p.category = "Keys";
        p.enumValues["algorithm"] = 0; // Serial 2→1→C
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 14.0f;
        p.values["mod1_index"] = 2.5f;
        p.values["mod2_ratio"] = 1.0f;
        p.values["mod2_index"] = 0.5f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 1.5f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 0.5f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 1.2f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 0.3f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.8f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.2f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "DX E-Piano";
        p.category = "Keys";
        p.enumValues["algorithm"] = 1; // Parallel (1+2)→C
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 3.0f;
        p.values["mod2_ratio"] = 7.0f;
        p.values["mod2_index"] = 1.5f;
        p.values["feedback"] = 0.1f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 2.0f;
        p.values["amp_sustain"] = 0.1f;
        p.values["amp_release"] = 0.6f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 1.5f;
        p.values["mod1_sustain"] = 0.1f;
        p.values["mod1_release"] = 0.4f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.5f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.2f;
        p.values["volume"] = 0.65f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Bells";
        p.category = "Keys";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 3.5f;
        p.values["mod1_index"] = 8.0f;
        p.values["mod2_ratio"] = 7.0f;
        p.values["mod2_index"] = 4.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 4.0f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 2.0f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 3.0f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 1.5f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 2.0f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 1.0f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Tubular Bells";
        p.category = "Keys";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 3.46f;
        p.values["mod1_index"] = 6.0f;
        p.values["mod2_ratio"] = 5.2f;
        p.values["mod2_index"] = 3.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 6.0f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 3.0f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 4.0f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 2.0f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 2.5f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 1.5f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Vibraphone";
        p.category = "Keys";
        p.enumValues["algorithm"] = 1;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 4.0f;
        p.values["mod1_index"] = 4.0f;
        p.values["mod2_ratio"] = 1.0f;
        p.values["mod2_index"] = 2.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 2.5f;
        p.values["amp_sustain"] = 0.1f;
        p.values["amp_release"] = 1.5f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 2.0f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 1.0f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 1.0f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.5f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Celeste";
        p.category = "Keys";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 5.0f;
        p.values["mod1_index"] = 5.0f;
        p.values["mod2_ratio"] = 8.0f;
        p.values["mod2_index"] = 2.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 3.0f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 2.0f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 2.5f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 1.5f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 1.5f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 1.0f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // ============ BASS ============
    {
        SynthPreset p;
        p.name = "FM Bass";
        p.category = "Bass";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 15.0f;
        p.values["mod2_ratio"] = 2.0f;
        p.values["mod2_index"] = 5.0f;
        p.values["feedback"] = 0.3f;
        p.values["amp_attack"] = 0.005f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.6f;
        p.values["amp_release"] = 0.2f;
        p.values["mod1_attack"] = 0.005f;
        p.values["mod1_decay"] = 0.15f;
        p.values["mod1_sustain"] = 0.2f;
        p.values["mod1_release"] = 0.1f;
        p.values["mod2_attack"] = 0.005f;
        p.values["mod2_decay"] = 0.1f;
        p.values["mod2_sustain"] = 0.1f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.8f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Slap Bass";
        p.category = "Bass";
        p.enumValues["algorithm"] = 1;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 3.0f;
        p.values["mod1_index"] = 20.0f;
        p.values["mod2_ratio"] = 1.0f;
        p.values["mod2_index"] = 8.0f;
        p.values["feedback"] = 0.2f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.4f;
        p.values["amp_release"] = 0.15f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 0.08f;
        p.values["mod1_sustain"] = 0.1f;
        p.values["mod1_release"] = 0.08f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.12f;
        p.values["mod2_sustain"] = 0.15f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.75f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Synth Bass";
        p.category = "Bass";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 2.0f;
        p.values["mod1_index"] = 12.0f;
        p.values["mod2_ratio"] = 1.0f;
        p.values["mod2_index"] = 6.0f;
        p.values["feedback"] = 0.4f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 0.25f;
        p.values["amp_sustain"] = 0.5f;
        p.values["amp_release"] = 0.15f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 0.2f;
        p.values["mod1_sustain"] = 0.3f;
        p.values["mod1_release"] = 0.1f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.15f;
        p.values["mod2_sustain"] = 0.2f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Rubber Bass";
        p.category = "Bass";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 10.0f;
        p.values["mod2_ratio"] = 3.0f;
        p.values["mod2_index"] = 3.0f;
        p.values["feedback"] = 0.5f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.3f;
        p.values["amp_release"] = 0.2f;
        p.values["mod1_attack"] = 0.01f;
        p.values["mod1_decay"] = 0.4f;
        p.values["mod1_sustain"] = 0.2f;
        p.values["mod1_release"] = 0.15f;
        p.values["mod2_attack"] = 0.01f;
        p.values["mod2_decay"] = 0.3f;
        p.values["mod2_sustain"] = 0.1f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    // ============ LEAD ============
    {
        SynthPreset p;
        p.name = "Synth Lead";
        p.category = "Lead";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 2.0f;
        p.values["mod1_index"] = 8.0f;
        p.values["mod2_ratio"] = 3.0f;
        p.values["mod2_index"] = 4.0f;
        p.values["feedback"] = 0.2f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_decay"] = 0.1f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 0.3f;
        p.values["mod1_attack"] = 0.01f;
        p.values["mod1_decay"] = 0.1f;
        p.values["mod1_sustain"] = 0.6f;
        p.values["mod1_release"] = 0.2f;
        p.values["mod2_attack"] = 0.01f;
        p.values["mod2_decay"] = 0.15f;
        p.values["mod2_sustain"] = 0.4f;
        p.values["mod2_release"] = 0.15f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Bright Lead";
        p.category = "Lead";
        p.enumValues["algorithm"] = 1;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 3.0f;
        p.values["mod1_index"] = 10.0f;
        p.values["mod2_ratio"] = 5.0f;
        p.values["mod2_index"] = 5.0f;
        p.values["feedback"] = 0.15f;
        p.values["amp_attack"] = 0.005f;
        p.values["amp_decay"] = 0.1f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 0.25f;
        p.values["mod1_attack"] = 0.005f;
        p.values["mod1_decay"] = 0.08f;
        p.values["mod1_sustain"] = 0.7f;
        p.values["mod1_release"] = 0.2f;
        p.values["mod2_attack"] = 0.005f;
        p.values["mod2_decay"] = 0.1f;
        p.values["mod2_sustain"] = 0.5f;
        p.values["mod2_release"] = 0.15f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Soft Lead";
        p.category = "Lead";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 3.0f;
        p.values["mod2_ratio"] = 2.0f;
        p.values["mod2_index"] = 2.0f;
        p.values["feedback"] = 0.1f;
        p.values["amp_attack"] = 0.05f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 0.4f;
        p.values["mod1_attack"] = 0.05f;
        p.values["mod1_decay"] = 0.2f;
        p.values["mod1_sustain"] = 0.5f;
        p.values["mod1_release"] = 0.3f;
        p.values["mod2_attack"] = 0.05f;
        p.values["mod2_decay"] = 0.15f;
        p.values["mod2_sustain"] = 0.4f;
        p.values["mod2_release"] = 0.25f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // ============ PAD ============
    {
        SynthPreset p;
        p.name = "FM Pad";
        p.category = "Pad";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 2.0f;
        p.values["mod1_index"] = 2.0f;
        p.values["mod2_ratio"] = 3.0f;
        p.values["mod2_index"] = 1.0f;
        p.values["feedback"] = 0.1f;
        p.values["amp_attack"] = 0.5f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 1.5f;
        p.values["mod1_attack"] = 0.6f;
        p.values["mod1_decay"] = 0.4f;
        p.values["mod1_sustain"] = 0.5f;
        p.values["mod1_release"] = 1.2f;
        p.values["mod2_attack"] = 0.7f;
        p.values["mod2_decay"] = 0.3f;
        p.values["mod2_sustain"] = 0.4f;
        p.values["mod2_release"] = 1.0f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Warm FM Pad";
        p.category = "Pad";
        p.enumValues["algorithm"] = 1;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 1.5f;
        p.values["mod2_ratio"] = 2.0f;
        p.values["mod2_index"] = 1.0f;
        p.values["feedback"] = 0.15f;
        p.values["amp_attack"] = 0.8f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 2.0f;
        p.values["mod1_attack"] = 1.0f;
        p.values["mod1_decay"] = 0.5f;
        p.values["mod1_sustain"] = 0.6f;
        p.values["mod1_release"] = 1.5f;
        p.values["mod2_attack"] = 0.8f;
        p.values["mod2_decay"] = 0.4f;
        p.values["mod2_sustain"] = 0.5f;
        p.values["mod2_release"] = 1.2f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Glass Pad";
        p.category = "Pad";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 4.0f;
        p.values["mod1_index"] = 3.0f;
        p.values["mod2_ratio"] = 7.0f;
        p.values["mod2_index"] = 1.5f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.6f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.6f;
        p.values["amp_release"] = 2.5f;
        p.values["mod1_attack"] = 0.7f;
        p.values["mod1_decay"] = 0.6f;
        p.values["mod1_sustain"] = 0.4f;
        p.values["mod1_release"] = 2.0f;
        p.values["mod2_attack"] = 0.5f;
        p.values["mod2_decay"] = 0.4f;
        p.values["mod2_sustain"] = 0.3f;
        p.values["mod2_release"] = 1.5f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // ============ PLUCK ============
    {
        SynthPreset p;
        p.name = "FM Pluck";
        p.category = "Pluck";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 5.0f;
        p.values["mod1_index"] = 6.0f;
        p.values["mod2_ratio"] = 1.0f;
        p.values["mod2_index"] = 2.0f;
        p.values["feedback"] = 0.1f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 0.3f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 0.3f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 0.2f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.2f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Harp";
        p.category = "Pluck";
        p.enumValues["algorithm"] = 1;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 3.0f;
        p.values["mod1_index"] = 4.0f;
        p.values["mod2_ratio"] = 5.0f;
        p.values["mod2_index"] = 2.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 1.5f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 0.8f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 1.0f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 0.5f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.5f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.3f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Kalimba";
        p.category = "Pluck";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 6.0f;
        p.values["mod1_index"] = 5.0f;
        p.values["mod2_ratio"] = 2.0f;
        p.values["mod2_index"] = 1.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 1.0f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 0.5f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 0.6f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 0.3f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.4f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.2f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // ============ BRASS/WIND ============
    {
        SynthPreset p;
        p.name = "FM Brass";
        p.category = "Brass";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 20.0f;
        p.values["mod2_ratio"] = 3.0f;
        p.values["mod2_index"] = 8.0f;
        p.values["feedback"] = 0.4f;
        p.values["amp_attack"] = 0.08f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 0.2f;
        p.values["mod1_attack"] = 0.06f;
        p.values["mod1_decay"] = 0.15f;
        p.values["mod1_sustain"] = 0.5f;
        p.values["mod1_release"] = 0.15f;
        p.values["mod2_attack"] = 0.05f;
        p.values["mod2_decay"] = 0.1f;
        p.values["mod2_sustain"] = 0.3f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "FM Organ";
        p.category = "Organ";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 2.0f;
        p.values["mod1_index"] = 3.0f;
        p.values["mod2_ratio"] = 4.0f;
        p.values["mod2_index"] = 1.0f;
        p.values["feedback"] = 0.5f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_decay"] = 0.1f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 0.1f;
        p.values["mod1_attack"] = 0.01f;
        p.values["mod1_decay"] = 0.1f;
        p.values["mod1_sustain"] = 0.7f;
        p.values["mod1_release"] = 0.1f;
        p.values["mod2_attack"] = 0.01f;
        p.values["mod2_decay"] = 0.1f;
        p.values["mod2_sustain"] = 0.5f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Marimba";
        p.category = "Mallet";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 10.0f;
        p.values["mod1_index"] = 5.0f;
        p.values["mod2_ratio"] = 1.0f;
        p.values["mod2_index"] = 0.0f;
        p.values["feedback"] = 0.0f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 1.0f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 0.5f;
        p.values["mod1_attack"] = 0.001f;
        p.values["mod1_decay"] = 0.5f;
        p.values["mod1_sustain"] = 0.0f;
        p.values["mod1_release"] = 0.3f;
        p.values["mod2_attack"] = 0.001f;
        p.values["mod2_decay"] = 0.3f;
        p.values["mod2_sustain"] = 0.0f;
        p.values["mod2_release"] = 0.2f;
        p.values["volume"] = 0.7f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Harmonica";
        p.category = "Wind";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 10.0f;
        p.values["mod2_ratio"] = 2.0f;
        p.values["mod2_index"] = 3.0f;
        p.values["feedback"] = 0.6f;
        p.values["amp_attack"] = 0.05f;
        p.values["amp_decay"] = 0.1f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 0.1f;
        p.values["mod1_attack"] = 0.04f;
        p.values["mod1_decay"] = 0.1f;
        p.values["mod1_sustain"] = 0.6f;
        p.values["mod1_release"] = 0.1f;
        p.values["mod2_attack"] = 0.03f;
        p.values["mod2_decay"] = 0.1f;
        p.values["mod2_sustain"] = 0.4f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    {
        SynthPreset p;
        p.name = "Flute";
        p.category = "Wind";
        p.enumValues["algorithm"] = 0;
        p.values["carrier_ratio"] = 1.0f;
        p.values["mod1_ratio"] = 1.0f;
        p.values["mod1_index"] = 2.0f;
        p.values["mod2_ratio"] = 3.0f;
        p.values["mod2_index"] = 1.0f;
        p.values["feedback"] = 0.3f;
        p.values["amp_attack"] = 0.1f;
        p.values["amp_decay"] = 0.15f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 0.2f;
        p.values["mod1_attack"] = 0.1f;
        p.values["mod1_decay"] = 0.15f;
        p.values["mod1_sustain"] = 0.6f;
        p.values["mod1_release"] = 0.15f;
        p.values["mod2_attack"] = 0.08f;
        p.values["mod2_decay"] = 0.1f;
        p.values["mod2_sustain"] = 0.5f;
        p.values["mod2_release"] = 0.1f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    return presets;
}
