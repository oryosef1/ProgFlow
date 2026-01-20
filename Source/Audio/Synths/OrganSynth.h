#pragma once

#include "SynthBase.h"
#include "SynthVoice.h"
#include <juce_dsp/juce_dsp.h>

/**
 * Percussion harmonic settings
 */
enum class PercussionHarmonic
{
    Second = 0,
    Third
};

/**
 * Percussion decay settings
 */
enum class PercussionDecay
{
    Fast = 0,
    Slow
};

/**
 * Rotary speaker speed settings
 */
enum class RotarySpeed
{
    Off = 0,
    Slow,
    Fast
};

/**
 * OrganSynthVoice - A single voice for the organ synth
 *
 * Signal chain per voice:
 * 9 Drawbar Oscs → Mix → Percussion → Click → Master Gain → Output
 *                         (with envelopes)
 */
class OrganSynthVoice : public SynthVoice
{
public:
    OrganSynthVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                        int startSample, int numSamples) override;

    //==========================================================================
    // Drawbar settings (set by parent synth)
    void setDrawbarLevel(int drawbarIndex, float level);

    //==========================================================================
    // Percussion settings
    void setPercussionAmount(float amount);
    void setPercussionHarmonic(PercussionHarmonic harmonic);
    void setPercussionDecay(PercussionDecay decay);

    //==========================================================================
    // Key click
    void setKeyClickAmount(float amount);

    //==========================================================================
    // Master volume
    void setMasterVolume(float volume);

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Drawbar ratios (16', 5-1/3', 8', 4', 2-2/3', 2', 1-3/5', 1-1/3', 1')
    static constexpr float DRAWBAR_RATIOS[9] = {0.5f, 1.5f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 8.0f};
    static constexpr int NUM_DRAWBARS = 9;

    // Drawbar oscillators (simple phase accumulators for sine waves)
    struct DrawbarOscillator
    {
        double phase = 0.0;
        float level = 0.0f;
        float ratio = 1.0f;

        float generate(double baseFrequency, double sampleRate);
        void reset() { phase = 0.0; }
    };

    DrawbarOscillator drawbars[NUM_DRAWBARS];

    // Percussion
    double percPhase = 0.0;
    juce::ADSR percEnvelope;
    juce::ADSR::Parameters percEnvParams{0.001f, 0.2f, 0.0f, 0.001f};
    float percAmount = 0.0f;
    PercussionHarmonic percHarmonic = PercussionHarmonic::Third;

    // Key click (simple noise burst)
    juce::Random random;
    juce::ADSR clickEnvelope;
    juce::ADSR::Parameters clickEnvParams{0.001f, 0.01f, 0.0f, 0.001f};
    float keyClickAmount = 0.0f;
    juce::dsp::IIR::Filter<float> clickFilter; // Low-pass for noise

    // Master volume
    float masterVolume = 0.6f;

    // Helper to generate percussion oscillator
    float generatePercussion(double baseFrequency);

    // Helper to generate key click
    float generateKeyClick();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganSynthVoice)
};

//==============================================================================

/**
 * OrganSynth - Tonewheel organ synthesizer (Hammond-style)
 *
 * Features:
 * - 9 drawbars with harmonic additive synthesis
 * - Percussion (2nd or 3rd harmonic with fast/slow decay)
 * - Key click (attack transient)
 * - Rotary speaker simulation (vibrato/tremolo)
 * - Drive/distortion
 * - 20 presets (Rock, Jazz, Gospel, Blues, etc.)
 */
class OrganSynth : public SynthBase
{
public:
    OrganSynth();
    ~OrganSynth() override;

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
    std::array<std::unique_ptr<OrganSynthVoice>, MAX_VOICES> voices;

    // Rotary speaker effect (Leslie simulation using vibrato)
    juce::dsp::Chorus<float> rotarySpeaker;
    float rotaryDepth = 0.5f;
    RotarySpeed rotarySpeed = RotarySpeed::Slow;

    // Drive/distortion
    juce::dsp::WaveShaper<float> distortion;
    float driveAmount = 0.1f;

    // Initialize all parameters
    void initializeParameters();

    // Update all voice parameters
    void updateVoiceParameters();

    // Voice allocation
    OrganSynthVoice* findFreeVoice();
    OrganSynthVoice* findVoiceToSteal();
    OrganSynthVoice* findVoicePlayingNote(int midiNote);

    // Helper to convert drawbar value (0-8) to gain
    static float drawbarToGain(int drawbarValue);

    // Setup distortion waveshaper
    void setupDistortion();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrganSynth)
};
