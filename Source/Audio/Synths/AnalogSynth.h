#pragma once

#include "SynthBase.h"
#include "SynthVoice.h"
#include <juce_dsp/juce_dsp.h>

/**
 * Waveform types for oscillators
 */
enum class WaveType
{
    Sine = 0,
    Triangle,
    Sawtooth,
    Square
};

/**
 * Filter types
 */
enum class FilterType
{
    LowPass = 0,
    HighPass,
    BandPass
};

/**
 * AnalogSynthVoice - A single voice for the analog synth
 *
 * Signal chain per voice:
 * OSC1 → Gain ─┐
 * OSC2 → Gain ─┼→ Mix → Filter → AmpEnv → Output
 * OSC3 → Gain ─┤        ↑
 * Sub  → Gain ─┘        FilterEnv
 */
class AnalogSynthVoice : public SynthVoice
{
public:
    AnalogSynthVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                         int startSample, int numSamples) override;

    //==========================================================================
    // Oscillator settings (set by parent synth)
    void setOscWaveType(int oscIndex, WaveType type);
    void setOscLevel(int oscIndex, float level);
    void setOscOctave(int oscIndex, int octave);
    void setOscDetune(int oscIndex, float cents);

    // Sub oscillator
    void setSubWaveType(WaveType type);
    void setSubLevel(float level);
    void setSubOctave(int octave);

    //==========================================================================
    // Filter settings
    void setFilterCutoff(float frequency);
    void setFilterResonance(float resonance);
    void setFilterType(FilterType type);
    void setFilterEnvAmount(float amount);
    void setFilterEnvelope(float attack, float decay, float sustain, float release);

    //==========================================================================
    // LFO modulation (applied per-sample from parent)
    void setLfoFilterMod(float amount) { lfoFilterMod = amount; }
    void setLfoPitchMod(float amount) { lfoPitchMod = amount; }

    //==========================================================================
    // Unison
    void setUnisonDetune(float cents) { unisonDetuneCents = cents; }
    int getUnisonIndex() const { return unisonIndex; }
    void setUnisonIndex(int index) { unisonIndex = index; }

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Oscillators (using simple wavetable approach)
    struct Oscillator
    {
        double phase = 0.0;
        WaveType waveType = WaveType::Sawtooth;
        float level = 0.8f;
        int octave = 0;
        float detuneCents = 0.0f;

        float generate(double frequency, double sampleRate);
        void reset() { phase = 0.0; }
    };

    Oscillator osc1, osc2, osc3, subOsc;

    // Filter - using StateVariableTPT for per-sample processing
    juce::dsp::StateVariableTPTFilter<float> filter;
    float filterCutoff = 5000.0f;
    float filterResonance = 0.5f;
    FilterType filterType = FilterType::LowPass;
    float filterEnvAmount = 2000.0f;

    // Filter envelope
    juce::ADSR filterEnvelope;
    juce::ADSR::Parameters filterEnvParams{0.01f, 0.2f, 0.5f, 0.3f};

    // LFO modulation values (set per-sample from parent)
    float lfoFilterMod = 0.0f;
    float lfoPitchMod = 0.0f;

    // Unison
    int unisonIndex = 0;
    float unisonDetuneCents = 0.0f;

    // Helper to calculate oscillator frequency with all modifiers
    double getOscFrequency(const Oscillator& osc, float baseFreq);

    // Generate waveform - public for LFO access
public:
    static float generateWave(WaveType type, double phase);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogSynthVoice)
};

//==============================================================================

/**
 * AnalogSynth - Classic polyphonic analog-style synthesizer
 *
 * Features:
 * - 3 oscillators + sub oscillator
 * - Multi-mode ladder filter with envelope
 * - Amp envelope (ADSR)
 * - 2 LFOs for filter/pitch modulation
 * - Unison mode with detuning
 * - Voice stealing
 * - 50+ presets
 */
class AnalogSynth : public SynthBase
{
public:
    AnalogSynth();
    ~AnalogSynth() override;

    //==========================================================================
    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override;
    void releaseResources() override;

    //==========================================================================
    // Note handling
    void noteOn(int midiNote, float velocity, int sampleOffset = 0) override;
    void noteOff(int midiNote, int sampleOffset = 0) override;
    void allNotesOff() override;
    void killAllNotes() override;

    //==========================================================================
    // Presets
    std::vector<SynthPreset> getPresets() const override;

    static constexpr int MAX_VOICES = 8;

protected:
    void onParameterChanged(const juce::String& name, float value) override;
    void onParameterEnumChanged(const juce::String& name, int index) override;

private:
    // Voice pool
    std::array<std::unique_ptr<AnalogSynthVoice>, MAX_VOICES> voices;
    int voiceRoundRobin = 0;

    // LFOs (global, shared across voices)
    struct LFO
    {
        double phase = 0.0;
        float rate = 2.0f;
        float depth = 0.0f;
        WaveType waveType = WaveType::Sine;

        float process(double sampleRate);
        void reset() { phase = 0.0; }
    };

    LFO lfo1, lfo2;

    // Unison settings
    int unisonVoices = 1;
    float unisonDetune = 10.0f;

    // Initialize all parameters
    void initializeParameters();

    // Update all voice parameters
    void updateVoiceParameters();

    // Voice allocation
    AnalogSynthVoice* findFreeVoice();
    AnalogSynthVoice* findVoiceToSteal();
    AnalogSynthVoice* findVoicePlayingNote(int midiNote);

    // Unison detune calculation
    float getUnisonDetuneForVoice(int voiceIndex, int totalVoices);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogSynth)
};
