#pragma once

#include "../UndoableAction.h"
#include "../../Audio/Track.h"
#include "../../Audio/AutomationLane.h"
#include <juce_core/juce_core.h>

//==============================================================================
/**
 * Action for adding an automation point
 */
class AddAutomationPointAction : public UndoableAction
{
public:
    AddAutomationPointAction(Track* track, const juce::String& parameterId,
                             double timeInBeats, float value, CurveType curve = CurveType::Linear)
        : UndoableAction("Add Automation Point")
        , targetTrack(track)
        , parameterId(parameterId)
        , timeInBeats(timeInBeats)
        , value(value)
        , curve(curve)
    {
    }

    bool execute() override
    {
        if (!targetTrack) return false;

        auto* lane = targetTrack->getOrCreateAutomationLane(parameterId);
        if (!lane) return false;

        lane->addPoint(timeInBeats, value, curve);
        return true;
    }

    bool undo() override
    {
        if (!targetTrack) return false;

        auto* lane = targetTrack->getAutomationLane(parameterId);
        if (!lane) return false;

        // Find and remove the point we added
        int index = lane->getPointIndexAt(timeInBeats, 0.001);
        if (index >= 0)
        {
            lane->removePoint(index);
            return true;
        }
        return false;
    }

private:
    Track* targetTrack;
    juce::String parameterId;
    double timeInBeats;
    float value;
    CurveType curve;
};

//==============================================================================
/**
 * Action for deleting an automation point
 */
class DeleteAutomationPointAction : public UndoableAction
{
public:
    DeleteAutomationPointAction(Track* track, const juce::String& parameterId, int pointIndex)
        : UndoableAction("Delete Automation Point")
        , targetTrack(track)
        , parameterId(parameterId)
        , pointIndex(pointIndex)
    {
        // Store the point data for undo
        if (track)
        {
            if (auto* lane = track->getAutomationLane(parameterId))
            {
                const auto& points = lane->getPoints();
                if (pointIndex >= 0 && pointIndex < static_cast<int>(points.size()))
                {
                    deletedPoint = points[static_cast<size_t>(pointIndex)];
                }
            }
        }
    }

    bool execute() override
    {
        if (!targetTrack) return false;

        auto* lane = targetTrack->getAutomationLane(parameterId);
        if (!lane) return false;

        lane->removePoint(pointIndex);
        return true;
    }

    bool undo() override
    {
        if (!targetTrack) return false;

        auto* lane = targetTrack->getAutomationLane(parameterId);
        if (!lane) return false;

        lane->addPoint(deletedPoint.timeInBeats, deletedPoint.value, deletedPoint.curve);
        return true;
    }

private:
    Track* targetTrack;
    juce::String parameterId;
    int pointIndex;
    AutomationPoint deletedPoint;
};

//==============================================================================
/**
 * Action for moving an automation point
 */
class MoveAutomationPointAction : public UndoableAction
{
public:
    MoveAutomationPointAction(Track* track, const juce::String& parameterId,
                              int pointIndex,
                              double oldTime, float oldValue,
                              double newTime, float newValue)
        : UndoableAction("Move Automation Point")
        , targetTrack(track)
        , parameterId(parameterId)
        , pointIndex(pointIndex)
        , oldTime(oldTime)
        , oldValue(oldValue)
        , newTime(newTime)
        , newValue(newValue)
    {
    }

    bool execute() override
    {
        return applyValues(newTime, newValue);
    }

    bool undo() override
    {
        return applyValues(oldTime, oldValue);
    }

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* moveAction = dynamic_cast<const MoveAutomationPointAction*>(other))
        {
            return moveAction->targetTrack == targetTrack &&
                   moveAction->parameterId == parameterId &&
                   moveAction->pointIndex == pointIndex;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* moveAction = dynamic_cast<const MoveAutomationPointAction*>(other))
        {
            // Keep our old values, take the new values from the merged action
            newTime = moveAction->newTime;
            newValue = moveAction->newValue;
        }
    }

private:
    bool applyValues(double time, float value)
    {
        if (!targetTrack) return false;

        auto* lane = targetTrack->getAutomationLane(parameterId);
        if (!lane) return false;

        lane->movePoint(pointIndex, time, value);

        // Update pointIndex after sort
        const auto& points = lane->getPoints();
        for (size_t i = 0; i < points.size(); ++i)
        {
            if (std::abs(points[i].timeInBeats - time) < 0.001 &&
                std::abs(points[i].value - value) < 0.001f)
            {
                pointIndex = static_cast<int>(i);
                break;
            }
        }

        return true;
    }

    Track* targetTrack;
    juce::String parameterId;
    int pointIndex;

    double oldTime;
    float oldValue;
    double newTime;
    float newValue;
};

//==============================================================================
/**
 * Action for changing the curve type of an automation point
 */
class ChangeCurveTypeAction : public UndoableAction
{
public:
    ChangeCurveTypeAction(Track* track, const juce::String& parameterId,
                          int pointIndex, CurveType oldCurve, CurveType newCurve)
        : UndoableAction("Change Curve Type")
        , targetTrack(track)
        , parameterId(parameterId)
        , pointIndex(pointIndex)
        , oldCurve(oldCurve)
        , newCurve(newCurve)
    {
    }

    bool execute() override
    {
        return applyCurve(newCurve);
    }

    bool undo() override
    {
        return applyCurve(oldCurve);
    }

private:
    bool applyCurve(CurveType curve)
    {
        if (!targetTrack) return false;

        auto* lane = targetTrack->getAutomationLane(parameterId);
        if (!lane) return false;

        lane->setPointCurve(pointIndex, curve);
        return true;
    }

    Track* targetTrack;
    juce::String parameterId;
    int pointIndex;
    CurveType oldCurve;
    CurveType newCurve;
};
