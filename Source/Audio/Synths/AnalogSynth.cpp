#include "AnalogSynth.h"
#include <cmath>

//==============================================================================
// Waveform generation
//==============================================================================

float AnalogSynthVoice::generateWave(WaveType type, double phase)
{
    switch (type)
    {
        case WaveType::Sine:
            return std::sin(phase * juce::MathConstants<double>::twoPi);

        case WaveType::Triangle:
        {
            // Triangle: 0→1→0→-1→0
            double t = std::fmod(phase, 1.0);
            if (t < 0.25)
                return static_cast<float>(t * 4.0);
            else if (t < 0.75)
                return static_cast<float>(2.0 - t * 4.0);
            else
                return static_cast<float>(t * 4.0 - 4.0);
        }

        case WaveType::Sawtooth:
        {
            // Sawtooth: -1 to 1 ramp
            double t = std::fmod(phase, 1.0);
            return static_cast<float>(2.0 * t - 1.0);
        }

        case WaveType::Square:
        {
            // Square: -1 or 1
            double t = std::fmod(phase, 1.0);
            return t < 0.5 ? 1.0f : -1.0f;
        }
    }

    return 0.0f;
}

float AnalogSynthVoice::Oscillator::generate(double frequency, double sr)
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
// AnalogSynthVoice Implementation
//==============================================================================

AnalogSynthVoice::AnalogSynthVoice()
{
    // Default oscillator settings
    osc1.waveType = WaveType::Sawtooth;
    osc1.level = 0.8f;
    osc1.octave = 0;

    osc2.waveType = WaveType::Sawtooth;
    osc2.level = 0.6f;
    osc2.octave = 0;
    osc2.detuneCents = 5.0f;

    osc3.waveType = WaveType::Square;
    osc3.level = 0.4f;
    osc3.octave = -1;

    subOsc.waveType = WaveType::Sine;
    subOsc.level = 0.0f;
    subOsc.octave = -1;

    filterEnvelope.setParameters(filterEnvParams);
}

void AnalogSynthVoice::prepareToPlay(double sr, int blockSize)
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

void AnalogSynthVoice::reset()
{
    SynthVoice::reset();

    osc1.reset();
    osc2.reset();
    osc3.reset();
    subOsc.reset();

    filter.reset();
    filterEnvelope.reset();

    lfoFilterMod = 0.0f;
    lfoPitchMod = 0.0f;
}

void AnalogSynthVoice::onNoteStart()
{
    // Reset oscillator phases for consistent attack
    osc1.phase = 0.0;
    osc2.phase = 0.0;
    osc3.phase = 0.0;
    subOsc.phase = 0.0;

    filterEnvelope.noteOn();
}

void AnalogSynthVoice::onNoteStop()
{
    filterEnvelope.noteOff();
}

double AnalogSynthVoice::getOscFrequency(const Oscillator& osc, float baseFreq)
{
    // Apply octave
    double freq = baseFreq * std::pow(2.0, osc.octave);

    // Apply detune (cents)
    float totalDetune = osc.detuneCents;

    // Add unison detune
    if (unisonIndex > 0)
    {
        totalDetune += unisonDetuneCents;
    }

    // Add LFO pitch modulation
    totalDetune += lfoPitchMod;

    freq *= std::pow(2.0, totalDetune / 1200.0);

    return freq;
}

