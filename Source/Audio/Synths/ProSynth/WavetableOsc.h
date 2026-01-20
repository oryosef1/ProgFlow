#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

/**
 * Wavetable data structure - contains multiple frames for morphing
 */
struct Wavetable
{
    juce::String id;
    juce::String name;
    juce::String category; // Basic, Analog, Digital, Vocal, Pads, Bass, FX
    std::vector<std::vector<float>> frames;

    static constexpr int WAVETABLE_SIZE = 2048;
};

/**
 * WavetableOsc - Wavetable oscillator with frame morphing
 *
 * Features:
 * - Multiple wavetable frames with smooth morphing
 * - Position control (0-1) for frame interpolation
 * - Efficient per-sample lookup
 * - Support for built-in and user wavetables
 */
class WavetableOsc
{
public:
    WavetableOsc();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Wavetable management
    void setWavetableById(const juce::String& id);
    void setWavetable(const Wavetable& wavetable);
    const Wavetable* getCurrentWavetable() const { return currentWavetable; }

    //==========================================================================
    // Position control (morphs between frames)
    void setPosition(float position); // 0-1
    float getPosition() const { return position; }

    //==========================================================================
    // Oscillator control
    void setFrequency(float frequency);
    float getFrequency() const { return frequency; }

    void setLevel(float level);
    float getLevel() const { return level; }

    //==========================================================================
    // Rendering
    float processSample();
    void start();
    void stop();
    bool isPlaying() const { return playing; }

    //==========================================================================
    // Built-in wavetables
    static std::vector<Wavetable> getBuiltInWavetables();

private:
    const Wavetable* currentWavetable = nullptr;
    float position = 0.0f; // 0-1 for frame morphing
    float frequency = 440.0f;
    float level = 1.0f;
    bool playing = false;

    double phase = 0.0;
    double sampleRate = 44100.0;

    // Current interpolated frame (cached)
    std::vector<float> interpolatedFrame;
    void updateInterpolatedFrame();

    // Built-in wavetables (static, shared)
    static std::vector<Wavetable> builtInWavetables;
    static void initializeBuiltInWavetables();
    static bool wavetablesInitialized;

    // Wavetable generation helpers
    static std::vector<float> generateWaveform(const juce::String& type, float param = 0.5f);
    static std::vector<float> generateHarmonics(const std::vector<int>& harmonics,
                                                const std::vector<float>& amplitudes);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableOsc)
};
