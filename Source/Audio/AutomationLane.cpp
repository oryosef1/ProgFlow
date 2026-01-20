#include "AutomationLane.h"
#include <algorithm>

AutomationLane::AutomationLane(const juce::String& parameterId)
    : parameterId(parameterId)
{
}

void AutomationLane::addPoint(double timeInBeats, float value, CurveType curve)
{
    AutomationPoint point;
    point.timeInBeats = timeInBeats;
    point.value = juce::jlimit(0.0f, 1.0f, value);
    point.curve = curve;
    points.push_back(point);
    sortPoints();
}

void AutomationLane::removePoint(int index)
{
    if (index >= 0 && index < static_cast<int>(points.size()))
    {
        points.erase(points.begin() + index);
    }
}

void AutomationLane::movePoint(int index, double newTime, float newValue)
{
    if (index >= 0 && index < static_cast<int>(points.size()))
    {
        points[index].timeInBeats = newTime;
        points[index].value = juce::jlimit(0.0f, 1.0f, newValue);
        sortPoints();
    }
}

void AutomationLane::setPointCurve(int index, CurveType curve)
{
    if (index >= 0 && index < static_cast<int>(points.size()))
    {
        points[index].curve = curve;
    }
}

float AutomationLane::getValueAtTime(double timeInBeats) const
{
    // Empty lane returns default center value
    if (points.empty())
        return 0.5f;

    // Before first point - return first point's value
    if (timeInBeats <= points.front().timeInBeats)
        return points.front().value;

    // After last point - return last point's value
    if (timeInBeats >= points.back().timeInBeats)
        return points.back().value;

    // Find surrounding points and interpolate
    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        const auto& p1 = points[i];
        const auto& p2 = points[i + 1];

        if (timeInBeats >= p1.timeInBeats && timeInBeats < p2.timeInBeats)
        {
            // Hold curve - return p1's value until we reach p2
            if (p1.curve == CurveType::Hold)
                return p1.value;

            // Linear interpolation
            double t = (timeInBeats - p1.timeInBeats) / (p2.timeInBeats - p1.timeInBeats);
            return p1.value + static_cast<float>(t) * (p2.value - p1.value);
        }
    }

    // Fallback (shouldn't reach here)
    return points.back().value;
}

int AutomationLane::getPointIndexAt(double timeInBeats, double tolerance) const
{
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (std::abs(points[i].timeInBeats - timeInBeats) <= tolerance)
            return static_cast<int>(i);
    }
    return -1;
}

void AutomationLane::sortPoints()
{
    std::sort(points.begin(), points.end(),
        [](const AutomationPoint& a, const AutomationPoint& b) {
            return a.timeInBeats < b.timeInBeats;
        });
}

juce::var AutomationLane::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("parameterId", parameterId);

    juce::Array<juce::var> pointsArray;
    for (const auto& pt : points)
    {
        auto* ptObj = new juce::DynamicObject();
        ptObj->setProperty("time", pt.timeInBeats);
        ptObj->setProperty("value", static_cast<double>(pt.value));
        ptObj->setProperty("curve", pt.curve == CurveType::Hold ? "hold" : "linear");
        pointsArray.add(ptObj);
    }
    obj->setProperty("points", pointsArray);

    return juce::var(obj);
}

std::unique_ptr<AutomationLane> AutomationLane::fromVar(const juce::var& v)
{
    if (!v.isObject())
        return nullptr;

    auto* obj = v.getDynamicObject();
    if (obj == nullptr)
        return nullptr;

    auto parameterId = obj->getProperty("parameterId").toString();
    if (parameterId.isEmpty())
        return nullptr;

    auto lane = std::make_unique<AutomationLane>(parameterId);

    auto pointsVar = obj->getProperty("points");
    if (pointsVar.isArray())
    {
        for (int i = 0; i < pointsVar.size(); ++i)
        {
            auto ptVar = pointsVar[i];
            if (ptVar.isObject())
            {
                auto* ptObj = ptVar.getDynamicObject();
                if (ptObj != nullptr)
                {
                    double time = ptObj->getProperty("time");
                    float value = static_cast<float>(static_cast<double>(ptObj->getProperty("value")));
                    auto curveStr = ptObj->getProperty("curve").toString();
                    CurveType curve = (curveStr == "hold") ? CurveType::Hold : CurveType::Linear;

                    lane->addPoint(time, value, curve);
                }
            }
        }
    }

    return lane;
}
