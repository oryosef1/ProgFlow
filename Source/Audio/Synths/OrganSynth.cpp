#include "OrganSynth.h"
#include <cmath>

//==============================================================================
// OrganSynthVoice Implementation
//==============================================================================

float OrganSynthVoice::DrawbarOscillator::generate(double baseFrequency, double sr)
{
    if (level <= 0.0f)
        return 0.0f;

    // Generate sine wave
    float sample = std::sin(phase * juce::MathConstants<double>::twoPi);

    // Advance phase
    double frequency = baseFrequency * ratio;
    double phaseIncrement = frequency / sr;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * level;
}

OrganSynthVoice::OrganSynthVoice()
{
    // Initialize drawbar oscillators with their harmonic ratios
    for (int i = 0; i < NUM_DRAWBARS; ++i)
    {
        drawbars[i].ratio = DRAWBAR_RATIOS[i];
        drawbars[i].level = 0.0f;
    }

    // Default drawbar settings (8' and 4' out)
    drawbars[2].level = 1.0f; // 8' full
    drawbars[3].level = 0.75f; // 4' at 6/8
    drawbars[0].level = 0.5f; // 16' at 4/8

    // Organ has very fast attack and release
    ampEnvParams.attack = 0.005f;
    ampEnvParams.decay = 0.01f;
    ampEnvParams.sustain = 1.0f;
    ampEnvParams.release = 0.05f;
    ampEnvelope.setParameters(ampEnvParams);

    percEnvelope.setParameters(percEnvParams);
    clickEnvelope.setParameters(clickEnvParams);
}

void OrganSynthVoice::prepareToPlay(double sr, int blockSize)
{
    SynthVoice::prepareToPlay(sr, blockSize);

    percEnvelope.setSampleRate(sr);
    clickEnvelope.setSampleRate(sr);

    // Setup click filter (low-pass at 2000Hz)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    clickFilter.prepare(spec);
    clickFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, 2000.0);
}

void OrganSynthVoice::reset()
{
    SynthVoice::reset();

    for (auto& drawbar : drawbars)
        drawbar.reset();

    percPhase = 0.0;
    percEnvelope.reset();
    clickEnvelope.reset();
    clickFilter.reset();
}

void OrganSynthVoice::onNoteStart()
{
    // Reset all oscillator phases for consistent attack
    for (auto& drawbar : drawbars)
        drawbar.phase = 0.0;

    percPhase = 0.0;
    percEnvelope.noteOn();
    clickEnvelope.noteOn();
}

void OrganSynthVoice::onNoteStop()
{
    // Percussion and click don't have release - only amp envelope does
}

float OrganSynthVoice::generatePercussion(double baseFrequency)
{
    if (percAmount <= 0.0f)
        return 0.0f;

    // Calculate percussion frequency (2nd or 3rd harmonic)
    float harmonic = (percHarmonic == PercussionHarmonic::Third) ? 3.0f : 2.0f;
    double percFreq = baseFrequency * harmonic;

    // Generate sine wave
    float sample = std::sin(percPhase * juce::MathConstants<double>::twoPi);

    // Advance phase
    double phaseIncrement = percFreq / sampleRate;
    percPhase += phaseIncrement;
    if (percPhase >= 1.0)
        percPhase -= 1.0;

    // Apply envelope
    float envelope = percEnvelope.getNextSample();

    return sample * envelope * percAmount;
}

float OrganSynthVoice::generateKeyClick()
{
    if (keyClickAmount <= 0.0f)
        return 0.0f;

    // Generate white noise
    float noise = random.nextFloat() * 2.0f - 1.0f;

    // Filter it
    float filtered = clickFilter.processSample(noise);

    // Apply envelope
    float envelope = clickEnvelope.getNextSample();

    return filtered * envelope * keyClickAmount * 0.1f;
}

void OrganSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
                                      int startSample, int numSamples)
{
    if (!isActive())
        return;

    auto* outputL = buffer.getWritePointer(0, startSample);
    auto* outputR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // Get current frequency (no portamento needed for organ, but included for completeness)
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

        // Generate all drawbar oscillators (additive synthesis)
        float mixed = 0.0f;
        for (int d = 0; d < NUM_DRAWBARS; ++d)
        {
            mixed += drawbars[d].generate(baseFreq, sampleRate);
        }

        // Add percussion
        mixed += generatePercussion(baseFreq);

        // Add key click
        mixed += generateKeyClick();

        // Apply amp envelope, velocity, and master volume
        float output = mixed * ampEnv * velocity * masterVolume * 0.15f;

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

void OrganSynthVoice::setDrawbarLevel(int drawbarIndex, float level)
{
    if (drawbarIndex >= 0 && drawbarIndex < NUM_DRAWBARS)
    {
        drawbars[drawbarIndex].level = juce::jlimit(0.0f, 1.0f, level);
    }
}

void OrganSynthVoice::setPercussionAmount(float amount)
{
    percAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void OrganSynthVoice::setPercussionHarmonic(PercussionHarmonic harmonic)
{
    percHarmonic = harmonic;
}

void OrganSynthVoice::setPercussionDecay(PercussionDecay decay)
{
    if (decay == PercussionDecay::Fast)
    {
        percEnvParams.decay = 0.2f;
    }
    else
    {
        percEnvParams.decay = 0.5f;
    }
    percEnvelope.setParameters(percEnvParams);
}

void OrganSynthVoice::setKeyClickAmount(float amount)
{
    keyClickAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void OrganSynthVoice::setMasterVolume(float volume)
{
    masterVolume = juce::jlimit(0.0f, 1.0f, volume);
}

//==============================================================================
// OrganSynth Implementation
//==============================================================================

OrganSynth::OrganSynth()
{
    initializeParameters();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<OrganSynthVoice>();
    }
}

OrganSynth::~OrganSynth()
{
    killAllNotes();
}

float OrganSynth::drawbarToGain(int drawbarValue)
{
    // Exponential mapping for authentic drawbar feel (0-8 range)
    drawbarValue = juce::jlimit(0, 8, drawbarValue);
    return std::pow(static_cast<float>(drawbarValue) / 8.0f, 2.0f);
}

void OrganSynth::initializeParameters()
{
    // 9 drawbars (values 0-8, like real Hammond)
    const char* drawbarLabels[9] = {"16'", "5⅓'", "8'", "4'", "2⅔'", "2'", "1⅗'", "1⅓'", "1'"};

    for (int i = 0; i < 9; ++i)
    {
        juce::String paramId = "drawbar_" + juce::String(i);
        juce::String paramName = "Drawbar " + juce::String(drawbarLabels[i]);

        // Default values: 8' and 4' out, 16' at 4
        float defaultValue = (i == 2) ? 8.0f : (i == 3 ? 6.0f : (i == 0 ? 4.0f : 0.0f));
        addParameter(paramId, paramName, defaultValue, 0.0f, 8.0f, 1.0f);
    }

    // Percussion
    addEnumParameter("percussion", "Percussion", {"Off", "Soft", "Normal"}, 0);
    addEnumParameter("percussion_decay", "Percussion Decay", {"Fast", "Slow"}, 0);
    addEnumParameter("percussion_harmonic", "Percussion Harmonic", {"Second", "Third"}, 1);

    // Key click
    addParameter("key_click", "Key Click", 0.3f, 0.0f, 1.0f);

    // Rotary speaker
    addEnumParameter("rotary_speed", "Rotary Speed", {"Off", "Slow", "Fast"}, 1);
    addParameter("rotary_depth", "Rotary Depth", 0.5f, 0.0f, 1.0f);

    // Drive
    addParameter("drive", "Drive", 0.1f, 0.0f, 1.0f);

    // Master volume
    addParameter("volume", "Volume", 0.6f, 0.0f, 1.0f);
}

void OrganSynth::setupDistortion()
{
    // Create soft clipping curve
    auto waveshaperFunction = [](float x)
    {
        // Soft clip with tanh
        return std::tanh(x);
    };

    distortion.functionToUse = waveshaperFunction;
}

void OrganSynth::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 2;

    // Prepare rotary speaker (chorus effect simulating Leslie)
    rotarySpeaker.prepare(spec);
    rotarySpeaker.setRate(1.5f);
    rotarySpeaker.setDepth(0.5f);
    rotarySpeaker.setFeedback(0.1f);
    rotarySpeaker.setMix(0.4f);
    rotarySpeaker.setCentreDelay(7.0f);

    // Prepare distortion
    distortion.prepare(spec);
    setupDistortion();

    // Prepare voices
    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    updateVoiceParameters();
}

void OrganSynth::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }

    rotarySpeaker.reset();
    distortion.reset();
}

void OrganSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // Apply rotary speaker effect
    if (rotarySpeed != RotarySpeed::Off)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        rotarySpeaker.process(context);
    }

    // Apply distortion/drive
    if (driveAmount > 0.01f)
    {
        // Apply gain before distortion based on drive amount
        buffer.applyGain(1.0f + driveAmount * 3.0f);

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        distortion.process(context);

        // Apply makeup gain and wet/dry mix
        float wetAmount = driveAmount * 0.5f;
        buffer.applyGain(wetAmount);
    }
}

