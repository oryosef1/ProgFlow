#include "AutomationLaneComponent.h"

AutomationLaneComponent::AutomationLaneComponent(Track& track, AutomationLane& lane, int barWidth)
    : track(track), lane(lane), barWidth(barWidth)
{
    setSize(800, LANE_HEIGHT);
}

void AutomationLaneComponent::setBarWidth(int width)
{
    barWidth = width;
    repaint();
}

void AutomationLaneComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Draw grid lines (every bar)
    g.setColour(juce::Colour(0xff2a2a4e));
    int numBars = getWidth() / barWidth + 1;
    for (int i = 0; i <= numBars; ++i)
    {
        int x = i * barWidth;
        g.drawVerticalLine(x, 0.0f, static_cast<float>(getHeight()));
    }

    // Draw center line (0.5 value)
    g.setColour(juce::Colour(0xff3a3a5e));
    float centerY = valueToY(0.5f);
    g.drawHorizontalLine(static_cast<int>(centerY), 0.0f, static_cast<float>(getWidth()));

    // Draw curve and points
    drawCurve(g);
    drawPoints(g);

    // Draw parameter name
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(12.0f);
    g.drawText(lane.getParameterId(), 5, 5, 200, 15, juce::Justification::left);
}

void AutomationLaneComponent::resized()
{
    // Nothing specific needed
}

void AutomationLaneComponent::drawCurve(juce::Graphics& g)
{
    const auto& points = lane.getPoints();
    if (points.empty())
        return;

    g.setColour(juce::Colour(0xff4a9eff));

    juce::Path path;
    bool pathStarted = false;

    // Draw line from left edge to first point
    float firstX = beatsToX(points.front().timeInBeats);
    float firstY = valueToY(points.front().value);

    if (firstX > 0)
    {
        path.startNewSubPath(0, firstY);
        path.lineTo(firstX, firstY);
        pathStarted = true;
    }

    // Draw between points
    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& pt = points[i];
        float x = beatsToX(pt.timeInBeats);
        float y = valueToY(pt.value);

        if (!pathStarted)
        {
            path.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            // Check previous point's curve type
            if (i > 0 && points[i - 1].curve == CurveType::Hold)
            {
                // Step: horizontal then vertical
                float prevY = valueToY(points[i - 1].value);
                path.lineTo(x, prevY);
                path.lineTo(x, y);
            }
            else
            {
                // Linear interpolation
                path.lineTo(x, y);
            }
        }
    }

    // Draw line from last point to right edge
    float lastX = beatsToX(points.back().timeInBeats);
    float lastY = valueToY(points.back().value);
    if (lastX < getWidth())
    {
        path.lineTo(static_cast<float>(getWidth()), lastY);
    }

    g.strokePath(path, juce::PathStrokeType(2.0f));
}

void AutomationLaneComponent::drawPoints(juce::Graphics& g)
{
    const auto& points = lane.getPoints();

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& pt = points[i];
        float x = beatsToX(pt.timeInBeats);
        float y = valueToY(pt.value);

        // Point fill
        if (static_cast<int>(i) == selectedPointIndex)
        {
            g.setColour(juce::Colour(0xffffa500));  // Orange for selected
        }
        else
        {
            g.setColour(juce::Colour(0xff4a9eff));  // Blue
        }
        g.fillEllipse(x - POINT_RADIUS, y - POINT_RADIUS,
                      POINT_RADIUS * 2.0f, POINT_RADIUS * 2.0f);

        // Point border
        g.setColour(juce::Colours::white);
        g.drawEllipse(x - POINT_RADIUS, y - POINT_RADIUS,
                      POINT_RADIUS * 2.0f, POINT_RADIUS * 2.0f, 1.0f);

        // Draw "H" indicator for hold curves
        if (pt.curve == CurveType::Hold)
        {
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(9.0f);
            g.drawText("H", static_cast<int>(x) + POINT_RADIUS + 2,
                       static_cast<int>(y) - 6, 10, 12, juce::Justification::left);
        }
    }
}

int AutomationLaneComponent::findPointAt(float x, float y) const
{
    const auto& points = lane.getPoints();
    double clickBeats = xToBeats(x);
    float clickValue = yToValue(y);

    for (size_t i = 0; i < points.size(); ++i)
    {
        const auto& pt = points[i];
        double timeDiff = std::abs(pt.timeInBeats - clickBeats);
        float valueDiff = std::abs(pt.value - clickValue);

        // Check if within hit radius
        float ptX = beatsToX(pt.timeInBeats);
        float ptY = valueToY(pt.value);
        float dx = x - ptX;
        float dy = y - ptY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= POINT_RADIUS * 2.0f)
            return static_cast<int>(i);
    }

    return -1;
}

void AutomationLaneComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        int pointIndex = findPointAt(static_cast<float>(e.x), static_cast<float>(e.y));
        if (pointIndex >= 0)
        {
            showPointContextMenu(pointIndex, e.getPosition());
        }
        return;
    }

    selectedPointIndex = findPointAt(static_cast<float>(e.x), static_cast<float>(e.y));

    if (selectedPointIndex >= 0)
    {
        // Start dragging existing point
        isDragging = true;
        const auto& pt = lane.getPoints()[static_cast<size_t>(selectedPointIndex)];
        dragStartTime = pt.timeInBeats;
        dragStartValue = pt.value;
    }

    repaint();
}

void AutomationLaneComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging || selectedPointIndex < 0)
        return;

    double newTime = xToBeats(static_cast<float>(e.x));
    float newValue = yToValue(static_cast<float>(e.y));

    // Clamp values
    newTime = std::max(0.0, newTime);
    newValue = juce::jlimit(0.0f, 1.0f, newValue);

    // Update point position (will be re-sorted)
    lane.movePoint(selectedPointIndex, newTime, newValue);

    // Find the new index after sorting
    const auto& points = lane.getPoints();
    for (size_t i = 0; i < points.size(); ++i)
    {
        if (std::abs(points[i].timeInBeats - newTime) < 0.001 &&
            std::abs(points[i].value - newValue) < 0.001f)
        {
            selectedPointIndex = static_cast<int>(i);
            break;
        }
    }

    repaint();
}

void AutomationLaneComponent::mouseUp(const juce::MouseEvent& e)
{
    if (isDragging && selectedPointIndex >= 0 && onPointMoved)
    {
        const auto& pt = lane.getPoints()[static_cast<size_t>(selectedPointIndex)];
        onPointMoved(selectedPointIndex, dragStartTime, dragStartValue,
                     pt.timeInBeats, pt.value);
    }

    isDragging = false;
}

void AutomationLaneComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Check if clicking on existing point
    int pointIndex = findPointAt(static_cast<float>(e.x), static_cast<float>(e.y));

    if (pointIndex >= 0)
    {
        // Toggle curve type on double-click
        const auto& pt = lane.getPoints()[static_cast<size_t>(pointIndex)];
        CurveType newCurve = (pt.curve == CurveType::Linear) ? CurveType::Hold : CurveType::Linear;
        lane.setPointCurve(pointIndex, newCurve);

        if (onCurveChanged)
            onCurveChanged(pointIndex, newCurve);
    }
    else
    {
        // Add new point
        double time = xToBeats(static_cast<float>(e.x));
        float value = yToValue(static_cast<float>(e.y));

        time = std::max(0.0, time);
        value = juce::jlimit(0.0f, 1.0f, value);

        lane.addPoint(time, value);

        if (onPointAdded)
            onPointAdded(time, value);
    }

    repaint();
}

void AutomationLaneComponent::showPointContextMenu(int pointIndex, juce::Point<int> position)
{
    juce::PopupMenu menu;

    const auto& pt = lane.getPoints()[static_cast<size_t>(pointIndex)];

    // Curve type options
    menu.addItem(1, "Linear", true, pt.curve == CurveType::Linear);
    menu.addItem(2, "Hold (Step)", true, pt.curve == CurveType::Hold);
    menu.addSeparator();
    menu.addItem(3, "Delete Point");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
        localAreaToGlobal(juce::Rectangle<int>(position.x, position.y, 1, 1))),
        [this, pointIndex](int result)
        {
            if (result == 1)
            {
                lane.setPointCurve(pointIndex, CurveType::Linear);
                if (onCurveChanged)
                    onCurveChanged(pointIndex, CurveType::Linear);
            }
            else if (result == 2)
            {
                lane.setPointCurve(pointIndex, CurveType::Hold);
                if (onCurveChanged)
                    onCurveChanged(pointIndex, CurveType::Hold);
            }
            else if (result == 3)
            {
                if (onPointDeleted)
                    onPointDeleted(pointIndex);
                lane.removePoint(pointIndex);
                selectedPointIndex = -1;
            }
            repaint();
        });
}

double AutomationLaneComponent::xToBeats(float x) const
{
    // barWidth pixels = 4 beats (one bar in 4/4)
    return (x / barWidth) * 4.0;
}

float AutomationLaneComponent::beatsToX(double beats) const
{
    return static_cast<float>((beats / 4.0) * barWidth);
}

float AutomationLaneComponent::yToValue(float y) const
{
    // Inverted: top = 1.0, bottom = 0.0
    return 1.0f - (y / static_cast<float>(getHeight()));
}

float AutomationLaneComponent::valueToY(float value) const
{
    // Inverted: top = 1.0, bottom = 0.0
    return (1.0f - value) * static_cast<float>(getHeight());
}
