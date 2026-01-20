#include "ProSynth.h"
#include <cmath>

namespace
{
    // Generate basic waveforms
    float generateWave(ProWaveType type, double phase)
    {
        switch (type)
        {
            case ProWaveType::Sine:
                return static_cast<float>(std::sin(phase * juce::MathConstants<double>::twoPi));

            case ProWaveType::Triangle:
            {
                double t = std::fmod(phase, 1.0);
                return static_cast<float>(2.0 * std::abs(2.0 * t - 1.0) - 1.0);
            }

            case ProWaveType::Sawtooth:
                return static_cast<float>(2.0 * std::fmod(phase, 1.0) - 1.0);

            case ProWaveType::Square:
                return std::fmod(phase, 1.0) < 0.5 ? 1.0f : -1.0f;

            default:
                return 0.0f;
        }
    }
}

//==============================================================================
// ProSynthVoice Implementation
//==============================================================================

ProSynthVoice::ProSynthVoice()
{
    // Initialize oscillators with default settings
    oscillators[0].basicWave = ProWaveType::Sawtooth;
    oscillators[0].level = 1.0f;

    oscillators[1].basicWave = ProWaveType::Sawtooth;
    oscillators[1].level = 0.5f;
    oscillators[1].fine = 7.0f;

    oscillators[2].basicWave = ProWaveType::Square;
    oscillators[2].level = 0.3f;
    oscillators[2].octave = -1;
    oscillators[2].enabled = false;

    filterEnvelope.setParameters(filterEnvParams);
}

void ProSynthVoice::prepareToPlay(double sr, int blockSize)
{
    SynthVoice::prepareToPlay(sr, blockSize);

    // Prepare wavetable oscillators
    for (auto& osc : oscillators)
    {
        osc.wavetableOsc.prepareToPlay(sr, blockSize);
    }

    // Prepare filters
    filter1.prepareToPlay(sr, blockSize);
    filter2.prepareToPlay(sr, blockSize);

    // Prepare sub and noise
    subOsc.prepareToPlay(sr);
    noiseGen.prepareToPlay(sr, blockSize);

    // Prepare envelopes
    filterEnvelope.setSampleRate(sr);
}

void ProSynthVoice::reset()
{
    SynthVoice::reset();

    for (auto& osc : oscillators)
        osc.reset();

    filter1.reset();
    filter2.reset();
    filterEnvelope.reset();
    subOsc.reset();
    noiseGen.reset();
}

void ProSynthVoice::Oscillator::reset()
{
    phase = 0.0;
    fmCarrierPhase = 0.0;
    fmModulatorPhase = 0.0;
    wavetableOsc.reset();
}

float ProSynthVoice::Oscillator::process(double baseFrequency, double sr)
{
    if (!enabled || level <= 0.0f)
        return 0.0f;

    switch (mode)
    {
        case ProOscMode::Basic:
        {
            float sample = generateWave(basicWave, phase);

            double phaseIncrement = baseFrequency / sr;
            phase += phaseIncrement;
            if (phase >= 1.0)
                phase -= 1.0;

            return sample * level;
        }

        case ProOscMode::Wavetable:
        {
            wavetableOsc.setFrequency(static_cast<float>(baseFrequency));
            return wavetableOsc.processSample() * level;
        }

        case ProOscMode::FM:
        {
            // 2-operator FM: modulator modulates carrier
            float modulator = std::sin(fmModulatorPhase * juce::MathConstants<double>::twoPi);
            float modulationAmount = modulator * fmDepth * fmRatio;
            float carrier = std::sin((fmCarrierPhase + modulationAmount) * juce::MathConstants<double>::twoPi);

            double carrierIncrement = baseFrequency / sr;
            double modulatorIncrement = (baseFrequency * fmRatio) / sr;

            fmCarrierPhase += carrierIncrement;
            if (fmCarrierPhase >= 1.0)
                fmCarrierPhase -= 1.0;

            fmModulatorPhase += modulatorIncrement;
            if (fmModulatorPhase >= 1.0)
                fmModulatorPhase -= 1.0;

            return carrier * level;
        }
    }

    return 0.0f;
}

float ProSynthVoice::calculateOscFrequency(const Oscillator& osc, float baseFreq)
{
    // Apply octave
    float freq = baseFreq * std::pow(2.0f, static_cast<float>(osc.octave));

    // Apply semitone
    freq *= std::pow(2.0f, static_cast<float>(osc.semi) / 12.0f);

    // Apply fine tuning (cents)
    freq *= std::pow(2.0f, osc.fine / 1200.0f);

    // Apply unison detune (cents)
    if (unisonDetuneCents != 0.0f)
    {
        freq *= std::pow(2.0f, unisonDetuneCents / 1200.0f);
    }

    return freq;
}

void ProSynthVoice::onNoteStart()
{
    // Reset oscillators for consistent attack
    for (auto& osc : oscillators)
    {
        osc.phase = 0.0;
        osc.fmCarrierPhase = 0.0;
        osc.fmModulatorPhase = 0.0;
        osc.wavetableOsc.start();
    }

    // Trigger sub and noise
    float baseFreq = SynthBase::midiToFrequency(currentNote);
    subOsc.trigger(baseFreq);
    noiseGen.trigger();

    // Trigger filter envelope
    filterEnvelope.noteOn();
}