//==============================================================================
// Voice management

OrganSynthVoice* OrganSynth::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

OrganSynthVoice* OrganSynth::findVoiceToSteal()
{
    // Find oldest voice
    OrganSynthVoice* oldest = nullptr;
    float oldestAge = 0.0f;

    for (auto& voice : voices)
    {
        if (voice->getAge() > oldestAge)
        {
            oldestAge = voice->getAge();
            oldest = voice.get();
        }
    }

    return oldest;
}

OrganSynthVoice* OrganSynth::findVoicePlayingNote(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice->isActive() && voice->getCurrentNote() == midiNote)
            return voice.get();
    }
    return nullptr;
}

void OrganSynth::noteOn(int midiNote, float velocity, int /* sampleOffset */)
{
    OrganSynthVoice* voice = findFreeVoice();
    if (!voice)
        voice = findVoiceToSteal();

    if (voice)
    {
        voice->startNote(midiNote, velocity, false);
    }

    activeNotes.insert(midiNote);
}

void OrganSynth::noteOff(int midiNote, int /* sampleOffset */)
{
    // Release voice playing this note
    OrganSynthVoice* voice = findVoicePlayingNote(midiNote);
    if (voice)
    {
        voice->stopNote(true);
    }

    activeNotes.erase(midiNote);
}

void OrganSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void OrganSynth::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Parameter updates

void OrganSynth::updateVoiceParameters()
{
    for (auto& voice : voices)
    {
        // Update drawbars
        for (int i = 0; i < 9; ++i)
        {
            juce::String paramId = "drawbar_" + juce::String(i);
            int drawbarValue = static_cast<int>(getParameter(paramId));
            float gain = drawbarToGain(drawbarValue);
            voice->setDrawbarLevel(i, gain);
        }

        // Percussion
        int percType = getParameterEnum("percussion");
        if (percType == 0) // Off
        {
            voice->setPercussionAmount(0.0f);
        }
        else if (percType == 1) // Soft
        {
            voice->setPercussionAmount(0.15f);
        }
        else // Normal
        {
            voice->setPercussionAmount(0.3f);
        }

        int percHarmonic = getParameterEnum("percussion_harmonic");
        voice->setPercussionHarmonic(percHarmonic == 0 ? PercussionHarmonic::Second : PercussionHarmonic::Third);

        int percDecay = getParameterEnum("percussion_decay");
        voice->setPercussionDecay(percDecay == 0 ? PercussionDecay::Fast : PercussionDecay::Slow);

        // Key click
        voice->setKeyClickAmount(getParameter("key_click"));

        // Master volume
        voice->setMasterVolume(getParameter("volume"));
    }

    // Update rotary speaker
    int rotarySpeedIdx = getParameterEnum("rotary_speed");
    rotarySpeed = static_cast<RotarySpeed>(rotarySpeedIdx);

    if (rotarySpeed == RotarySpeed::Off)
    {
        rotarySpeaker.setMix(0.0f);
    }
    else if (rotarySpeed == RotarySpeed::Slow)
    {
        rotarySpeaker.setRate(1.5f);
        rotarySpeaker.setMix(0.4f);
    }
    else // Fast
    {
        rotarySpeaker.setRate(6.5f);
        rotarySpeaker.setMix(0.5f);
    }

    rotaryDepth = getParameter("rotary_depth");
    rotarySpeaker.setDepth(rotaryDepth);

    // Update drive
    driveAmount = getParameter("drive");
}

void OrganSynth::onParameterChanged(const juce::String& name, float value)
{
    updateVoiceParameters();
}

void OrganSynth::onParameterEnumChanged(const juce::String& name, int index)
{
    updateVoiceParameters();
}

//==============================================================================
// Presets

