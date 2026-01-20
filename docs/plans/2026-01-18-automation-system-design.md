# Automation System Design

**Date:** 2026-01-18
**Status:** Approved
**Scope:** J6.4 Automation System

## Overview

Add automation support to ProgFlow-JUCE, allowing users to record and draw parameter changes over time. This enables dynamic mixing (volume/pan sweeps) and musical expression (filter sweeps, synth parameter modulation).

## Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Automatable parameters | Track + Synth | Best bang for buck; effects can be added later |
| Interpolation | Linear + Hold | Covers smooth sweeps and instant jumps |
| UI location | Below each track | Ableton-style, most intuitive |
| Creation method | Draw + Record (all modes) | Full DAW capability |
| Storage | Per-track | Simpler, automation deleted with track |

## Data Model

### AutomationPoint

```cpp
enum class CurveType { Linear, Hold };

struct AutomationPoint {
    double timeInBeats;      // Position on timeline
    float value;             // 0.0-1.0 normalized
    CurveType curve;         // Interpolation to next point
};
```

### AutomationLane

```cpp
class AutomationLane {
public:
    AutomationLane(const juce::String& parameterId);

    const juce::String& getParameterId() const;

    // Point management
    void addPoint(double timeInBeats, float value, CurveType curve = CurveType::Linear);
    void removePoint(int index);
    void movePoint(int index, double newTime, float newValue);
    void setPointCurve(int index, CurveType curve);

    // Lookup
    float getValueAtTime(double timeInBeats) const;
    int getPointIndexAt(double timeInBeats, double tolerance) const;

    // Access
    const std::vector<AutomationPoint>& getPoints() const;
    int getNumPoints() const;

    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<AutomationLane> fromVar(const juce::var& v);

private:
    juce::String parameterId;
    std::vector<AutomationPoint> points;  // Always sorted by time

    void sortPoints();
};
```

### AutomationMode

```cpp
enum class AutomationMode {
    Off,    // Automation ignored
    Read,   // Playback only
    Write,  // Record everything (destructive)
    Touch,  // Record while touching, snap back on release
    Latch   // Record while touching, hold value after release
};
```

### Track Extensions

```cpp
class Track {
    // ... existing members ...

    // Automation
    std::vector<std::unique_ptr<AutomationLane>> automationLanes;
    AutomationMode automationMode = AutomationMode::Read;

    // Automation API
    AutomationLane* getAutomationLane(const juce::String& parameterId);
    AutomationLane* getOrCreateAutomationLane(const juce::String& parameterId);
    void removeAutomationLane(const juce::String& parameterId);
    const std::vector<std::unique_ptr<AutomationLane>>& getAutomationLanes() const;

    void setAutomationMode(AutomationMode mode);
    AutomationMode getAutomationMode() const;

    // Get list of automatable parameters
    std::vector<juce::String> getAutomatableParameters() const;
};
```

## Automatable Parameters

### Track Parameters

| Parameter ID | Range | Description |
|--------------|-------|-------------|
| `volume` | 0.0-1.0 → 0.0-2.0 | Track volume (normalized to actual) |
| `pan` | 0.0-1.0 → -1.0-1.0 | Track pan (normalized to actual) |

### Synth Parameters

All parameters registered via `SynthBase::addParameter()` are automatable. Parameter IDs are prefixed with `synth.`:

| Parameter ID | Example |
|--------------|---------|
| `synth.filter_cutoff` | Filter cutoff frequency |
| `synth.filter_resonance` | Filter resonance |
| `synth.osc1_detune` | Oscillator 1 detune |
| `synth.attack` | Amp envelope attack |

## Audio Thread Integration

### Automation Playback

```cpp
void Track::processBlock(juce::AudioBuffer<float>& buffer, int numSamples,
                         double positionInBeats, double bpm) {
    // Apply automation at block start
    if (automationMode == AutomationMode::Read ||
        automationMode == AutomationMode::Touch ||
        automationMode == AutomationMode::Latch) {
        applyAutomation(positionInBeats);
    }

    // ... existing processing ...
}

void Track::applyAutomation(double positionInBeats) {
    for (const auto& lane : automationLanes) {
        if (lane->getNumPoints() == 0) continue;

        float normalizedValue = lane->getValueAtTime(positionInBeats);
        const auto& paramId = lane->getParameterId();

        if (paramId == "volume") {
            setVolume(normalizedValue * 2.0f);
        }
        else if (paramId == "pan") {
            setPan(normalizedValue * 2.0f - 1.0f);
        }
        else if (paramId.startsWith("synth.") && synth != nullptr) {
            auto synthParamId = paramId.substring(6);
            auto* paramInfo = synth->getParameterInfo(synthParamId);
            if (paramInfo != nullptr) {
                float actualValue = paramInfo->minValue +
                    normalizedValue * (paramInfo->maxValue - paramInfo->minValue);
                synth->setParameter(synthParamId, actualValue);
            }
        }
    }
}
```