void ProSynthVoice::onNoteStop()
{
    // Stop oscillators
    for (auto& osc : oscillators)
    {
        osc.wavetableOsc.stop();
    }

    // Release sub and noise
    subOsc.release();
    noiseGen.release();

    // Release filter envelope
    filterEnvelope.noteOff();
}

void ProSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples)
{
    if (!isActive())
        return;

    auto* outputL = buffer.getWritePointer(0, startSample);
    auto* outputR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1, startSample) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        // Update portamento
        float baseFreq = getNextFrequency();

        // Get envelope values
        float ampEnv = ampEnvelope.getNextSample();
        float filterEnv = filterEnvelope.getNextSample();

        // Check if voice should go idle
        if (state == VoiceState::Release && ampEnv < 0.0001f)
        {
            state = VoiceState::Idle;
            currentNote = -1;
            break;
        }

        // Generate oscillators
        float osc1Sample = oscillators[0].process(calculateOscFrequency(oscillators[0], baseFreq), sampleRate);
        float osc2Sample = oscillators[1].process(calculateOscFrequency(oscillators[1], baseFreq), sampleRate);
        float osc3Sample = oscillators[2].process(calculateOscFrequency(oscillators[2], baseFreq), sampleRate);

        // Generate sub and noise
        float subSample = subOsc.processSample();
        float noiseSample = noiseGen.processSample();

        // Mix oscillators + sub + noise
        float mixed = osc1Sample + osc2Sample + osc3Sample + subSample + noiseSample;

        // Process through filters based on routing
        float filtered = 0.0f;

        // Calculate filter cutoff with envelope and keytracking
        float filter1Cutoff = filter1.getCutoff();
        if (filterKeytrack > 0.0f)
        {
            float keytrackAmount = (baseFreq / 261.63f) * filterKeytrack;
            filter1Cutoff *= (1.0f + keytrackAmount);
        }
        filter1Cutoff += filterEnvAmount * filterEnv;
        filter1Cutoff = juce::jlimit(20.0f, 20000.0f, filter1Cutoff);

        if (filter2Enabled)
        {
            switch (filterRouting)
            {
                case FilterRouting::Serial:
                    // OSC -> Filter1 -> Filter2
                    filtered = filter1.processSample(mixed);
                    filtered = filter2.processSample(filtered);
                    break;

                case FilterRouting::Parallel:
                    // OSC -> (Filter1 + Filter2) / 2
                    {
                        float f1 = filter1.processSample(mixed);
                        float f2 = filter2.processSample(mixed);
                        filtered = (f1 + f2) * 0.5f;
                    }
                    break;

                case FilterRouting::Split:
                    // Osc1+2 -> Filter1, Osc3 -> Filter2
                    {
                        float path1 = osc1Sample + osc2Sample + subSample + noiseSample;
                        float path2 = osc3Sample;
                        float f1 = filter1.processSample(path1);
                        float f2 = filter2.processSample(path2);
                        filtered = f1 + f2;
                    }
                    break;
            }
        }
        else
        {
            // Single filter
            filtered = filter1.processSample(mixed);
        }

        // Apply amp envelope and velocity
        float output = filtered * ampEnv * velocity * 0.5f;

        // Write to buffer (mono for now - panning handled later if needed)
        outputL[i] += output;
        if (outputR != nullptr)
            outputR[i] += output;

        // Update voice age
        incrementAge(1);
    }
}

void ProSynthVoice::setOscSettings(int oscIndex, const OscSettings& settings)
{
    if (oscIndex < 0 || oscIndex >= 3)
        return;

    auto& osc = oscillators[oscIndex];
    osc.enabled = settings.enabled;
    osc.mode = settings.mode;
    osc.basicWave = settings.basicWave;
    osc.level = settings.level;
    osc.pan = settings.pan;
    osc.octave = settings.octave;
    osc.semi = settings.semi;
    osc.fine = settings.fine;

    // Wavetable settings
    if (settings.mode == ProOscMode::Wavetable)
    {
        osc.wavetableOsc.setWavetableById(settings.wavetableId);
        osc.wavetableOsc.setPosition(settings.wtPosition);
    }

    // FM settings
    if (settings.mode == ProOscMode::FM)
    {
        osc.fmRatio = settings.fmRatio;
        osc.fmDepth = settings.fmDepth;
    }
}

void ProSynthVoice::setSubOscSettings(bool enabled, SubOscWaveform wave, int octave, float level)
{
    if (enabled)
    {
        subOsc.setWaveform(wave);
        subOsc.setOctave(octave);
        subOsc.setLevel(level);
    }
    else
    {
        subOsc.setLevel(0.0f);
    }
}

