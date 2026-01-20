#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

/**
 * LFO waveform shapes
 */
enum class LFOShape
{
    Sine = 0,
    Triangle,
    Saw,
    Square,
    SampleHold,
    Random
};

/**
 * BPM sync note values
 */
enum class LFOSyncValue
{
    Whole = 0,      // 1/1
    Half,           // 1/2
    Quarter,        // 1/4
    Eighth,         // 1/8
    Sixteenth,      // 1/16
    ThirtySecond,   // 1/32
    HalfTriplet,    // 1/2T
    QuarterTriplet, // 1/4T
    EighthTriplet,  // 1/8T
    QuarterDotted,  // 1/4D
    EighthDotted    // 1/8D
};

/**
 * ProSynthLFO - Low Frequency Oscillator with BPM sync
 *
 * Features:
 * - 6 waveform shapes
 * - Free-running or BPM-synced
 * - Phase offset
 * - Delay/fade-in
 * - Retrigger on note-on
 */
class ProSynthLFO
{
public:
    ProSynthLFO();

    void prepareToPlay(double sampleRate);
    void reset();

    //==========================================================================
    // Rate control
    void setRate(float hz); // 0.01 - 50 Hz
    float getRate() const { return rate; }

    //==========================================================================
    // Shape
    void setShape(LFOShape shape);
    LFOShape getShape() const { return shape; }

    //==========================================================================
    // Phase offset (0-360 degrees)
    void setPhase(float degrees);
    float getPhase() const { return phaseOffset; }

    //==========================================================================
    // Delay/fade-in (0-5 seconds)
    void setDelay(float seconds);
    float getDelay() const { return delayTime; }

    //==========================================================================
    // Retrigger
    void setRetrigger(bool enabled) { retrigger = enabled; }
    bool isRetriggerEnabled() const { return retrigger; }
    void triggerRetrigger();

    //==========================================================================
    // BPM sync
    void setSync(bool enabled, LFOSyncValue syncValue = LFOSyncValue::Quarter);
    bool isSynced() const { return synced; }
    LFOSyncValue getSyncValue() const { return syncValue; }
    void setBPM(float bpm);

    //==========================================================================
    // Output range
    void setRange(float min, float max);
    float getMin() const { return minValue; }
    float getMax() const { return maxValue; }

    //==========================================================================
    // Processing
    void start();
    void stop();
    bool isRunning() const { return running; }
    float processSample();

private:
    LFOShape shape = LFOShape::Sine;
    float rate = 1.0f;
    float phaseOffset = 0.0f;
    float delayTime = 0.0f;
    bool retrigger = false;
    bool synced = false;
    LFOSyncValue syncValue = LFOSyncValue::Quarter;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    bool running = false;

    double phase = 0.0;
    double sampleRate = 44100.0;
    float bpm = 120.0f;

    // Delay envelope
    float delayEnvelope = 1.0f;
    float delayCounter = 0.0f;

    // For sample & hold / random
    float sampleHoldValue = 0.0f;
    float sampleHoldCounter = 0.0f;
    float randomTargetValue = 0.0f;
    float randomCurrentValue = 0.0f;

    // Generate waveform sample
    float generateSample(LFOShape waveShape, double phaseValue);

    // Calculate effective rate (considering BPM sync)
    float getEffectiveRate() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynthLFO)
};