void AnalogSynthVoice::renderNextBlock(juce::AudioBuffer<float>& buffer,
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

        // Generate oscillators
        float osc1Sample = osc1.generate(getOscFrequency(osc1, baseFreq), sampleRate);
        float osc2Sample = osc2.generate(getOscFrequency(osc2, baseFreq), sampleRate);
        float osc3Sample = osc3.generate(getOscFrequency(osc3, baseFreq), sampleRate);

        // Sub oscillator doesn't get unison detune or pitch LFO for stability
        double subFreq = baseFreq * std::pow(2.0, subOsc.octave);
        float subSample = subOsc.generate(subFreq, sampleRate);

        // Mix oscillators
        float mixed = osc1Sample + osc2Sample + osc3Sample + subSample;

        // Normalize mix (prevent clipping with all oscs at max)
        float totalLevel = osc1.level + osc2.level + osc3.level + subOsc.level;
        if (totalLevel > 0.0f)
            mixed *= juce::jmin(1.0f, 2.0f / totalLevel);

        // Calculate filter cutoff with envelope and LFO modulation
        float modulatedCutoff = filterCutoff;
        modulatedCutoff += filterEnvAmount * filterEnv;
        modulatedCutoff += lfoFilterMod;
        modulatedCutoff = juce::jlimit(20.0f, 20000.0f, modulatedCutoff);

        // Update filter
        filter.setCutoffFrequency(modulatedCutoff);

        // Process through filter (mono) - StateVariableTPT uses processSample with channel
        float filtered = filter.processSample(0, mixed);

        // Apply amp envelope and velocity
        float output = filtered * ampEnv * velocity * 0.5f;

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

void AnalogSynthVoice::setOscWaveType(int oscIndex, WaveType type)
{
    switch (oscIndex)
    {
        case 0: osc1.waveType = type; break;
        case 1: osc2.waveType = type; break;
        case 2: osc3.waveType = type; break;
    }
}

void AnalogSynthVoice::setOscLevel(int oscIndex, float level)
{
    level = juce::jlimit(0.0f, 1.0f, level);
    switch (oscIndex)
    {
        case 0: osc1.level = level; break;
        case 1: osc2.level = level; break;
        case 2: osc3.level = level; break;
    }
}

void AnalogSynthVoice::setOscOctave(int oscIndex, int octave)
{
    octave = juce::jlimit(-2, 2, octave);
    switch (oscIndex)
    {
        case 0: osc1.octave = octave; break;
        case 1: osc2.octave = octave; break;
        case 2: osc3.octave = octave; break;
    }
}

void AnalogSynthVoice::setOscDetune(int oscIndex, float cents)
{
    cents = juce::jlimit(-100.0f, 100.0f, cents);
    switch (oscIndex)
    {
        case 0: osc1.detuneCents = cents; break;
        case 1: osc2.detuneCents = cents; break;
        case 2: osc3.detuneCents = cents; break;
    }
}

void AnalogSynthVoice::setSubWaveType(WaveType type)
{
    subOsc.waveType = type;
}

void AnalogSynthVoice::setSubLevel(float level)
{
    subOsc.level = juce::jlimit(0.0f, 1.0f, level);
}

void AnalogSynthVoice::setSubOctave(int octave)
{
    subOsc.octave = juce::jlimit(-2, -1, octave);
}

void AnalogSynthVoice::setFilterCutoff(float frequency)
{
    filterCutoff = juce::jlimit(20.0f, 20000.0f, frequency);
}

void AnalogSynthVoice::setFilterResonance(float resonance)
{
    filterResonance = juce::jlimit(0.0f, 1.0f, resonance);
    filter.setResonance(filterResonance);
}

void AnalogSynthVoice::setFilterType(FilterType type)
{
    filterType = type;
    switch (type)
    {
        case FilterType::LowPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case FilterType::HighPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case FilterType::BandPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }
}

void AnalogSynthVoice::setFilterEnvAmount(float amount)
{
    filterEnvAmount = juce::jlimit(-10000.0f, 10000.0f, amount);
}

void AnalogSynthVoice::setFilterEnvelope(float attack, float decay, float sustain, float release)
{
    filterEnvParams.attack = juce::jmax(0.001f, attack);
    filterEnvParams.decay = juce::jmax(0.001f, decay);
    filterEnvParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    filterEnvParams.release = juce::jmax(0.001f, release);
    filterEnvelope.setParameters(filterEnvParams);
}

//==============================================================================
// AnalogSynth Implementation
//==============================================================================

float AnalogSynth::LFO::process(double sr)
{
    float sample = AnalogSynthVoice::generateWave(waveType, phase);

    // Advance phase
    double phaseIncrement = rate / sr;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * depth;
}

AnalogSynth::AnalogSynth()
{
    initializeParameters();

    // Create voice pool
    for (auto& voice : voices)
    {
        voice = std::make_unique<AnalogSynthVoice>();
    }
}

AnalogSynth::~AnalogSynth()
{
    killAllNotes();
}

void AnalogSynth::initializeParameters()
{
    // Oscillator 1
    addEnumParameter("osc1_wave", "Osc 1 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 2);
    addParameter("osc1_octave", "Osc 1 Octave", 0.0f, -2.0f, 2.0f, 1.0f);
    addParameter("osc1_detune", "Osc 1 Detune", 0.0f, -100.0f, 100.0f);
    addParameter("osc1_level", "Osc 1 Level", 0.8f, 0.0f, 1.0f);

    // Oscillator 2
    addEnumParameter("osc2_wave", "Osc 2 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 2);
    addParameter("osc2_octave", "Osc 2 Octave", 0.0f, -2.0f, 2.0f, 1.0f);
    addParameter("osc2_detune", "Osc 2 Detune", 5.0f, -100.0f, 100.0f);
    addParameter("osc2_level", "Osc 2 Level", 0.6f, 0.0f, 1.0f);

    // Oscillator 3
    addEnumParameter("osc3_wave", "Osc 3 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 3);
    addParameter("osc3_octave", "Osc 3 Octave", -1.0f, -2.0f, 2.0f, 1.0f);
    addParameter("osc3_detune", "Osc 3 Detune", 0.0f, -100.0f, 100.0f);
    addParameter("osc3_level", "Osc 3 Level", 0.4f, 0.0f, 1.0f);

    // Sub Oscillator
    addEnumParameter("sub_wave", "Sub Wave", {"Sine", "Triangle", "Square"}, 0);
    addParameter("sub_octave", "Sub Octave", -1.0f, -2.0f, -1.0f, 1.0f);
    addParameter("sub_level", "Sub Level", 0.0f, 0.0f, 1.0f);

    // Filter
    addParameter("filter_cutoff", "Filter Cutoff", 5000.0f, 20.0f, 20000.0f);
    addParameter("filter_resonance", "Filter Resonance", 0.3f, 0.0f, 1.0f);
    addEnumParameter("filter_type", "Filter Type", {"LowPass", "HighPass", "BandPass"}, 0);
    addParameter("filter_env_amount", "Filter Env Amount", 2000.0f, -10000.0f, 10000.0f);

    // Filter Envelope
    addParameter("filter_attack", "Filter Attack", 0.01f, 0.001f, 2.0f);
    addParameter("filter_decay", "Filter Decay", 0.2f, 0.001f, 2.0f);
    addParameter("filter_sustain", "Filter Sustain", 0.5f, 0.0f, 1.0f);
    addParameter("filter_release", "Filter Release", 0.3f, 0.001f, 5.0f);

    // Amp Envelope
    addParameter("amp_attack", "Amp Attack", 0.01f, 0.001f, 2.0f);
    addParameter("amp_decay", "Amp Decay", 0.1f, 0.001f, 2.0f);
    addParameter("amp_sustain", "Amp Sustain", 0.7f, 0.0f, 1.0f);
    addParameter("amp_release", "Amp Release", 0.3f, 0.001f, 5.0f);

    // LFO 1 (Filter)
    addParameter("lfo1_rate", "LFO 1 Rate", 2.0f, 0.01f, 50.0f);
    addParameter("lfo1_depth", "LFO 1 Depth", 0.0f, 0.0f, 1.0f);
    addEnumParameter("lfo1_wave", "LFO 1 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 0);

    // LFO 2 (Pitch)
    addParameter("lfo2_rate", "LFO 2 Rate", 0.5f, 0.01f, 50.0f);
    addParameter("lfo2_depth", "LFO 2 Depth", 0.0f, 0.0f, 1.0f);
    addEnumParameter("lfo2_wave", "LFO 2 Wave", {"Sine", "Triangle", "Sawtooth", "Square"}, 0);

    // Glide
    addParameter("glide", "Glide Time", 0.0f, 0.0f, 1.0f);

    // Unison
    addParameter("unison_voices", "Unison Voices", 1.0f, 1.0f, 4.0f, 1.0f);
    addParameter("unison_detune", "Unison Detune", 10.0f, 0.0f, 50.0f);

    // Master
    addParameter("master_volume", "Volume", 0.8f, 0.0f, 1.0f);
}

void AnalogSynth::prepareToPlay(double sr, int blockSize)
{
    SynthBase::prepareToPlay(sr, blockSize);

    for (auto& voice : voices)
    {
        voice->prepareToPlay(sr, blockSize);
    }

    lfo1.reset();
    lfo2.reset();

    updateVoiceParameters();
}

void AnalogSynth::releaseResources()
{
    SynthBase::releaseResources();

    for (auto& voice : voices)
    {
        voice->reset();
    }
}

void AnalogSynth::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // Clear buffer
    buffer.clear();

    // Process MIDI
    processMidiMessages(midiMessages);

    // Process LFOs and voices
    int numSamples = buffer.getNumSamples();

    // Calculate LFO values for this block
    float lfo1Depth = getParameter("lfo1_depth");
    float lfo2Depth = getParameter("lfo2_depth");
    lfo1.rate = getParameter("lfo1_rate");
    lfo2.rate = getParameter("lfo2_rate");
    lfo1.waveType = static_cast<WaveType>(getParameterEnum("lfo1_wave"));
    lfo2.waveType = static_cast<WaveType>(getParameterEnum("lfo2_wave"));
    lfo1.depth = lfo1Depth * 50.0f; // Scale for filter mod
    lfo2.depth = lfo2Depth * 10.0f; // Scale for pitch mod (cents)

    // Process in small chunks for LFO modulation
    const int chunkSize = 32;
    int samplesRemaining = numSamples;
    int sampleOffset = 0;

    while (samplesRemaining > 0)
    {
        int samplesToProcess = juce::jmin(chunkSize, samplesRemaining);

        // Update LFO values
        float lfo1Value = lfo1.process(sampleRate);
        float lfo2Value = lfo2.process(sampleRate);

        // Apply LFO modulation to all active voices
        for (auto& voice : voices)
        {
            if (voice->isActive())
            {
                voice->setLfoFilterMod(lfo1Value);
                voice->setLfoPitchMod(lfo2Value);
                voice->renderNextBlock(buffer, sampleOffset, samplesToProcess);
            }
        }

        sampleOffset += samplesToProcess;
        samplesRemaining -= samplesToProcess;
    }

    // Apply master volume
    float masterVol = getParameter("master_volume");
    buffer.applyGain(masterVol);
}

//==============================================================================
// Voice management

AnalogSynthVoice* AnalogSynth::findFreeVoice()
{
    for (auto& voice : voices)
    {
        if (!voice->isActive())
            return voice.get();
    }
    return nullptr;
}

AnalogSynthVoice* AnalogSynth::findVoiceToSteal()
{
    // Find oldest voice in release, or oldest overall
    AnalogSynthVoice* oldestRelease = nullptr;
    AnalogSynthVoice* oldest = nullptr;
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

AnalogSynthVoice* AnalogSynth::findVoicePlayingNote(int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice->isActive() && voice->getCurrentNote() == midiNote)
            return voice.get();
    }
    return nullptr;
}

float AnalogSynth::getUnisonDetuneForVoice(int voiceIndex, int totalVoices)
{
    if (totalVoices <= 1) return 0.0f;

    // Spread symmetrically: -detune to +detune
    float spread = (static_cast<float>(voiceIndex) / static_cast<float>(totalVoices - 1)) * 2.0f - 1.0f;
    return spread * unisonDetune;
}

void AnalogSynth::noteOn(int midiNote, float velocity, int /* sampleOffset */)
{
    int unisonCount = static_cast<int>(getParameter("unison_voices"));
    unisonDetune = getParameter("unison_detune");
    float glideTime = getParameter("glide") * 0.5f; // Max 0.5s glide

    // Check if we're doing legato (note already playing)
    bool legato = hasActiveNotes() && glideTime > 0.0f;

    for (int i = 0; i < unisonCount; ++i)
    {
        AnalogSynthVoice* voice = findFreeVoice();
        if (!voice)
            voice = findVoiceToSteal();

        if (voice)
        {
            voice->setUnisonIndex(i);
            voice->setUnisonDetune(getUnisonDetuneForVoice(i, unisonCount));
            voice->setPortamentoTime(glideTime);
            voice->startNote(midiNote, velocity, legato);
        }
    }

    activeNotes.insert(midiNote);
}

void AnalogSynth::noteOff(int midiNote, int /* sampleOffset */)
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

void AnalogSynth::allNotesOff()
{
    for (auto& voice : voices)
    {
        if (voice->isActive())
            voice->stopNote(true);
    }
    activeNotes.clear();
}

void AnalogSynth::killAllNotes()
{
    for (auto& voice : voices)
    {
        voice->killNote();
    }
    activeNotes.clear();
}

//==============================================================================
// Parameter updates

void AnalogSynth::updateVoiceParameters()
{
    for (auto& voice : voices)
    {
        // Oscillators
        voice->setOscWaveType(0, static_cast<WaveType>(getParameterEnum("osc1_wave")));
        voice->setOscLevel(0, getParameter("osc1_level"));
        voice->setOscOctave(0, static_cast<int>(getParameter("osc1_octave")));
        voice->setOscDetune(0, getParameter("osc1_detune"));

        voice->setOscWaveType(1, static_cast<WaveType>(getParameterEnum("osc2_wave")));
        voice->setOscLevel(1, getParameter("osc2_level"));
        voice->setOscOctave(1, static_cast<int>(getParameter("osc2_octave")));
        voice->setOscDetune(1, getParameter("osc2_detune"));

        voice->setOscWaveType(2, static_cast<WaveType>(getParameterEnum("osc3_wave")));
        voice->setOscLevel(2, getParameter("osc3_level"));
        voice->setOscOctave(2, static_cast<int>(getParameter("osc3_octave")));
        voice->setOscDetune(2, getParameter("osc3_detune"));

        // Sub
        int subWaveIdx = getParameterEnum("sub_wave");
        WaveType subWave = subWaveIdx == 0 ? WaveType::Sine :
                          subWaveIdx == 1 ? WaveType::Triangle : WaveType::Square;
        voice->setSubWaveType(subWave);
        voice->setSubLevel(getParameter("sub_level"));
        voice->setSubOctave(static_cast<int>(getParameter("sub_octave")));

        // Filter
        voice->setFilterCutoff(getParameter("filter_cutoff"));
        voice->setFilterResonance(getParameter("filter_resonance"));
        voice->setFilterType(static_cast<FilterType>(getParameterEnum("filter_type")));
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
    }

    // Update unison settings
    unisonVoices = static_cast<int>(getParameter("unison_voices"));
    unisonDetune = getParameter("unison_detune");
}

void AnalogSynth::onParameterChanged(const juce::String& name, float value)
{
    // Update all voices with the new parameter
    updateVoiceParameters();
}

void AnalogSynth::onParameterEnumChanged(const juce::String& name, int index)
{
    updateVoiceParameters();
}

//==============================================================================
// Presets

std::vector<SynthPreset> AnalogSynth::getPresets() const
{
    std::vector<SynthPreset> presets;

    // Init preset
    {
        SynthPreset p;
        p.name = "Init";
        p.category = "Basic";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.values["osc1_level"] = 0.8f;
        p.values["osc1_octave"] = 0.0f;
        p.values["filter_cutoff"] = 5000.0f;
        p.values["filter_resonance"] = 0.3f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_decay"] = 0.1f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 0.3f;
        presets.push_back(p);
    }

    // Warm Pad
    {
        SynthPreset p;
        p.name = "Warm Pad";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2;
        p.values["osc1_level"] = 0.6f;
        p.values["osc2_level"] = 0.6f;
        p.values["osc2_detune"] = 8.0f;
        p.values["filter_cutoff"] = 2000.0f;
        p.values["filter_resonance"] = 0.2f;
        p.values["amp_attack"] = 0.8f;
        p.values["amp_decay"] = 0.5f;
        p.values["amp_sustain"] = 0.7f;
        p.values["amp_release"] = 1.5f;
        p.values["lfo1_rate"] = 0.5f;
        p.values["lfo1_depth"] = 0.2f;
        presets.push_back(p);
    }

    // Fat Bass
    {
        SynthPreset p;
        p.name = "Fat Bass";
        p.category = "Bass";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 3; // Square
        p.values["osc1_level"] = 0.7f;
        p.values["osc2_level"] = 0.5f;
        p.values["osc2_octave"] = -1.0f;
        p.values["sub_level"] = 0.4f;
        p.values["filter_cutoff"] = 800.0f;
        p.values["filter_resonance"] = 0.4f;
        p.values["filter_env_amount"] = 3000.0f;
        p.values["filter_decay"] = 0.3f;
        p.values["filter_sustain"] = 0.2f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_decay"] = 0.2f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 0.2f;
        presets.push_back(p);
    }

    // Pluck
    {
        SynthPreset p;
        p.name = "Pluck";
        p.category = "Pluck";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.values["osc1_level"] = 0.8f;
        p.values["filter_cutoff"] = 3000.0f;
        p.values["filter_env_amount"] = 5000.0f;
        p.values["filter_attack"] = 0.001f;
        p.values["filter_decay"] = 0.15f;
        p.values["filter_sustain"] = 0.1f;
        p.values["amp_attack"] = 0.001f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.0f;
        p.values["amp_release"] = 0.1f;
        presets.push_back(p);
    }

    // Brass
    {
        SynthPreset p;
        p.name = "Brass";
        p.category = "Brass";
        p.enumValues["osc1_wave"] = 2; // Sawtooth
        p.enumValues["osc2_wave"] = 2;
        p.values["osc1_level"] = 0.5f;
        p.values["osc2_level"] = 0.5f;
        p.values["osc2_detune"] = 3.0f;
        p.values["filter_cutoff"] = 1500.0f;
        p.values["filter_resonance"] = 0.15f;
        p.values["filter_env_amount"] = 4000.0f;
        p.values["filter_attack"] = 0.1f;
        p.values["filter_decay"] = 0.2f;
        p.values["filter_sustain"] = 0.6f;
        p.values["amp_attack"] = 0.08f;
        p.values["amp_decay"] = 0.1f;
        p.values["amp_sustain"] = 0.9f;
        p.values["amp_release"] = 0.15f;
        presets.push_back(p);
    }

    // Supersaw
    {
        SynthPreset p;
        p.name = "Supersaw";
        p.category = "Lead";
        p.enumValues["osc1_wave"] = 2;
        p.enumValues["osc2_wave"] = 2;
        p.enumValues["osc3_wave"] = 2;
        p.values["osc1_level"] = 0.5f;
        p.values["osc2_level"] = 0.5f;
        p.values["osc3_level"] = 0.5f;
        p.values["osc2_detune"] = 10.0f;
        p.values["osc3_detune"] = -10.0f;
        p.values["unison_voices"] = 3.0f;
        p.values["unison_detune"] = 15.0f;
        p.values["filter_cutoff"] = 6000.0f;
        p.values["filter_resonance"] = 0.2f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_release"] = 0.4f;
        presets.push_back(p);
    }

    // Wobble Bass
    {
        SynthPreset p;
        p.name = "Wobble Bass";
        p.category = "Bass";
        p.enumValues["osc1_wave"] = 2;
        p.values["osc1_level"] = 0.8f;
        p.values["sub_level"] = 0.5f;
        p.values["filter_cutoff"] = 500.0f;
        p.values["filter_resonance"] = 0.5f;
        p.values["lfo1_rate"] = 4.0f;
        p.values["lfo1_depth"] = 0.6f;
        p.values["amp_attack"] = 0.01f;
        p.values["amp_sustain"] = 1.0f;
        presets.push_back(p);
    }

    // Strings
    {
        SynthPreset p;
        p.name = "Strings";
        p.category = "Pad";
        p.enumValues["osc1_wave"] = 2;
        p.enumValues["osc2_wave"] = 2;
        p.values["osc1_level"] = 0.5f;
        p.values["osc2_level"] = 0.5f;
        p.values["osc2_detune"] = 12.0f;
        p.values["filter_cutoff"] = 3000.0f;
        p.values["filter_resonance"] = 0.1f;
        p.values["amp_attack"] = 0.5f;
        p.values["amp_decay"] = 0.3f;
        p.values["amp_sustain"] = 0.8f;
        p.values["amp_release"] = 0.8f;
        p.values["lfo2_rate"] = 5.0f;
        p.values["lfo2_depth"] = 0.1f;
        presets.push_back(p);
    }

    return presets;
}
