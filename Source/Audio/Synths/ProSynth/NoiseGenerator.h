#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

enum class NoiseType
{
    White = 0,
    Pink,
    Brown
};

enum class NoiseFilterType
{
    LowPass = 0,
    HighPass,
    BandPass
};

/**
 * NoiseGenerator - Noise source with optional filtering
 *
 * Features:
 * - White, pink, and brown noise
 * - Optional filter
 * - Level control
 */
class NoiseGenerator
{
public:
    NoiseGenerator();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Noise settings
    void setNoiseType(NoiseType type);
    NoiseType getNoiseType() const { return noiseType; }

    void setLevel(float level);
    float getLevel() const { return level; }

    //==========================================================================
    // Filter settings
    void setFilterEnabled(bool enabled);
    bool isFilterEnabled() const { return filterEnabled; }

    void setFilterType(NoiseFilterType type);
    NoiseFilterType getFilterType() const { return filterType; }

    void setFilterCutoff(float hz);
    float getFilterCutoff() const { return filterCutoff; }

    void setFilterResonance(float resonance);
    float getFilterResonance() const { return filterResonance; }

    //==========================================================================
    // Playback
    void trigger();
    void release();
    bool isPlaying() const { return playing; }

    //==========================================================================
    // Processing
    float processSample();

private:
    NoiseType noiseType = NoiseType::White;
    float level = 0.0f;
    bool playing = false;

    bool filterEnabled = false;
    NoiseFilterType filterType = NoiseFilterType::LowPass;
    float filterCutoff = 2000.0f;
    float filterResonance = 0.0f;

    // Filter
    juce::dsp::StateVariableTPTFilter<float> filter;

    // Pink noise state
    float pinkState[7] = {0};

    // Brown noise state
    float brownState = 0.0f;

    double sampleRate = 44100.0;

    float generateNoise();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGenerator)
};
