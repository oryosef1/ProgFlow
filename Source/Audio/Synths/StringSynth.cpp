#include "StringSynth.h"
#include <cmath>

//==============================================================================
// StringSynthVoice Implementation
//==============================================================================

StringSynthVoice::StringSynthVoice()
{
    filterEnvelope.setParameters(filterEnvParams);
}

void StringSynthVoice::prepareToPlay(double sr, int blockSize)
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

void StringSynthVoice::reset()
{
    SynthVoice::reset();

    for (auto& osc : oscillators)
        osc.reset();

    filter.reset();
    filterEnvelope.reset();
}

void StringSynthVoice::onNoteStart()
{
    // Build oscillator array for this note's frequency
    float baseFreq = SynthBase::midiToFrequency(currentNote);
    rebuildOscillators(baseFreq);

    filterEnvelope.noteOn();
}

void StringSynthVoice::onNoteStop()
{
    filterEnvelope.noteOff();
}

void StringSynthVoice::rebuildOscillators(float baseFrequency)
{
    oscillators.clear();

    // Define orchestral sections with octave offsets
    struct Section
    {
        float level;
        int octave;
    };

    Section sections[] = {
        { violinsLevel, 1 },   // Violins (1 octave up)
        { violasLevel, 0 },    // Violas (same octave)
        { cellosLevel, -1 },   // Cellos (1 octave down)
        { bassesLevel, -2 }    // Basses (2 octaves down)
    };

    // Create ensemble oscillators for each active section
    for (const auto& section : sections)
    {
        if (section.level > 0.0f)
        {
            for (int i = 0; i < ensembleVoices; ++i)
            {
                Oscillator osc;
                osc.level = section.level;
                osc.octave = section.octave;

                // Spread detuning symmetrically around center
                float spread = ((float)i - (float)ensembleVoices / 2.0f) / (float)ensembleVoices;
                osc.detuneCents = spread * ensembleSpread;

                oscillators.push_back(osc);
            }
        }
    }
}

float StringSynthVoice::Oscillator::generate(double frequency, double sr)
{
    // Apply octave and detune
    double freq = frequency * std::pow(2.0, octave);
    freq *= std::pow(2.0, detuneCents / 1200.0);

    // Generate sawtooth wave
    double t = std::fmod(phase, 1.0);
    float sample = static_cast<float>(2.0 * t - 1.0);

    // Advance phase
    double phaseIncrement = freq / sr;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * level;
}

void StringSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
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

        // Get filter envelope value
        float filterEnv = filterEnvelope.getNextSample();

        // Check if voice should go idle
        if (state == VoiceState::Release && ampEnv < 0.0001f)
        {
            state = VoiceState::Idle;
            currentNote = -1;
            break;
        }

        // Generate and mix all oscillators
        float mixed = 0.0f;
        for (auto& osc : oscillators)
        {
            mixed += osc.generate(baseFreq, sampleRate);
        }

        // Normalize by number of oscillators to prevent clipping
        if (!oscillators.empty())
            mixed *= 0.15f / std::max(1.0f, (float)oscillators.size());

        // Calculate filter cutoff with envelope modulation
        float modulatedCutoff = filterCutoff + (filterEnvAmount * filterEnv);
        modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);

        // Update filter
        filter.setCutoffFrequency(modulatedCutoff);

        // Process through filter
        float filtered = filter.processSample(0, mixed);

        // Apply amp envelope, velocity, and master volume
        float output = filtered * ampEnv * velocity * masterVolume;

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

void StringSynthVoice::setViolinsLevel(float level)
{
    violinsLevel = juce::jlimit(0.0f, 1.0f, level);
}

void StringSynthVoice::setViolasLevel(float level)
{
    violasLevel = juce::jlimit(0.0f, 1.0f, level);
}

void StringSynthVoice::setCellosLevel(float level)
{
    cellosLevel = juce::jlimit(0.0f, 1.0f, level);
}

void StringSynthVoice::setBassesLevel(float level)
{
    bassesLevel = juce::jlimit(0.0f, 1.0f, level);
}

void StringSynthVoice::setEnsembleVoices(int numVoices)
{
    ensembleVoices = juce::jlimit(2, 8, numVoices);
}

void StringSynthVoice::setEnsembleSpread(float cents)
{
    ensembleSpread = juce::jlimit(0.0f, 50.0f, cents);
}

void StringSynthVoice::setFilterCutoff(float frequency)
{
    filterCutoff = juce::jlimit(100.0f, 10000.0f, frequency);
}