### Interpolation

```cpp
float AutomationLane::getValueAtTime(double timeInBeats) const {
    if (points.empty()) return 0.5f;  // Default to center

    // Before first point
    if (timeInBeats <= points.front().timeInBeats) {
        return points.front().value;
    }

    // After last point
    if (timeInBeats >= points.back().timeInBeats) {
        return points.back().value;
    }

    // Find surrounding points
    for (size_t i = 0; i < points.size() - 1; ++i) {
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];

        if (timeInBeats >= p1.timeInBeats && timeInBeats < p2.timeInBeats) {
            if (p1.curve == CurveType::Hold) {
                return p1.value;  // Step/hold
            }

            // Linear interpolation
            double t = (timeInBeats - p1.timeInBeats) /
                       (p2.timeInBeats - p1.timeInBeats);
            return p1.value + static_cast<float>(t) * (p2.value - p1.value);
        }
    }

    return points.back().value;
}
```

## Automation Recording

### AutomationRecorder

```cpp
class AutomationRecorder {
public:
    AutomationRecorder(Track* track);

    // Called when user interacts with a control
    void onControlTouchStart(const juce::String& parameterId);
    void onControlTouchEnd(const juce::String& parameterId);
    void onParameterChanged(const juce::String& parameterId, float normalizedValue);

    // Called each audio block during playback
    void process(double positionInBeats, bool isPlaying);

private:
    Track* track;

    struct RecordingState {
        bool isTouching = false;
        bool hasLatched = false;
        float lastValue = 0.0f;
    };
    std::map<juce::String, RecordingState> recordingStates;

    double lastRecordedBeat = -1.0;
    static constexpr double MIN_POINT_INTERVAL = 0.0625;  // 1/16th beat
};
```

### Recording Logic

```cpp
void AutomationRecorder::onParameterChanged(const juce::String& parameterId,
                                            float normalizedValue) {
    auto mode = track->getAutomationMode();
    if (mode == AutomationMode::Off || mode == AutomationMode::Read) return;

    // Get current playback position from audio engine
    double currentBeat = getCurrentPlaybackPosition();
    if (currentBeat < 0) return;  // Not playing

    auto& state = recordingStates[parameterId];

    bool shouldRecord = false;

    switch (mode) {
        case AutomationMode::Write:
            shouldRecord = true;
            break;

        case AutomationMode::Touch:
            shouldRecord = state.isTouching;
            break;

        case AutomationMode::Latch:
            shouldRecord = state.isTouching || state.hasLatched;
            if (state.isTouching) state.hasLatched = true;
            break;

        default:
            break;
    }

    if (shouldRecord && (currentBeat - lastRecordedBeat) >= MIN_POINT_INTERVAL) {
        auto* lane = track->getOrCreateAutomationLane(parameterId);
        lane->addPoint(currentBeat, normalizedValue, CurveType::Linear);
        lastRecordedBeat = currentBeat;
        state.lastValue = normalizedValue;
    }
}
```

## UI Components

### AutomationLaneComponent

Renders a single automation lane below the track.

```cpp
class AutomationLaneComponent : public juce::Component {
public:
    AutomationLaneComponent(Track* track, AutomationLane* lane);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Coordinate conversion
    double xToBeats(float x) const;
    float beatsToX(double beats) const;
    float yToValue(float y) const;
    float valueToY(float value) const;

private:
    Track* track;
    AutomationLane* lane;

    int selectedPointIndex = -1;
    bool isDragging = false;

    static constexpr int POINT_RADIUS = 5;
    static constexpr int LANE_HEIGHT = 60;
};
```

### TrackLane Extensions

```cpp
class TrackLane : public juce::Component {
    // ... existing ...

    // Automation UI
    std::vector<std::unique_ptr<AutomationLaneComponent>> automationLaneComponents;
    bool automationExpanded = false;

    void setAutomationExpanded(bool expanded);
    bool isAutomationExpanded() const;
    void refreshAutomationLanes();

    int getExpandedHeight() const;  // Base height + automation lanes
};
```

### TrackHeader Extensions

```cpp
class TrackHeader : public juce::Component {
    // ... existing ...

    // Automation controls
    juce::ComboBox automationModeSelector;
    juce::ComboBox parameterSelector;
    juce::TextButton expandAutomationBtn;

    void populateParameterSelector();
    void onAutomationModeChanged();
    void onParameterSelected();
    void onExpandClicked();
};
```

### UI Layout

