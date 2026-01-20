/**
 * AutomationLane Unit Tests
 *
 * Tests for automation lane data model and interpolation.
 */

#include <juce_core/juce_core.h>
#include "../Source/Audio/AutomationLane.h"

class AutomationLaneTests : public juce::UnitTest
{
public:
    AutomationLaneTests() : UnitTest("AutomationLane") {}

    void runTest() override
    {
        beginTest("Empty lane returns default value");
        {
            AutomationLane lane("volume");
            expectEquals(lane.getValueAtTime(0.0), 0.5f);
            expectEquals(lane.getValueAtTime(100.0), 0.5f);
        }

        beginTest("Single point returns that value everywhere");
        {
            AutomationLane lane("volume");
            lane.addPoint(4.0, 0.8f);
            expectEquals(lane.getValueAtTime(0.0), 0.8f);
            expectEquals(lane.getValueAtTime(4.0), 0.8f);
            expectEquals(lane.getValueAtTime(100.0), 0.8f);
        }

        beginTest("Linear interpolation between two points");
        {
            AutomationLane lane("volume");
            lane.addPoint(0.0, 0.0f);
            lane.addPoint(4.0, 1.0f);

            expectEquals(lane.getValueAtTime(0.0), 0.0f);
            expectEquals(lane.getValueAtTime(2.0), 0.5f);
            expectEquals(lane.getValueAtTime(4.0), 1.0f);
        }

        beginTest("Hold curve returns previous value until next point");
        {
            AutomationLane lane("volume");
            lane.addPoint(0.0, 0.2f, CurveType::Hold);
            lane.addPoint(4.0, 0.8f);

            expectEquals(lane.getValueAtTime(0.0), 0.2f);
            expectEquals(lane.getValueAtTime(2.0), 0.2f);  // Hold, not interpolate
            expectEquals(lane.getValueAtTime(3.9), 0.2f);  // Still hold
            expectEquals(lane.getValueAtTime(4.0), 0.8f);  // At next point
        }

        beginTest("Points auto-sorted by time");
        {
            AutomationLane lane("volume");
            lane.addPoint(4.0, 0.8f);
            lane.addPoint(0.0, 0.2f);
            lane.addPoint(2.0, 0.5f);

            const auto& points = lane.getPoints();
            expect(points.size() == 3);
            expectEquals(points[0].timeInBeats, 0.0);
            expectEquals(points[1].timeInBeats, 2.0);
            expectEquals(points[2].timeInBeats, 4.0);
        }

        beginTest("Remove point by index");
        {
            AutomationLane lane("volume");
            lane.addPoint(0.0, 0.2f);
            lane.addPoint(2.0, 0.5f);
            lane.addPoint(4.0, 0.8f);

            lane.removePoint(1);  // Remove middle point

            const auto& points = lane.getPoints();
            expect(points.size() == 2);
            expectEquals(points[0].timeInBeats, 0.0);
            expectEquals(points[1].timeInBeats, 4.0);
        }

        beginTest("Move point changes time and value");
        {
            AutomationLane lane("volume");
            lane.addPoint(0.0, 0.2f);
            lane.addPoint(2.0, 0.5f);
            lane.addPoint(4.0, 0.8f);

            lane.movePoint(1, 3.0, 0.6f);  // Move middle point

            const auto& points = lane.getPoints();
            expectEquals(points[1].timeInBeats, 3.0);
            expectEquals(points[1].value, 0.6f);
        }

        beginTest("Set point curve type");
        {
            AutomationLane lane("volume");
            lane.addPoint(0.0, 0.2f, CurveType::Linear);
            lane.addPoint(4.0, 0.8f);

            lane.setPointCurve(0, CurveType::Hold);

            const auto& points = lane.getPoints();
            expect(points[0].curve == CurveType::Hold);
        }

        beginTest("Get parameter ID");
        {
            AutomationLane lane("synth.filter_cutoff");
            expectEquals(lane.getParameterId(), juce::String("synth.filter_cutoff"));
        }

        beginTest("Serialization round-trip");
        {
            AutomationLane original("volume");
            original.addPoint(0.0, 0.2f, CurveType::Linear);
            original.addPoint(2.0, 0.6f, CurveType::Hold);
            original.addPoint(4.0, 0.9f, CurveType::Linear);

            auto var = original.toVar();
            auto restored = AutomationLane::fromVar(var);

            expect(restored != nullptr);
            expectEquals(restored->getParameterId(), juce::String("volume"));

            const auto& points = restored->getPoints();
            expect(points.size() == 3);
            expectEquals(points[0].timeInBeats, 0.0);
            expectEquals(points[0].value, 0.2f);
            expect(points[0].curve == CurveType::Linear);
            expectEquals(points[1].timeInBeats, 2.0);
            expectEquals(points[1].value, 0.6f);
            expect(points[1].curve == CurveType::Hold);
        }
    }
};

// Register the test
static AutomationLaneTests automationLaneTests;