void StringSynthVoice::setFilterResonance(float resonance)
{
    filterResonance = juce::jlimit(0.1f, 10.0f, resonance);
    filter.setResonance(juce::jmap(filterResonance, 0.1f, 10.0f, 0.0f, 1.0f));
}

void StringSynthVoice::setFilterEnvAmount(float amount)
{
    filterEnvAmount = juce::jlimit(0.0f, 8000.0f, amount);
}

void StringSynthVoice::setFilterEnvelope(float attack, float decay, float sustain, float release)
{
    filterEnvParams.attack = juce::jmax(0.01f, attack);
    filterEnvParams.decay = juce::jmax(0.01f, decay);
    filterEnvParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    filterEnvParams.release = juce::jmax(0.01f, release);
    filterEnvelope.setParameters(filterEnvParams);
}

void StringSynthVoice::setMasterVolume(float volume)
{
    masterVolume = juce::jlimit(0.0f, 1.0f, volume);
}

//==============================================================================
// StringSynth Implementation
//==============================================================================

StringSynth::StringSynth()
{
    initializeParameters();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<StringSynthVoice>();
    }
}

StringSynth::~StringSynth()
{
    killAllNotes();
}

void StringSynth::initializeParameters()
{
    // Section levels
    addParameter("violins", "Violins", 1.0f, 0.0f, 1.0f, 0.01f);
    addParameter("violas", "Violas", 0.5f, 0.0f, 1.0f, 0.01f);
    addParameter("cellos", "Cellos", 0.3f, 0.0f, 1.0f, 0.01f);
    addParameter("basses", "Basses", 0.0f, 0.0f, 1.0f, 0.01f);

    // Ensemble settings
    addParameter("ensemble_spread", "Ensemble Spread", 15.0f, 0.0f, 50.0f, 1.0f);
    addParameter("ensemble_voices", "Ensemble Voices", 4.0f, 2.0f, 8.0f, 1.0f);

    // Filter
    addParameter("filter_cutoff", "Filter Cutoff", 3000.0f, 100.0f, 10000.0f, 10.0f);
    addParameter("filter_resonance", "Filter Resonance", 1.0f, 0.1f, 10.0f, 0.1f);

    // Filter envelope
    addParameter("filter_env_amount", "Filter Env Amount", 2000.0f, 0.0f, 8000.0f, 100.0f);
    addParameter("filter_attack", "Filter Attack", 0.8f, 0.01f, 4.0f, 0.01f);
    addParameter("filter_decay", "Filter Decay", 0.5f, 0.01f, 2.0f, 0.01f);
    addParameter("filter_sustain", "Filter Sustain", 0.4f, 0.0f, 1.0f, 0.01f);
    addParameter("filter_release", "Filter Release", 1.5f, 0.01f, 8.0f, 0.01f);

    // Amplitude envelope
    addParameter("amp_attack", "Attack", 0.5f, 0.01f, 4.0f, 0.01f);
    addParameter("amp_decay", "Decay", 0.3f, 0.01f, 2.0f, 0.01f);
    addParameter("amp_sustain", "Sustain", 0.8f, 0.0f, 1.0f, 0.01f);
    addParameter("amp_release", "Release", 1.5f, 0.01f, 8.0f, 0.01f);

    // Chorus
    addParameter("chorus_rate", "Chorus Rate", 1.5f, 0.1f, 8.0f, 0.1f);
    addParameter("chorus_depth", "Chorus Depth", 0.7f, 0.0f, 1.0f, 0.01f);
    addParameter("chorus_wet", "Chorus Mix", 0.5f, 0.0f, 1.0f, 0.01f);

    // Phaser
    addParameter("phaser_wet", "Phaser Mix", 0.3f, 0.0f, 1.0f, 0.01f);

    // Master volume
    addParameter("volume", "Volume", 0.5f, 0.0f, 1.0f, 0.01f);
}

void StringSynth::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    // Prepare voices
    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    // Prepare effects
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 2;

    chorus.prepare(spec);
    chorus.reset();
    chorus.setRate(1.5f);
    chorus.setDepth(0.7f);
    chorus.setCentreDelay(3.5f);
    chorus.setFeedback(0.0f);
    chorus.setMix(0.5f);

    phaser.prepare(spec);
    phaser.reset();
    phaser.setRate(0.5f);
    phaser.setDepth(0.5f);
    phaser.setCentreFrequency(350.0f);
    phaser.setFeedback(0.0f);
    phaser.setMix(0.3f);

    updateVoiceParameters();
}