void ProSynthVoice::setNoiseSettings(bool enabled, NoiseType type, float level,
                                    bool filterEnabled, NoiseFilterType filterType,
                                    float filterCutoff, float filterResonance)
{
    if (enabled)
    {
        noiseGen.setNoiseType(type);
        noiseGen.setLevel(level);
        noiseGen.setFilterEnabled(filterEnabled);
        noiseGen.setFilterType(filterType);
        noiseGen.setFilterCutoff(filterCutoff);
        noiseGen.setFilterResonance(filterResonance);
    }
    else
    {
        noiseGen.setLevel(0.0f);
    }
}

void ProSynthVoice::setFilter1(ProFilterModel model, ProFilterType type, float cutoff,
                               float resonance, float drive, float keytrack)
{
    filter1.setModel(model);
    filter1.setType(type);
    filter1.setCutoff(cutoff);
    filter1.setResonance(resonance);
    filter1.setDrive(drive);
    filterKeytrack = keytrack;
}

void ProSynthVoice::setFilter2(bool enabled, ProFilterModel model, ProFilterType type,
                               float cutoff, float resonance, float drive)
{
    filter2Enabled = enabled;
    filter2.setModel(model);
    filter2.setType(type);
    filter2.setCutoff(cutoff);
    filter2.setResonance(resonance);
    filter2.setDrive(drive);
}

void ProSynthVoice::setFilterRouting(FilterRouting routing)
{
    filterRouting = routing;
}

void ProSynthVoice::setFilterEnvelope(float attack, float decay, float sustain, float release, float amount)
{
    filterEnvParams.attack = juce::jmax(0.001f, attack);
    filterEnvParams.decay = juce::jmax(0.001f, decay);
    filterEnvParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    filterEnvParams.release = juce::jmax(0.001f, release);
    filterEnvelope.setParameters(filterEnvParams);
    filterEnvAmount = amount;
}

//==============================================================================
// ProSynth Implementation
//==============================================================================

ProSynth::ProSynth()
{
    initializeParameters();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<ProSynthVoice>();
    }

    // Initialize LFOs
    for (auto& lfo : lfos)
    {
        lfo.setRange(-1.0f, 1.0f);
        lfo.start();
    }
}

ProSynth::~ProSynth()
{
    killAllNotes();
}

void ProSynth::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    // Prepare voices
    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    // Prepare LFOs
    for (auto& lfo : lfos)
    {
        lfo.prepareToPlay(sr);
    }

    // Prepare effects
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 2;

    distortion.prepare(spec);
    chorus.prepare(spec);
    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(static_cast<int>(sr * 2.0)); // 2 second max delay

    updateVoiceParameters();
}

void ProSynth::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }
}

//==============================================================================
// Voice management

ProSynthVoice* ProSynth::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

ProSynthVoice* ProSynth::findVoiceToSteal()
{
    // Find oldest voice in release, or oldest overall
    ProSynthVoice* oldestRelease = nullptr;
    ProSynthVoice* oldest = nullptr;
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

void ProSynth::noteOn(int midiNote, float vel, int /*sampleOffset*/)
{
    int unisonCount = static_cast<int>(getParameter("unison_voices"));
    float glideTime = getParameter("glide") * 0.5f;
    bool legato = hasActiveNotes() && glideTime > 0.0f;

    // Allocate unison voices
    for (int i = 0; i < unisonCount; ++i)
    {
        ProSynthVoice* voice = findFreeVoice();
        if (!voice)
            voice = findVoiceToSteal();

        if (voice)
        {
            // Apply unison detune (getDetuneForVoice returns cents)
            float detune = unisonEngine.getDetuneForVoice(i);
            voice->setUnisonDetune(detune);

            voice->setPortamentoTime(glideTime);
            voice->startNote(midiNote, vel, legato);
        }
    }

    activeNotes.insert(midiNote);
}

void ProSynth::noteOff(int midiNote, int /*sampleOffset*/)
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

void ProSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void ProSynth::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Process block

void ProSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    buffer.clear();

    // Process MIDI
    processMidiMessages(midiMessages);

    // Process LFOs and voices
    int numSamples = buffer.getNumSamples();

    // Update LFOs with current BPM from transport
    for (size_t i = 0; i < lfos.size(); ++i)
    {
        lfos[i].setBPM(static_cast<float>(getBpm()));
    }

    // Process voices
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

    // Process effects
    processEffects(buffer);
}

void ProSynth::processEffects(juce::AudioBuffer<float>& buffer)
{
    // Built-in effects processing
    // Simplified for now - distortion, chorus, delay

    bool distEnabled = getParameter("fx_distortion_enabled") > 0.5f;
    bool chorusEnabled = getParameter("fx_chorus_enabled") > 0.5f;
    bool delayEnabled = getParameter("fx_delay_enabled") > 0.5f;

    if (distEnabled)
    {
        float drive = getParameter("fx_distortion_drive");

        // Apply drive as pre-gain, then soft-clip
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            float preGain = 1.0f + drive * 5.0f;
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                data[i] = std::tanh(data[i] * preGain);
            }
        }
    }

    if (chorusEnabled)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        chorus.process(context);
    }

    // Delay processing simplified
    (void)delayEnabled; // Suppress warning for now
}

//==============================================================================
// Continue in next message due to length...
