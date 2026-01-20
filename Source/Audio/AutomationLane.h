#pragma once

#include <juce_core/juce_core.h>
#include <vector>

/**
 * CurveType - How to interpolate between automation points
 */
enum class CurveType
{
    Linear,  // Smooth linear interpolation to next point
    Hold     // Step/hold - keep value until next point
};

/**
 * AutomationPoint - A single point on an automation curve
 */
struct AutomationPoint
{
    double timeInBeats = 0.0;   // Position on timeline
    float value = 0.5f;         // 0.0-1.0 normalized value
    CurveType curve = CurveType::Linear;  // How to interpolate to next point
};

/**
 * AutomationLane - Automation data for a single parameter
 *
 * Stores a series of automation points and provides interpolation
 * to get values at any point in time.
 */
class AutomationLane
{
public:
    explicit AutomationLane(const juce::String& parameterId);

    // Parameter identification
    const juce::String& getParameterId() const { return parameterId; }

    // Point management
    void addPoint(double timeInBeats, float value, CurveType curve = CurveType::Linear);
    void removePoint(int index);
    void movePoint(int index, double newTime, float newValue);
    void setPointCurve(int index, CurveType curve);

    // Value lookup with interpolation
    float getValueAtTime(double timeInBeats) const;

    // Point access
    const std::vector<AutomationPoint>& getPoints() const { return points; }
    int getNumPoints() const { return static_cast<int>(points.size()); }

    // Find point at time (within tolerance)
    int getPointIndexAt(double timeInBeats, double tolerance = 0.1) const;

    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<AutomationLane> fromVar(const juce::var& v);

private:
    juce::String parameterId;
    std::vector<AutomationPoint> points;

    void sortPoints();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLane)
};