void StringSynth::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }

    chorus.reset();
    phaser.reset();
}

void StringSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // Apply effects (chorus → phaser)
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    chorus.process(context);
    phaser.process(context);
}

//==============================================================================
// Voice management

StringSynthVoice* StringSynth::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

StringSynthVoice* StringSynth::findVoiceToSteal()
{
    // Find oldest voice in release, or oldest overall
    StringSynthVoice* oldestRelease = nullptr;
    StringSynthVoice* oldest = nullptr;
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

StringSynthVoice* StringSynth::findVoicePlayingNote(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice->isActive() && voice->getCurrentNote() == midiNote)
            return voice.get();
    }
    return nullptr;
}

void StringSynth::noteOn(int midiNote, float velocity, int /* sampleOffset */)
{
    StringSynthVoice* voice = findFreeVoice();
    if (!voice)
        voice = findVoiceToSteal();

    if (voice)
    {
        voice->startNote(midiNote, velocity);
    }

    activeNotes.insert(midiNote);
}

void StringSynth::noteOff(int midiNote, int /* sampleOffset */)
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

void StringSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void StringSynth::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Parameter updates

void StringSynth::updateVoiceParameters()
{
    for (auto& voice : voices)
    {
        // Section levels
        voice->setViolinsLevel(getParameter("violins"));
        voice->setViolasLevel(getParameter("violas"));
        voice->setCellosLevel(getParameter("cellos"));
        voice->setBassesLevel(getParameter("basses"));

        // Ensemble
        voice->setEnsembleVoices(static_cast<int>(getParameter("ensemble_voices")));
        voice->setEnsembleSpread(getParameter("ensemble_spread"));

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

        // Amp envelope
        voice->setAmpEnvelope(
            getParameter("amp_attack"),
            getParameter("amp_decay"),
            getParameter("amp_sustain"),
            getParameter("amp_release")
        );

        // Master volume
        voice->setMasterVolume(getParameter("volume"));
    }
}

void StringSynth::onParameterChanged(const juce::String& name, float value)
{
    // Update effects
    if (name == "chorus_rate")
    {
        chorus.setRate(value);
    }
    else if (name == "chorus_depth")
    {
        chorus.setDepth(value);
    }
    else if (name == "chorus_wet")
    {
        chorus.setMix(value);
    }
    else if (name == "phaser_wet")
    {
        phaser.setMix(value);
    }

    // Update all voices with the new parameter
    updateVoiceParameters();
}

//==============================================================================
// Presets

