#pragma once

#include <juce_core/juce_core.h>
#include "Track.h"
#include "AutomationLane.h"
#include <map>
#include <functional>

/**
 * AutomationRecorder - Handles real-time recording of automation
 *
 * Supports three recording modes:
 * - Write: Records everything while playing (destructive)
 * - Touch: Records while control is touched, returns to existing curve on release
 * - Latch: Records while touching, then holds last value after release
 */
class AutomationRecorder
{
public:
    AutomationRecorder(Track& track);

    // Control touch events (called from UI when user interacts with control)
    void onControlTouchStart(const juce::String& parameterId);
    void onControlTouchEnd(const juce::String& parameterId);

    // Parameter change events (called when parameter value changes)
    void onParameterChanged(const juce::String& parameterId, float normalizedValue);

    // Called each audio block during playback
    void process(double positionInBeats, bool isPlaying);

    // Reset recording state (e.g., when stopping playback)
    void reset();

    // Get current position (for UI feedback)
    double getCurrentPosition() const { return currentPosition; }

    // Callback for when a point is recorded (for UI updates)
    std::function<void(const juce::String& parameterId, double time, float value)> onPointRecorded;

private:
    Track& track;

    // Recording state per parameter
    struct RecordingState
    {
        bool isTouching = false;
        bool hasLatched = false;
        float lastValue = 0.5f;
        double lastRecordedBeat = -1.0;
    };

    std::map<juce::String, RecordingState> recordingStates;

    // Current playback position
    double currentPosition = 0.0;
    bool wasPlaying = false;

    // Minimum interval between recorded points (to avoid flooding)
    static constexpr double MIN_POINT_INTERVAL = 0.0625;  // 1/16th beat

    // Record a point if conditions are met
    void maybeRecordPoint(const juce::String& parameterId, float value, double position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationRecorder)
};
