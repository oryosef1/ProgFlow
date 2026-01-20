#include "AutomationRecorder.h"

AutomationRecorder::AutomationRecorder(Track& track)
    : track(track)
{
}

void AutomationRecorder::onControlTouchStart(const juce::String& parameterId)
{
    auto& state = recordingStates[parameterId];
    state.isTouching = true;

    // In Latch mode, touching starts the latch
    if (track.getAutomationMode() == AutomationMode::Latch)
    {
        state.hasLatched = true;
    }
}

void AutomationRecorder::onControlTouchEnd(const juce::String& parameterId)
{
    auto& state = recordingStates[parameterId];
    state.isTouching = false;

    // In Touch mode, we stop recording when released
    // In Latch mode, we keep recording until playback stops
}

void AutomationRecorder::onParameterChanged(const juce::String& parameterId, float normalizedValue)
{
    auto mode = track.getAutomationMode();

    // Only record in Write, Touch, or Latch modes
    if (mode == AutomationMode::Off || mode == AutomationMode::Read)
        return;

    auto& state = recordingStates[parameterId];
    state.lastValue = normalizedValue;

    // Record immediately if we're playing and conditions are met
    maybeRecordPoint(parameterId, normalizedValue, currentPosition);
}

void AutomationRecorder::process(double positionInBeats, bool isPlaying)
{
    currentPosition = positionInBeats;

    // Reset latch states when stopping
    if (wasPlaying && !isPlaying)
    {
        for (auto& [paramId, state] : recordingStates)
        {
            state.hasLatched = false;
            state.lastRecordedBeat = -1.0;
        }
    }

    wasPlaying = isPlaying;
}

void AutomationRecorder::reset()
{
    recordingStates.clear();
    currentPosition = 0.0;
    wasPlaying = false;
}

void AutomationRecorder::maybeRecordPoint(const juce::String& parameterId,
                                           float value,
                                           double position)
{
    auto mode = track.getAutomationMode();

    // Not recording
    if (mode == AutomationMode::Off || mode == AutomationMode::Read)
        return;

    auto& state = recordingStates[parameterId];

    // Check if we should record based on mode
    bool shouldRecord = false;

    switch (mode)
    {
        case AutomationMode::Write:
            // Always record while playing
            shouldRecord = true;
            break;

        case AutomationMode::Touch:
            // Only record while control is being touched
            shouldRecord = state.isTouching;
            break;

        case AutomationMode::Latch:
            // Record while touching OR after touch (latched)
            shouldRecord = state.isTouching || state.hasLatched;
            break;

        default:
            break;
    }

    if (!shouldRecord)
        return;

    // Check minimum interval to avoid flooding
    if (state.lastRecordedBeat >= 0 &&
        (position - state.lastRecordedBeat) < MIN_POINT_INTERVAL)
    {
        return;
    }

    // Get or create the automation lane
    auto* lane = track.getOrCreateAutomationLane(parameterId);
    if (!lane)
        return;

    // Add the point
    lane->addPoint(position, value, CurveType::Linear);
    state.lastRecordedBeat = position;

    // Notify listeners
    if (onPointRecorded)
        onPointRecorded(parameterId, position, value);
}