```
┌─────────────────────────────────────────────────────────────┐
│ Track Header                              [▼ Auto] [Read ▼] │
├─────────────────────────────────────────────────────────────┤
│ Track Lane (clips)                                          │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ [Clip 1    ] [Clip 2        ]                           │ │
│ └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ Automation: Volume                           [+ Add Lane]   │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │     ●───────●                    ●                      │ │
│ │            ╲                   ╱                        │ │
│ │              ╲               ╱                          │ │
│ │                ●───────────●                            │ │
│ └─────────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ Automation: synth.filter_cutoff                             │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ ●                         ●───────────────●             │ │
│ │  ╲                       ╱                              │ │
│ │    ╲                   ╱                                │ │
│ │      ●───────────────●                                  │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Serialization

### JSON Format

```json
{
  "tracks": [
    {
      "id": "...",
      "name": "Track 1",
      "automationMode": "read",
      "automationLanes": [
        {
          "parameterId": "volume",
          "points": [
            { "time": 0.0, "value": 0.5, "curve": "linear" },
            { "time": 4.0, "value": 0.8, "curve": "linear" },
            { "time": 8.0, "value": 0.3, "curve": "hold" }
          ]
        },
        {
          "parameterId": "synth.filter_cutoff",
          "points": [
            { "time": 0.0, "value": 0.2, "curve": "linear" },
            { "time": 8.0, "value": 0.9, "curve": "linear" }
          ]
        }
      ]
    }
  ]
}
```

## Undo/Redo Integration

### Actions

```cpp
// Source/Core/Actions/AutomationActions.h

class AddAutomationPointAction : public UndoableAction {
public:
    AddAutomationPointAction(const juce::Uuid& trackId,
                             const juce::String& parameterId,
                             const AutomationPoint& point);

    void perform() override;
    void undo() override;
    juce::String getDescription() const override;

private:
    juce::Uuid trackId;
    juce::String parameterId;
    AutomationPoint point;
};

class MoveAutomationPointAction : public UndoableAction {
public:
    MoveAutomationPointAction(const juce::Uuid& trackId,
                              const juce::String& parameterId,
                              int pointIndex,
                              double oldTime, float oldValue,
                              double newTime, float newValue);

    void perform() override;
    void undo() override;
    juce::String getDescription() const override;

private:
    juce::Uuid trackId;
    juce::String parameterId;
    int pointIndex;
    double oldTime, newTime;
    float oldValue, newValue;
};

class DeleteAutomationPointAction : public UndoableAction {
public:
    DeleteAutomationPointAction(const juce::Uuid& trackId,
                                const juce::String& parameterId,
                                int pointIndex,
                                const AutomationPoint& point);

    void perform() override;
    void undo() override;
    juce::String getDescription() const override;

private:
    juce::Uuid trackId;
    juce::String parameterId;
    int pointIndex;
    AutomationPoint point;
};

class ChangeAutomationCurveAction : public UndoableAction {
public:
    ChangeAutomationCurveAction(const juce::Uuid& trackId,
                                const juce::String& parameterId,
                                int pointIndex,
                                CurveType oldCurve,
                                CurveType newCurve);

    void perform() override;
    void undo() override;
    juce::String getDescription() const override;

private:
    juce::Uuid trackId;
    juce::String parameterId;
    int pointIndex;
    CurveType oldCurve, newCurve;
};
```

## File Structure

New files to create:

```
Source/
├── Audio/
│   ├── AutomationLane.h
│   ├── AutomationLane.cpp
│   ├── AutomationRecorder.h
│   └── AutomationRecorder.cpp
├── Core/
│   └── Actions/
│       └── AutomationActions.h
└── UI/
    └── Timeline/
        ├── AutomationLaneComponent.h
        └── AutomationLaneComponent.cpp
```

Files to modify:

```
Source/
├── Audio/
│   └── Track.h/cpp          # Add automation lanes, mode, apply method
├── Project/
│   └── ProjectSerializer.cpp # Save/load automation
└── UI/
    ├── Timeline/
    │   └── TrackLane.h/cpp   # Expand/collapse automation
    └── Tracks/
        └── TrackHeader.h/cpp  # Mode selector, parameter dropdown
```

## Implementation Order

1. **AutomationLane** - Data model with interpolation
2. **Track integration** - Add lanes to Track, apply in processBlock
3. **AutomationLaneComponent** - Basic drawing and point editing
4. **TrackLane/TrackHeader** - Expand/collapse UI, mode selector
5. **Serialization** - Save/load automation data
6. **Undo actions** - Point add/move/delete undo
7. **AutomationRecorder** - Write/Touch/Latch recording
8. **UI polish** - Parameter dropdown, visual refinements

## Testing

### Unit Tests

- `AutomationLane::getValueAtTime()` interpolation
- `AutomationLane::addPoint()` maintains sort order
- Recording logic for each mode

### Integration Tests

- Automation playback affects actual parameter values
- Save/load preserves automation data
- Undo/redo works correctly

### Manual Tests

- Draw automation points with mouse
- Record automation while playing
- Switch between automation modes
- Multiple lanes per track
