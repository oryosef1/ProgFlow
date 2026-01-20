#pragma once

#include "SynthBase.h"
#include "SynthVoice.h"
#include <juce_dsp/juce_dsp.h>

/**
 * StringSynthVoice - A single voice for the string synth
 *
 * Signal chain per voice:
 * Multiple detuned oscillators (ensemble) → Filter → AmpEnv → Output
 *                                             ↑
 *                                         FilterEnv
 *
 * Each voice creates multiple oscillators across different sections
 * (violins, violas, cellos, basses) with slight detuning for ensemble richness.
 */
class StringSynthVoice : public SynthVoice
{
public:
    StringSynthVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                         int startSample, int numSamples) override;

    //==========================================================================
    // Section levels (0-1 for each section)
    void setViolinsLevel(float level);
    void setViolasLevel(float level);
    void setCellosLevel(float level);
    void setBassesLevel(float level);

    //==========================================================================
    // Ensemble settings
    void setEnsembleVoices(int numVoices);
    void setEnsembleSpread(float cents);

    //==========================================================================
    // Filter settings
    void setFilterCutoff(float frequency);
    void setFilterResonance(float resonance);
    void setFilterEnvAmount(float amount);
    void setFilterEnvelope(float attack, float decay, float sustain, float release);

    //==========================================================================
    // Master volume
    void setMasterVolume(float volume);

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Oscillator - simple sawtooth for string ensemble
    struct Oscillator
    {
        double phase = 0.0;
        float level = 1.0f;
        float detuneCents = 0.0f;
        int octave = 0;

        float generate(double frequency, double sampleRate);
        void reset() { phase = 0.0; }
    };

    // Ensemble oscillators (dynamically allocated based on settings)
    std::vector<Oscillator> oscillators;

    // Section levels
    float violinsLevel = 1.0f;
    float violasLevel = 0.5f;
    float cellosLevel = 0.3f;
    float bassesLevel = 0.0f;

    // Ensemble settings
    int ensembleVoices = 4;
    float ensembleSpread = 15.0f;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;
    float filterCutoff = 3000.0f;
    float filterResonance = 0.1f;
    float filterEnvAmount = 2000.0f;

    // Filter envelope
    juce::ADSR filterEnvelope;
    juce::ADSR::Parameters filterEnvParams{0.8f, 0.5f, 0.4f, 1.5f};

    // Master volume
    float masterVolume = 0.5f;

    // Rebuild oscillator array when settings change
    void rebuildOscillators(float baseFrequency);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSynthVoice)
};

//==============================================================================

/**
 * StringSynth - Orchestral string ensemble synthesizer
 *
 * Features:
 * - 4 orchestral sections (violins, violas, cellos, basses) with independent levels
 * - Multiple detuned oscillators per section for realistic ensemble effect
 * - Lowpass filter with envelope for warmth and expression
 * - Built-in chorus and phaser effects for richness
 * - Slow attack and long release for realistic strings
 * - Up to 12 voices of polyphony
 */
class StringSynth : public SynthBase
{
public:
    StringSynth();
    ~StringSynth() override;

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

    static constexpr int MAX_VOICES = 12;

protected:
    void onParameterChanged(const juce::String& name, float value) override;

private:
    // Voice pool
    std::array<std::unique_ptr<StringSynthVoice>, MAX_VOICES> voices;
    int voiceRoundRobin = 0;

    // Built-in effects (chorus and phaser)
    juce::dsp::Chorus<float> chorus;
    juce::dsp::Phaser<float> phaser;

    // Effect processing chain
    juce::dsp::ProcessorChain<
        juce::dsp::Chorus<float>,
        juce::dsp::Phaser<float>
    > effectChain;

    // Initialize all parameters
    void initializeParameters();

    // Update all voice parameters
    void updateVoiceParameters();

    // Voice allocation
    StringSynthVoice* findFreeVoice();
    StringSynthVoice* findVoiceToSteal();
    StringSynthVoice* findVoicePlayingNote(int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringSynth)
};