std::vector<SynthPreset> OrganSynth::getPresets() const
{
    std::vector<SynthPreset> presets;

    // Rock Organ
    {
        SynthPreset p;
        p.name = "Rock Organ";
        p.category = "Rock";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 6.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 0.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.4f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.5f;
        p.values["drive"] = 0.3f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // Jazz Organ
    {
        SynthPreset p;
        p.name = "Jazz Organ";
        p.category = "Jazz";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 6.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 0.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 4.0f;
        p.enumValues["percussion"] = 1; // Soft
        p.enumValues["percussion_decay"] = 1; // Slow
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.3f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.6f;
        p.values["drive"] = 0.1f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Church Organ
    {
        SynthPreset p;
        p.name = "Church Organ";
        p.category = "Classical";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 4.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 6.0f;
        p.values["drawbar_5"] = 8.0f;
        p.values["drawbar_6"] = 4.0f;
        p.values["drawbar_7"] = 4.0f;
        p.values["drawbar_8"] = 4.0f;
        p.enumValues["percussion"] = 0; // Off
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 0; // Second
        p.values["key_click"] = 0.0f;
        p.enumValues["rotary_speed"] = 0; // Off
        p.values["rotary_depth"] = 0.0f;
        p.values["drive"] = 0.0f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Gospel Organ
    {
        SynthPreset p;
        p.name = "Gospel Organ";
        p.category = "Gospel";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 4.0f;
        p.values["drawbar_5"] = 6.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 0; // Second
        p.values["key_click"] = 0.5f;
        p.enumValues["rotary_speed"] = 2; // Fast
        p.values["rotary_depth"] = 0.7f;
        p.values["drive"] = 0.4f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // Ballad Organ
    {
        SynthPreset p;
        p.name = "Ballad Organ";
        p.category = "Ballad";
        p.values["drawbar_0"] = 0.0f;
        p.values["drawbar_1"] = 0.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 6.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 4.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 2.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 1; // Soft
        p.enumValues["percussion_decay"] = 1; // Slow
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.2f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.4f;
        p.values["drive"] = 0.05f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Full Organ
    {
        SynthPreset p;
        p.name = "Full Organ";
        p.category = "Full";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 8.0f;
        p.values["drawbar_5"] = 8.0f;
        p.values["drawbar_6"] = 8.0f;
        p.values["drawbar_7"] = 8.0f;
        p.values["drawbar_8"] = 8.0f;
        p.enumValues["percussion"] = 0; // Off
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.3f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.5f;
        p.values["drive"] = 0.2f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Theatre Organ
    {
        SynthPreset p;
        p.name = "Theatre Organ";
        p.category = "Theatre";
        p.values["drawbar_0"] = 6.0f;
        p.values["drawbar_1"] = 6.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 7.0f;
        p.values["drawbar_4"] = 5.0f;
        p.values["drawbar_5"] = 6.0f;
        p.values["drawbar_6"] = 3.0f;
        p.values["drawbar_7"] = 3.0f;
        p.values["drawbar_8"] = 2.0f;
        p.enumValues["percussion"] = 0; // Off
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.1f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.3f;
        p.values["drive"] = 0.0f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Blues Organ
    {
        SynthPreset p;
        p.name = "Blues Organ";
        p.category = "Blues";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 6.0f;
        p.values["drawbar_3"] = 4.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 2.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.6f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.6f;
        p.values["drive"] = 0.5f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // Bright Organ
    {
        SynthPreset p;
        p.name = "Bright Organ";
        p.category = "Bright";
        p.values["drawbar_0"] = 4.0f;
        p.values["drawbar_1"] = 4.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 6.0f;
        p.values["drawbar_5"] = 8.0f;
        p.values["drawbar_6"] = 4.0f;
        p.values["drawbar_7"] = 4.0f;
        p.values["drawbar_8"] = 6.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.3f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.4f;
        p.values["drive"] = 0.15f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Soft Organ
    {
        SynthPreset p;
        p.name = "Soft Organ";
        p.category = "Soft";
        p.values["drawbar_0"] = 4.0f;
        p.values["drawbar_1"] = 2.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 4.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 2.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 0; // Off
        p.enumValues["percussion_decay"] = 1; // Slow
        p.enumValues["percussion_harmonic"] = 0; // Second
        p.values["key_click"] = 0.1f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.3f;
        p.values["drive"] = 0.0f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Funky Organ
    {
        SynthPreset p;
        p.name = "Funky Organ";
        p.category = "Funk";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 6.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 6.0f;
        p.values["drawbar_4"] = 2.0f;
        p.values["drawbar_5"] = 4.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.5f;
        p.enumValues["rotary_speed"] = 2; // Fast
        p.values["rotary_depth"] = 0.5f;
        p.values["drive"] = 0.35f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    // Classic Combo
    {
        SynthPreset p;
        p.name = "Classic Combo";
        p.category = "Classic";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 0.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 0.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 1; // Soft
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.35f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.5f;
        p.values["drive"] = 0.2f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    // Reggae Organ
    {
        SynthPreset p;
        p.name = "Reggae Organ";
        p.category = "Reggae";
        p.values["drawbar_0"] = 0.0f;
        p.values["drawbar_1"] = 0.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 0.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 0.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 0; // Off
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 0; // Second
        p.values["key_click"] = 0.15f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.3f;
        p.values["drive"] = 0.05f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Hammond A
    {
        SynthPreset p;
        p.name = "Hammond A";
        p.category = "Classic";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 6.0f;
        p.values["drawbar_3"] = 5.0f;
        p.values["drawbar_4"] = 4.0f;
        p.values["drawbar_5"] = 3.0f;
        p.values["drawbar_6"] = 2.0f;
        p.values["drawbar_7"] = 1.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.4f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.5f;
        p.values["drive"] = 0.25f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    // Hammond B
    {
        SynthPreset p;
        p.name = "Hammond B";
        p.category = "Classic";
        p.values["drawbar_0"] = 0.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 5.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 3.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 1; // Soft
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 0; // Second
        p.values["key_click"] = 0.3f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.45f;
        p.values["drive"] = 0.2f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Soulful Organ
    {
        SynthPreset p;
        p.name = "Soulful Organ";
        p.category = "Soul";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 6.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 2.0f;
        p.values["drawbar_5"] = 4.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 2.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 1; // Slow
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.4f;
        p.enumValues["rotary_speed"] = 2; // Fast
        p.values["rotary_depth"] = 0.6f;
        p.values["drive"] = 0.35f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    // Prog Organ
    {
        SynthPreset p;
        p.name = "Prog Organ";
        p.category = "Prog";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 4.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 8.0f;
        p.values["drawbar_4"] = 4.0f;
        p.values["drawbar_5"] = 6.0f;
        p.values["drawbar_6"] = 4.0f;
        p.values["drawbar_7"] = 4.0f;
        p.values["drawbar_8"] = 4.0f;
        p.enumValues["percussion"] = 0; // Off
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.2f;
        p.enumValues["rotary_speed"] = 2; // Fast
        p.values["rotary_depth"] = 0.7f;
        p.values["drive"] = 0.45f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    // Mellow Organ
    {
        SynthPreset p;
        p.name = "Mellow Organ";
        p.category = "Mellow";
        p.values["drawbar_0"] = 4.0f;
        p.values["drawbar_1"] = 4.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 6.0f;
        p.values["drawbar_4"] = 2.0f;
        p.values["drawbar_5"] = 4.0f;
        p.values["drawbar_6"] = 1.0f;
        p.values["drawbar_7"] = 2.0f;
        p.values["drawbar_8"] = 1.0f;
        p.enumValues["percussion"] = 1; // Soft
        p.enumValues["percussion_decay"] = 1; // Slow
        p.enumValues["percussion_harmonic"] = 0; // Second
        p.values["key_click"] = 0.15f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.35f;
        p.values["drive"] = 0.05f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // Dirty Organ
    {
        SynthPreset p;
        p.name = "Dirty Organ";
        p.category = "Dirty";
        p.values["drawbar_0"] = 8.0f;
        p.values["drawbar_1"] = 8.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 6.0f;
        p.values["drawbar_4"] = 4.0f;
        p.values["drawbar_5"] = 4.0f;
        p.values["drawbar_6"] = 2.0f;
        p.values["drawbar_7"] = 2.0f;
        p.values["drawbar_8"] = 2.0f;
        p.enumValues["percussion"] = 2; // Normal
        p.enumValues["percussion_decay"] = 0; // Fast
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.6f;
        p.enumValues["rotary_speed"] = 2; // Fast
        p.values["rotary_depth"] = 0.7f;
        p.values["drive"] = 0.7f;
        p.values["volume"] = 0.55f;
        presets.push_back(p);
    }

    // Smooth Organ
    {
        SynthPreset p;
        p.name = "Smooth Organ";
        p.category = "Smooth";
        p.values["drawbar_0"] = 6.0f;
        p.values["drawbar_1"] = 4.0f;
        p.values["drawbar_2"] = 8.0f;
        p.values["drawbar_3"] = 6.0f;
        p.values["drawbar_4"] = 0.0f;
        p.values["drawbar_5"] = 4.0f;
        p.values["drawbar_6"] = 0.0f;
        p.values["drawbar_7"] = 2.0f;
        p.values["drawbar_8"] = 0.0f;
        p.enumValues["percussion"] = 1; // Soft
        p.enumValues["percussion_decay"] = 1; // Slow
        p.enumValues["percussion_harmonic"] = 1; // Third
        p.values["key_click"] = 0.2f;
        p.enumValues["rotary_speed"] = 1; // Slow
        p.values["rotary_depth"] = 0.4f;
        p.values["drive"] = 0.1f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    return presets;
}
