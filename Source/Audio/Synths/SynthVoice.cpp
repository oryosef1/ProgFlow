#include "SynthVoice.h"
#include "SynthBase.h"

SynthVoice::SynthVoice()
{
    ampEnvelope.setParameters(ampEnvParams);
}

void SynthVoice::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;
    ampEnvelope.setSampleRate(sampleRate);
    reset();
}

void SynthVoice::reset()
{
    ampEnvelope.reset();
    state = VoiceState::Idle;
    currentNote = -1;
    previousNote = -1;
    velocity = 0.0f;
    age = 0.0f;
    currentFrequency = 440.0f;
    targetFrequency = 440.0f;
}

void SynthVoice::startNote(int midiNote, float vel, bool legato)
{
    previousNote = currentNote;
    currentNote = midiNote;
    velocity = vel;
    age = 0.0f;

    targetFrequency = SynthBase::midiToFrequency(midiNote);

    // Handle portamento
    if (legato && portamentoTime > 0.0f && previousNote >= 0)
    {
        // Glide from previous note
        // currentFrequency stays where it was
        float frequencyDiff = targetFrequency - currentFrequency;
        float samplesForGlide = portamentoTime * static_cast<float>(sampleRate);
        portamentoRate = frequencyDiff / samplesForGlide;
    }
    else
    {
        // Jump to target immediately
        currentFrequency = targetFrequency;
        portamentoRate = 0.0f;
    }

    state = VoiceState::Attack;
    ampEnvelope.noteOn();

    onNoteStart();
}

void SynthVoice::stopNote(bool allowTailOff)
{
    if (allowTailOff)
    {
        // Start release phase
        state = VoiceState::Release;
        ampEnvelope.noteOff();
        onNoteStop();
    }
    else
    {
        // Hard stop
        killNote();
    }
}

void SynthVoice::killNote()
{
    ampEnvelope.reset();
    state = VoiceState::Idle;
    currentNote = -1;
    velocity = 0.0f;
    age = 0.0f;
}

void SynthVoice::setAmpEnvelope(float attack, float decay, float sustain, float release)
{
    ampEnvParams.attack = juce::jmax(0.001f, attack);
    ampEnvParams.decay = juce::jmax(0.001f, decay);
    ampEnvParams.sustain = juce::jlimit(0.0f, 1.0f, sustain);
    ampEnvParams.release = juce::jmax(0.001f, release);
    ampEnvelope.setParameters(ampEnvParams);
}

void SynthVoice::setPortamentoTime(float timeInSeconds)
{
    portamentoTime = juce::jmax(0.0f, timeInSeconds);
}

float SynthVoice::getNextFrequency()
{
    if (portamentoRate != 0.0f)
    {
        currentFrequency += portamentoRate;

        // Check if we've reached target
        if ((portamentoRate > 0 && currentFrequency >= targetFrequency) ||
            (portamentoRate < 0 && currentFrequency <= targetFrequency))
        {
            currentFrequency = targetFrequency;
            portamentoRate = 0.0f;
        }
    }

    return currentFrequency;
}
