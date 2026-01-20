#pragma once

#include "SynthBase.h"
#include "SynthVoice.h"
#include <juce_dsp/juce_dsp.h>

/**
 * FM Algorithm descriptions (3-operator):
 * 1: mod2 → mod1 → carrier (serial chain - metallic, harsh)
 * 2: (mod1 + mod2) → carrier (parallel mods - rich harmonics)
 * 3: mod1 → carrier, mod2 as 2nd voice (dual voice)
 * 4: mod2 → mod1 → carrier + mod2 direct (Y-shape - bright)
 * 5: mod1 → carrier + mod2 output (carrier + additive)
 * 6: mod1 → mod2 → carrier (serial, reversed - different timbre)
 * 7: mod1 → carrier + mod2 → carrier (parallel to carrier)
 * 8: carrier + mod1 + mod2 (all outputs, pure additive)
 */
enum class FMAlgorithm
{
    Serial_2_1_C = 1,      // mod2 → mod1 → carrier
    Parallel_12_C = 2,     // (mod1 + mod2) → carrier
    Dual_1C_2 = 3,         // mod1 → carrier, mod2 voice
    YShape_21C_2 = 4,      // mod2 → mod1 → carrier + mod2
    Split_1C_2 = 5,        // mod1 → carrier + mod2
    Serial_1_2_C = 6,      // mod1 → mod2 → carrier
    Parallel_1C_2C = 7,    // mod1 → carrier + mod2 → carrier
    Additive_C_1_2 = 8     // carrier + mod1 + mod2
};

/**
 * FMSynthVoice - A single voice for FM synthesis
 *
 * Signal chain per voice (depends on algorithm):
 * - 3 sine wave operators (carrier, modulator1, modulator2)
 * - Each operator has its own envelope
 * - Operators modulate each other based on algorithm
 * - Modulation depth controlled by index parameters
 */
class FMSynthVoice : public SynthVoice
{
public:
    FMSynthVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                         int startSample, int numSamples) override;

    //==========================================================================
    // FM Parameters (set by parent synth)
    void setAlgorithm(FMAlgorithm algorithm);
    void setCarrierRatio(float ratio);
    void setMod1Ratio(float ratio);
    void setMod2Ratio(float ratio);
    void setMod1Index(float index);
    void setMod2Index(float index);
    void setFeedback(float feedback);

    //==========================================================================
    // Operator envelopes
    void setModEnvelope1(float attack, float decay, float sustain, float release);
    void setModEnvelope2(float attack, float decay, float sustain, float release);

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Oscillators - simple phase-based sine wave generators
    struct SineOscillator
    {
        double phase = 0.0;
        float ratio = 1.0f;  // Frequency ratio vs base note

        float process(double baseFreq, double sr, float modulation = 0.0f);
        void reset() { phase = 0.0; }
    };

    SineOscillator carrier;
    SineOscillator modulator1;
    SineOscillator modulator2;

    // Envelopes for modulators (carrier uses ampEnvelope from SynthVoice)
    juce::ADSR modEnv1;
    juce::ADSR modEnv2;
    juce::ADSR::Parameters modEnv1Params{0.01f, 0.3f, 0.3f, 0.2f};
    juce::ADSR::Parameters modEnv2Params{0.01f, 0.5f, 0.2f, 0.3f};

    // FM parameters
    FMAlgorithm algorithm = FMAlgorithm::Serial_2_1_C;
    float mod1Index = 5.0f;
    float mod2Index = 2.0f;
    float feedback = 0.0f;
    float feedbackSample = 0.0f; // Previous carrier output for feedback

    // Output mixing (algorithm-dependent)
    float carrierGain = 1.0f;
    float mod1OutputGain = 0.0f;
    float mod2OutputGain = 0.0f;

    // Process single sample based on algorithm
    float processSample(double baseFreq);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynthVoice)
};

//==============================================================================

/**
 * FMSynth - 3-operator FM synthesizer
 *
 * Features:
 * - 3 sine wave operators (carrier, modulator1, modulator2)
 * - 8 FM algorithms
 * - Independent envelopes for each operator
 * - Modulation index (depth) controls
 * - Carrier feedback
 * - 24 factory presets (keys, bass, lead, pad, pluck, brass/wind)
 */
class FMSynth : public SynthBase
{
public:
    FMSynth();
    ~FMSynth() override;

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
    std::array<std::unique_ptr<FMSynthVoice>, MAX_VOICES> voices;

    // Initialize all parameters
    void initializeParameters();

    // Update all voice parameters
    void updateVoiceParameters();

    // Voice allocation
    FMSynthVoice* findFreeVoice();
    FMSynthVoice* findVoiceToSteal();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FMSynth)
};
