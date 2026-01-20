#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Track.h"
#include "../../Audio/AutomationLane.h"
#include <functional>

/**
 * AutomationLaneComponent - Displays and edits a single automation lane
 *
 * Shows the automation curve with draggable points.
 * Supports adding, moving, and deleting points.
 */
class AutomationLaneComponent : public juce::Component
{
public:
    AutomationLaneComponent(Track& track, AutomationLane& lane, int barWidth);

    void setBarWidth(int width);
    int getBarWidth() const { return barWidth; }

    Track& getTrack() { return track; }
    AutomationLane& getLane() { return lane; }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Coordinate conversion
    double xToBeats(float x) const;
    float beatsToX(double beats) const;
    float yToValue(float y) const;
    float valueToY(float value) const;

    // Callbacks for undo integration
    std::function<void(int pointIndex, double oldTime, float oldValue,
                       double newTime, float newValue)> onPointMoved;
    std::function<void(double time, float value)> onPointAdded;
    std::function<void(int pointIndex)> onPointDeleted;
    std::function<void(int pointIndex, CurveType newCurve)> onCurveChanged;

    // Lane height constant
    static constexpr int LANE_HEIGHT = 60;

private:
    Track& track;
    AutomationLane& lane;
    int barWidth;

    int selectedPointIndex = -1;
    bool isDragging = false;
    double dragStartTime = 0.0;
    float dragStartValue = 0.0f;

    static constexpr int POINT_RADIUS = 5;
    static constexpr double POINT_HIT_TOLERANCE = 0.25;  // In beats

    // Find point at position
    int findPointAt(float x, float y) const;

    // Draw the automation curve
    void drawCurve(juce::Graphics& g);

    // Draw automation points
    void drawPoints(juce::Graphics& g);

    // Show context menu for point
    void showPointContextMenu(int pointIndex, juce::Point<int> position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLaneComponent)
};
