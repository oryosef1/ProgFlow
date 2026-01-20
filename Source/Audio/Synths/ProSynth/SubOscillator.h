#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

enum class SubOscWaveform
{
    Sine = 0,
    Triangle,
    Square
};

/**
 * SubOscillator - Simple sub-bass oscillator
 *
 * Features:
 * - One or two octaves below main pitch
 * - Simple waveforms for low-end weight
 * - Level control
 */
class SubOscillator
{
public:
    SubOscillator();

    void prepareToPlay(double sampleRate);
    void reset();

    //==========================================================================
    // Settings
    void setOctave(int octave); // -1 or -2
    int getOctave() const { return octave; }

    void setWaveform(SubOscWaveform waveform);
    SubOscWaveform getWaveform() const { return waveform; }

    void setLevel(float level);
    float getLevel() const { return level; }

    //==========================================================================
    // Playback
    void trigger(float baseFrequency);
    void release();
    bool isPlaying() const { return playing; }

    //==========================================================================
    // Processing
    float processSample();

private:
    SubOscWaveform waveform = SubOscWaveform::Sine;
    int octave = -1;
    float level = 0.0f;
    bool playing = false;

    double phase = 0.0;
    double frequency = 0.0;
    double sampleRate = 44100.0;

    float generateSample();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubOscillator)
};
