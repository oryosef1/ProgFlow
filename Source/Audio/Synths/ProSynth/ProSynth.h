#pragma once

#include "../SynthBase.h"
#include "../SynthVoice.h"
#include "WavetableOsc.h"
#include "ProSynthFilter.h"
#include "ProSynthLFO.h"
#include "ModMatrix.h"
#include "SubOscillator.h"
#include "NoiseGenerator.h"
#include "UnisonEngine.h"
#include <juce_dsp/juce_dsp.h>
#include <array>

/**
 * Basic waveform types for ProSynth oscillators
 * (named differently to avoid conflict with AnalogSynth WaveType)
 */
enum class ProWaveType
{
    Sine = 0,
    Triangle,
    Sawtooth,
    Square
};

/**
 * Oscillator modes for ProSynth
 */
enum class ProOscMode
{
    Basic = 0,     // Standard waveforms (saw, square, sine, triangle)
    Wavetable,     // Wavetable synthesis with morphing
    FM             // FM synthesis (2-operator)
};

/**
 * Filter routing modes
 */
enum class FilterRouting
{
    Serial = 0,    // Filter1 -> Filter2
    Parallel,      // Filter1 + Filter2 mixed
    Split          // Osc1+2 -> Filter1, Osc3 -> Filter2
};

/**
 * ProSynthVoice - A single voice for ProSynth
 *
 * Signal chain:
 * 3 Oscillators (Basic/WT/FM) + Sub + Noise
 *   → Filter1 → Filter2 (routing dependent)
 *     → Amp Envelope → Output
 */
class ProSynthVoice : public SynthVoice
{
public:
    ProSynthVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override;

    //==========================================================================
    // Oscillator settings (set by parent synth)
    struct OscSettings
    {
        bool enabled = true;
        ProOscMode mode = ProOscMode::Basic;
        ProWaveType basicWave = ProWaveType::Sawtooth;
        juce::String wavetableId = "wt-basic-saw";
        float wtPosition = 0.0f;
        float fmRatio = 2.0f;
        float fmDepth = 0.5f;
        float level = 1.0f;
        float pan = 0.0f;
        int octave = 0;
        int semi = 0;
        float fine = 0.0f;
    };

    void setOscSettings(int oscIndex, const OscSettings& settings);

    //==========================================================================
    // Sub oscillator
    void setSubOscSettings(bool enabled, SubOscWaveform wave, int octave, float level);

    //==========================================================================
    // Noise generator
    void setNoiseSettings(bool enabled, NoiseType type, float level,
                         bool filterEnabled, NoiseFilterType filterType,
                         float filterCutoff, float filterResonance);

    //==========================================================================
    // Filter settings
    void setFilter1(ProFilterModel model, ProFilterType type, float cutoff,
                   float resonance, float drive, float keytrack);
    void setFilter2(bool enabled, ProFilterModel model, ProFilterType type,
                   float cutoff, float resonance, float drive);
    void setFilterRouting(FilterRouting routing);
    void setFilterEnvelope(float attack, float decay, float sustain, float release, float amount);

    //==========================================================================
    // Unison detune (set per-voice for unison spread)
    void setUnisonDetune(float cents) { unisonDetuneCents = cents; }
    float getUnisonDetune() const { return unisonDetuneCents; }

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Oscillators
    struct Oscillator
    {
        ProOscMode mode = ProOscMode::Basic;
        bool enabled = true;

        // Basic mode
        double phase = 0.0;
        ProWaveType basicWave = ProWaveType::Sawtooth;

        // Wavetable mode
        WavetableOsc wavetableOsc;

        // FM mode
        double fmCarrierPhase = 0.0;
        double fmModulatorPhase = 0.0;
        float fmRatio = 2.0f;
        float fmDepth = 0.5f;

        // Common settings
        float level = 1.0f;
        float pan = 0.0f;
        int octave = 0;
        int semi = 0;
        float fine = 0.0f;

        void reset();
        float process(double baseFrequency, double sampleRate);
    };

    std::array<Oscillator, 3> oscillators;
    SubOscillator subOsc;
    NoiseGenerator noiseGen;

    // Filters
    ProSynthFilter filter1;
    ProSynthFilter filter2;
    bool filter2Enabled = false;
    FilterRouting filterRouting = FilterRouting::Serial;
    float filterKeytrack = 0.0f;

    // Envelopes
    juce::ADSR filterEnvelope;
    juce::ADSR::Parameters filterEnvParams{0.01f, 0.3f, 0.5f, 0.5f};
    float filterEnvAmount = 0.0f;

    // Unison detune (in cents)
    float unisonDetuneCents = 0.0f;

    // Helper methods
    float calculateOscFrequency(const Oscillator& osc, float baseFreq);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynthVoice)
};

//==============================================================================

/**
 * ProSynth - Professional-grade polyphonic synthesizer
 *
 * Features:
 * - 3 oscillators (Basic/Wavetable/FM modes)
 * - Sub oscillator + Noise generator
 * - Dual filters with multiple models and routing
 * - 4 LFOs with BPM sync
 * - Modulation matrix (16 slots)
 * - Unison (up to 16 voices per note)
 * - Built-in effects (distortion, chorus, delay)
 * - 4 performance macros
 * - 210+ presets
 * - 16-voice polyphony
 */
class ProSynth : public SynthBase
{
public:
    ProSynth();
    ~ProSynth() override;

    //==========================================================================
    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
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

    //==========================================================================
    // Access to subsystems (for UI)
    ModMatrix& getModMatrix() { return modMatrix; }
    const ModMatrix& getModMatrix() const { return modMatrix; }

    static constexpr int MAX_VOICES = 16;

protected:
    void onParameterChanged(const juce::String& name, float value) override;
    void onParameterEnumChanged(const juce::String& name, int index) override;

private:
    // Voice pool
    std::array<std::unique_ptr<ProSynthVoice>, MAX_VOICES> voices;

    // Modulation system
    std::array<ProSynthLFO, 4> lfos;
    ModMatrix modMatrix;

    // Unison
    UnisonEngine unisonEngine;

    // Built-in effects
    juce::dsp::WaveShaper<float> distortion;
    juce::dsp::Chorus<float> chorus;
    juce::dsp::DelayLine<float> delayLine;
    float delayFeedback = 0.3f;
    float delayMix = 0.0f;

    // Macro controls
    std::array<float, 4> macros = {0.0f, 0.0f, 0.0f, 0.0f};

    // Initialize all parameters
    void initializeParameters();

    // Update voice parameters
    void updateVoiceParameters();

    // Voice allocation
    ProSynthVoice* findFreeVoice();
    ProSynthVoice* findVoiceToSteal();

    // Built-in effects processing
    void processEffects(juce::AudioBuffer<float>& buffer);

    // Generate saturation curve for distortion
    static std::vector<float> generateDistortionCurve(float drive);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynth)
};