std::vector<SynthPreset> StringSynth::getPresets() const
{
    std::vector<SynthPreset> presets;

    // Full Orchestra
    {
        SynthPreset p;
        p.name = "Full Orchestra";
        p.category = "Orchestra";
        p.values["violins"] = 1.0f;
        p.values["violas"] = 0.7f;
        p.values["cellos"] = 0.5f;
        p.values["basses"] = 0.3f;
        p.values["ensemble_spread"] = 15.0f;
        p.values["ensemble_voices"] = 4.0f;
        p.values["filter_cutoff"] = 4000.0f;
        p.values["filter_resonance"] = 1.0f;
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.8f;
        p.values["filter_decay"] = 0.5f;
        p.values["filter_sustain"] = 0.4f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.5f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 1.5f;
        p.values["chorus_rate"] = 1.5f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.4f;
        p.values["phaser_wet"] = 0.2f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Violins
    {
        SynthPreset p;
        p.name = "Violins";
        p.category = "Strings";
        p.values["violins"] = 1.0f;
        p.values["violas"] = 0.0f;
        p.values["cellos"] = 0.0f;
        p.values["basses"] = 0.0f;
        p.values["ensemble_spread"] = 12.0f;
        p.values["ensemble_voices"] = 6.0f;
        p.values["filter_cutoff"] = 5000.0f;
        p.values["filter_resonance"] = 0.5f;
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.4f;
        p.values["filter_decay"] = 0.2f;
        p.values["filter_sustain"] = 0.85f;
        p.values["filter_release"] = 1.2f;
        p.values["amp_attack"] = 0.4f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 1.2f;
        p.values["chorus_rate"] = 2.0f;
        p.values["chorus_depth"] = 0.4f;
        p.values["chorus_wet"] = 0.5f;
        p.values["phaser_wet"] = 0.2f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Cellos
    {
        SynthPreset p;
        p.name = "Cellos";
        p.category = "Strings";
        p.values["violins"] = 0.0f;
        p.values["violas"] = 0.3f;
        p.values["cellos"] = 1.0f;
        p.values["basses"] = 0.4f;
        p.values["ensemble_spread"] = 10.0f;
        p.values["ensemble_voices"] = 4.0f;
        p.values["filter_cutoff"] = 2500.0f;
        p.values["filter_resonance"] = 1.0f;
        p.values["filter_env_amount"] = 1500.0f;
        p.values["filter_attack"] = 0.6f;
        p.values["filter_decay"] = 0.3f;
        p.values["filter_sustain"] = 0.75f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.6f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 2.0f;
        p.values["chorus_rate"] = 1.0f;
        p.values["chorus_depth"] = 0.6f;
        p.values["chorus_wet"] = 0.4f;
        p.values["phaser_wet"] = 0.15f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // Lush Strings
    {
        SynthPreset p;
        p.name = "Lush Strings";
        p.category = "Pad";
        p.values["violins"] = 0.8f;
        p.values["violas"] = 0.8f;
        p.values["cellos"] = 0.6f;
        p.values["basses"] = 0.2f;
        p.values["ensemble_spread"] = 25.0f;
        p.values["ensemble_voices"] = 6.0f;
        p.values["filter_cutoff"] = 3000.0f;
        p.values["filter_resonance"] = 2.0f;
        p.values["filter_env_amount"] = 2500.0f;
        p.values["filter_attack"] = 1.0f;
        p.values["filter_decay"] = 0.5f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 3.0f;
        p.values["amp_attack"] = 1.0f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 3.0f;
        p.values["chorus_rate"] = 0.8f;
        p.values["chorus_depth"] = 0.8f;
        p.values["chorus_wet"] = 0.6f;
        p.values["phaser_wet"] = 0.4f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Chamber Strings
    {
        SynthPreset p;
        p.name = "Chamber Strings";
        p.category = "Chamber";
        p.values["violins"] = 0.6f;
        p.values["violas"] = 0.5f;
        p.values["cellos"] = 0.4f;
        p.values["basses"] = 0.0f;
        p.values["ensemble_spread"] = 8.0f;
        p.values["ensemble_voices"] = 3.0f;
        p.values["filter_cutoff"] = 4500.0f;
        p.values["filter_resonance"] = 0.8f;
        p.values["filter_env_amount"] = 1500.0f;
        p.values["filter_attack"] = 0.35f;
        p.values["filter_decay"] = 0.25f;
        p.values["filter_sustain"] = 0.8f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.35f;
        p.values["amp_decay"] = 0.25f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 1.0f;
        p.values["chorus_rate"] = 1.8f;
        p.values["chorus_depth"] = 0.3f;
        p.values["chorus_wet"] = 0.35f;
        p.values["phaser_wet"] = 0.15f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Synth Strings
    {
        SynthPreset p;
        p.name = "Synth Strings";
        p.category = "Synth";
        p.values["violins"] = 1.0f;
        p.values["violas"] = 0.5f;
        p.values["cellos"] = 0.0f;
        p.values["basses"] = 0.0f;
        p.values["ensemble_spread"] = 30.0f;
        p.values["ensemble_voices"] = 8.0f;
        p.values["filter_cutoff"] = 6000.0f;
        p.values["filter_resonance"] = 3.0f;
        p.values["filter_env_amount"] = 3000.0f;
        p.values["filter_attack"] = 0.2f;
        p.values["filter_decay"] = 0.3f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.2f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 1.0f;
        p.values["chorus_rate"] = 3.0f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.7f;
        p.values["phaser_wet"] = 0.5f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Warm Strings
    {
        SynthPreset p;
        p.name = "Warm Strings";
        p.category = "Warm";
        p.values["violins"] = 0.5f;
        p.values["violas"] = 0.8f;
        p.values["cellos"] = 0.7f;
        p.values["basses"] = 0.5f;
        p.values["ensemble_spread"] = 12.0f;
        p.values["ensemble_voices"] = 4.0f;
        p.values["filter_cutoff"] = 2000.0f;
        p.values["filter_resonance"] = 1.5f;
        p.values["filter_env_amount"] = 1200.0f;
        p.values["filter_attack"] = 0.8f;
        p.values["filter_decay"] = 0.4f;
        p.values["filter_sustain"] = 0.75f;
        p.values["filter_release"] = 2.5f;
        p.values["amp_attack"] = 0.8f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 2.5f;
        p.values["chorus_rate"] = 0.6f;
        p.values["chorus_depth"] = 0.6f;
        p.values["chorus_wet"] = 0.45f;
        p.values["phaser_wet"] = 0.25f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Epic Strings
    {
        SynthPreset p;
        p.name = "Epic Strings";
        p.category = "Epic";
        p.values["violins"] = 1.0f;
        p.values["violas"] = 1.0f;
        p.values["cellos"] = 1.0f;
        p.values["basses"] = 1.0f;
        p.values["ensemble_spread"] = 20.0f;
        p.values["ensemble_voices"] = 6.0f;
        p.values["filter_cutoff"] = 5000.0f;
        p.values["filter_resonance"] = 1.0f;
        p.values["filter_env_amount"] = 2500.0f;
        p.values["filter_attack"] = 0.3f;
        p.values["filter_decay"] = 0.2f;
        p.values["filter_sustain"] = 0.9f;
        p.values["filter_release"] = 1.5f;
        p.values["amp_attack"] = 0.3f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.9f;
        p.values["amp_release"] = 1.5f;
        p.values["chorus_rate"] = 1.2f;
        p.values["chorus_depth"] = 0.4f;
        p.values["chorus_wet"] = 0.5f;
        p.values["phaser_wet"] = 0.2f;
        p.values["volume"] = 0.6f;
        presets.push_back(p);
    }

    // Bright Strings
    {
        SynthPreset p;
        p.name = "Bright Strings";
        p.category = "Bright";
        p.values["violins"] = 1.0f;
        p.values["violas"] = 0.4f;
        p.values["cellos"] = 0.2f;
        p.values["basses"] = 0.0f;
        p.values["ensemble_spread"] = 15.0f;
        p.values["ensemble_voices"] = 5.0f;
        p.values["filter_cutoff"] = 7000.0f;
        p.values["filter_resonance"] = 1.0f;
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.25f;
        p.values["filter_decay"] = 0.2f;
        p.values["filter_sustain"] = 0.85f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.3f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 1.0f;
        p.values["chorus_rate"] = 2.0f;
        p.values["chorus_depth"] = 0.4f;
        p.values["chorus_wet"] = 0.45f;
        p.values["phaser_wet"] = 0.2f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // Dark Strings
    {
        SynthPreset p;
        p.name = "Dark Strings";
        p.category = "Dark";
        p.values["violins"] = 0.3f;
        p.values["violas"] = 0.6f;
        p.values["cellos"] = 0.9f;
        p.values["basses"] = 0.8f;
        p.values["ensemble_spread"] = 12.0f;
        p.values["ensemble_voices"] = 4.0f;
        p.values["filter_cutoff"] = 1500.0f;
        p.values["filter_resonance"] = 2.0f;
        p.values["filter_env_amount"] = 800.0f;
        p.values["filter_attack"] = 1.0f;
        p.values["filter_decay"] = 0.5f;
        p.values["filter_sustain"] = 0.6f;
        p.values["filter_release"] = 2.5f;
        p.values["amp_attack"] = 0.9f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 2.5f;
        p.values["chorus_rate"] = 0.5f;
        p.values["chorus_depth"] = 0.7f;
        p.values["chorus_wet"] = 0.5f;
        p.values["phaser_wet"] = 0.3f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Solo Strings
    {
        SynthPreset p;
        p.name = "Solo Strings";
        p.category = "Solo";
        p.values["violins"] = 1.0f;
        p.values["violas"] = 0.0f;
        p.values["cellos"] = 0.0f;
        p.values["basses"] = 0.0f;
        p.values["ensemble_spread"] = 5.0f;
        p.values["ensemble_voices"] = 2.0f;
        p.values["filter_cutoff"] = 5000.0f;
        p.values["filter_resonance"] = 0.5f;
        p.values["filter_env_amount"] = 1500.0f;
        p.values["filter_attack"] = 0.2f;
        p.values["filter_decay"] = 0.15f;
        p.values["filter_sustain"] = 0.85f;
        p.values["filter_release"] = 0.8f;
        p.values["amp_attack"] = 0.2f;
        p.values["amp_decay"] = 0.15f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 0.8f;
        p.values["chorus_rate"] = 3.0f;
        p.values["chorus_depth"] = 0.2f;
        p.values["chorus_wet"] = 0.25f;
        p.values["phaser_wet"] = 0.1f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // Ambient Strings
    {
        SynthPreset p;
        p.name = "Ambient Strings";
        p.category = "Ambient";
        p.values["violins"] = 0.7f;
        p.values["violas"] = 0.7f;
        p.values["cellos"] = 0.5f;
        p.values["basses"] = 0.3f;
        p.values["ensemble_spread"] = 30.0f;
        p.values["ensemble_voices"] = 6.0f;
        p.values["filter_cutoff"] = 2500.0f;
        p.values["filter_resonance"] = 1.5f;
        p.values["filter_env_amount"] = 1500.0f;
        p.values["filter_attack"] = 2.0f;
        p.values["filter_decay"] = 1.0f;
        p.values["filter_sustain"] = 0.6f;
        p.values["filter_release"] = 4.0f;
        p.values["amp_attack"] = 2.5f;
        p.values["amp_decay"] = 1.0f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 5.0f;
        p.values["chorus_rate"] = 0.4f;
        p.values["chorus_depth"] = 0.8f;
        p.values["chorus_wet"] = 0.6f;
        p.values["phaser_wet"] = 0.4f;
        p.values["volume"] = 0.4f;
        presets.push_back(p);
    }

    // Cinematic Strings
    {
        SynthPreset p;
        p.name = "Cinematic Strings";
        p.category = "Cinematic";
        p.values["violins"] = 0.9f;
        p.values["violas"] = 0.8f;
        p.values["cellos"] = 0.8f;
        p.values["basses"] = 0.6f;
        p.values["ensemble_spread"] = 18.0f;
        p.values["ensemble_voices"] = 5.0f;
        p.values["filter_cutoff"] = 4000.0f;
        p.values["filter_resonance"] = 1.0f;
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.5f;
        p.values["filter_decay"] = 0.3f;
        p.values["filter_sustain"] = 0.8f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.5f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.85f;
        p.values["amp_release"] = 2.0f;
        p.values["chorus_rate"] = 1.0f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.5f;
        p.values["phaser_wet"] = 0.25f;
        p.values["volume"] = 0.5f;
        presets.push_back(p);
    }

    // 70s Strings
    {
        SynthPreset p;
        p.name = "70s Strings";
        p.category = "Vintage";
        p.values["violins"] = 0.8f;
        p.values["violas"] = 0.4f;
        p.values["cellos"] = 0.2f;
        p.values["basses"] = 0.0f;
        p.values["ensemble_spread"] = 20.0f;
        p.values["ensemble_voices"] = 6.0f;
        p.values["filter_cutoff"] = 4000.0f;
        p.values["filter_resonance"] = 2.0f;
        p.values["filter_env_amount"] = 2000.0f;
        p.values["filter_attack"] = 0.3f;
        p.values["filter_decay"] = 0.3f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 1.0f;
        p.values["amp_attack"] = 0.25f;
        p.values["amp_decay"] = 0.25f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 1.0f;
        p.values["chorus_rate"] = 1.5f;
        p.values["chorus_depth"] = 0.7f;
        p.values["chorus_wet"] = 0.6f;
        p.values["phaser_wet"] = 0.4f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    // Soft Strings
    {
        SynthPreset p;
        p.name = "Soft Strings";
        p.category = "Soft";
        p.values["violins"] = 0.6f;
        p.values["violas"] = 0.6f;
        p.values["cellos"] = 0.4f;
        p.values["basses"] = 0.2f;
        p.values["ensemble_spread"] = 10.0f;
        p.values["ensemble_voices"] = 4.0f;
        p.values["filter_cutoff"] = 2500.0f;
        p.values["filter_resonance"] = 0.8f;
        p.values["filter_env_amount"] = 1000.0f;
        p.values["filter_attack"] = 0.8f;
        p.values["filter_decay"] = 0.4f;
        p.values["filter_sustain"] = 0.7f;
        p.values["filter_release"] = 2.0f;
        p.values["amp_attack"] = 0.7f;
        p.values["amp_decay"] = 0.4f;
        p.values["amp_sustain"] = 0.75f;
        p.values["amp_release"] = 2.0f;
        p.values["chorus_rate"] = 0.8f;
        p.values["chorus_depth"] = 0.5f;
        p.values["chorus_wet"] = 0.4f;
        p.values["phaser_wet"] = 0.2f;
        p.values["volume"] = 0.45f;
        presets.push_back(p);
    }

    return presets;
}
