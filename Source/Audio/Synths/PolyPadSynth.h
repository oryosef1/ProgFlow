#pragma once

#include "SynthBase.h"
#include "SynthVoice.h"
#include <juce_dsp/juce_dsp.h>

/**
 * Waveform types for oscillators
 */
enum class PadWaveType
{
    Sine = 0,
    Triangle,
    Sawtooth,
    Square
};

/**
 * Filter types for pad synth
 */
enum class PadFilterType
{
    LowPass = 0,
    HighPass,
    BandPass
};

/**
 * PolyPadSynthVoice - A single voice for the polyphonic pad synth
 *
 * Signal chain per voice:
 * OSC1 → Gain ─┐
 * OSC2 → Gain ─┤
 * OSC3 → Gain ─┼→ Mix → Filter → AmpEnv → Chorus → Output
 * OSC4 → Gain ─┘        ↑
 *                   FilterEnv
 *
 * Features:
 * - 4 oscillators with crossfade mixing (osc1/3 vs osc2/4)
 * - Multi-mode filter with envelope
 * - Per-voice chorus for width
 */
class PolyPadSynthVoice : public SynthVoice
{
public:
    PolyPadSynthVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                         int startSample, int numSamples) override;

    //==========================================================================
    // Oscillator settings
    void setOsc1WaveType(PadWaveType type);
    void setOsc2WaveType(PadWaveType type);
    void setOsc2Detune(float cents);
    void setOscMix(float mix); // 0 = osc1 pair, 1 = osc2 pair

    //==========================================================================
    // Filter settings
    void setFilterCutoff(float frequency);
    void setFilterResonance(float resonance);
    void setFilterType(PadFilterType type);
    void setFilterEnvAmount(float amount);
    void setFilterEnvelope(float attack, float decay, float sustain, float release);

    //==========================================================================
    // Chorus settings
    void setChorusRate(float rate);
    void setChorusDepth(float depth);
    void setChorusWet(float wet);

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Oscillator implementation
    struct Oscillator
    {
        double phase = 0.0;
        PadWaveType waveType = PadWaveType::Sawtooth;
        float detuneCents = 0.0f;
        float level = 0.5f; // Individual gain for crossfade

        float generate(double frequency, double sampleRate);
        void reset() { phase = 0.0; }
    };

    // 4 oscillators for rich pad sound
    // osc[0], osc[2] = osc1 type
    // osc[1], osc[3] = osc2 type
    Oscillator osc[4];
    float oscMix = 0.5f; // Crossfade between osc1 and osc2 pairs

    // Filter - using StateVariableTPT
    juce::dsp::StateVariableTPTFilter<float> filter;
    float filterCutoff = 2000.0f;
    float filterResonance = 0.5f;
    PadFilterType filterType = PadFilterType::LowPass;
    float filterEnvAmount = 2000.0f;

    // Filter envelope
    juce::ADSR filterEnvelope;
    juce::ADSR::Parameters filterEnvParams{0.3f, 0.5f, 0.5f, 1.0f};

    // Per-voice chorus
    juce::dsp::Chorus<float> chorus;
    float chorusRate = 0.8f;
    float chorusDepth = 0.7f;
    float chorusWet = 0.2f; // Per-voice is lower than master

    // Helper to calculate oscillator frequency with detune
    double getOscFrequency(const Oscillator& oscillator, float baseFreq);

    // Generate waveform sample
    static float generateWave(PadWaveType type, double phase);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyPadSynthVoice)
};

//==============================================================================

/**
 * PolyPadSynth - Lush polyphonic pad synthesizer
 *
 * Features:
 * - 4 detuned oscillators with crossfade mixing
 * - Multi-mode filter with envelope modulation
 * - Per-voice chorus + master chorus for rich ensemble sound
 * - Long attack/release envelopes for pad sounds
 * - Voice stealing with up to 8 voices
 * - 18 factory presets
 */
class PolyPadSynth : public SynthBase
{
public:
    PolyPadSynth();
    ~PolyPadSynth() override;

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
    std::array<std::unique_ptr<PolyPadSynthVoice>, MAX_VOICES> voices;
    int voiceRoundRobin = 0;

    // Master chorus for ensemble effect
    juce::dsp::Chorus<float> masterChorus;

    // Initialize all parameters
    void initializeParameters();

    // Update all voice parameters
    void updateVoiceParameters();

    // Voice allocation
    PolyPadSynthVoice* findFreeVoice();
    PolyPadSynthVoice* findVoiceToSteal();
    PolyPadSynthVoice* findVoicePlayingNote(int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PolyPadSynth)
};
