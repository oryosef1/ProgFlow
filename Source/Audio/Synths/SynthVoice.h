#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Voice state for voice allocation
 */
enum class VoiceState
{
    Idle,      // Voice is free
    Attack,    // Note just triggered, in attack phase
    Sustain,   // Note held, in sustain
    Release    // Note released, in release phase
};

/**
 * SynthVoice - Base class for a single polyphonic voice
 *
 * Each voice handles:
 * - A single note with velocity
 * - Amplitude envelope (ADSR)
 * - Portamento/glide between notes
 *
 * Subclasses add oscillators, filters, and other DSP
 */
class SynthVoice
{
public:
    SynthVoice();
    virtual ~SynthVoice() = default;

    //==========================================================================
    // Lifecycle
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);
    virtual void reset();

    //==========================================================================
    // Note control
    virtual void startNote(int midiNote, float velocity, bool legato = false);
    virtual void stopNote(bool allowTailOff = true);
    virtual void killNote(); // Hard stop, no release

    //==========================================================================
    // Audio processing
    virtual void renderNextBlock(juce::AudioBuffer<float>& buffer,
                                 int startSample, int numSamples) = 0;

    //==========================================================================
    // State queries
    VoiceState getState() const { return state; }
    bool isActive() const { return state != VoiceState::Idle; }
    bool isPlaying() const { return state == VoiceState::Attack || state == VoiceState::Sustain; }

    int getCurrentNote() const { return currentNote; }
    float getCurrentVelocity() const { return velocity; }

    // For voice stealing priority
    float getAge() const { return age; }
    void incrementAge(int samples) { age += static_cast<float>(samples) / static_cast<float>(sampleRate); }

    //==========================================================================
    // Envelope settings
    void setAmpEnvelope(float attack, float decay, float sustain, float release);

    //==========================================================================
    // Portamento
    void setPortamentoTime(float timeInSeconds);
    float getPortamentoTime() const { return portamentoTime; }

protected:
    // Current note state
    int currentNote = -1;
    int previousNote = -1;
    float velocity = 0.0f;
    VoiceState state = VoiceState::Idle;
    float age = 0.0f; // Time since note started, for voice stealing

    // Amp envelope
    juce::ADSR ampEnvelope;
    juce::ADSR::Parameters ampEnvParams{0.01f, 0.1f, 0.7f, 0.3f};

    // Portamento
    float portamentoTime = 0.0f; // 0 = no glide
    float currentFrequency = 440.0f;
    float targetFrequency = 440.0f;
    float portamentoRate = 0.0f; // Frequency change per sample

    // Audio settings
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Helper: get frequency with portamento applied
    float getNextFrequency();

    // Called after note starts - override to reset oscillators etc.
    virtual void onNoteStart() {}

    // Called after note stops (release starts)
    virtual void onNoteStop() {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthVoice)
};
