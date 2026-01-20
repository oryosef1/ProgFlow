#pragma once

#include "../UndoableAction.h"
#include "../../Audio/Track.h"
#include "../../Audio/MidiClip.h"
#include <juce_core/juce_core.h>

class Track;
class MidiClip;

//==============================================================================
/**
 * Action for adding a clip to a track
 */
class AddClipAction : public UndoableAction
{
public:
    AddClipAction(Track* track, double startBar, double durationBars)
        : UndoableAction("Add Clip")
        , targetTrack(track)
        , startBar(startBar)
        , durationBars(durationBars)
    {
    }

    bool execute() override
    {
        if (!targetTrack) return false;

        auto* clip = targetTrack->addClip(startBar, durationBars);
        if (clip)
        {
            clipId = clip->getId();
            return true;
        }
        return false;
    }

    bool undo() override
    {
        if (!targetTrack) return false;
        targetTrack->removeClip(clipId);
        return true;
    }

    const juce::Uuid& getClipId() const { return clipId; }

private:
    Track* targetTrack;
    double startBar;
    double durationBars;
    juce::Uuid clipId;
};

//==============================================================================
/**
 * Action for deleting a clip from a track
 */
class DeleteClipAction : public UndoableAction
{
public:
    DeleteClipAction(Track* track, const juce::Uuid& clipId)
        : UndoableAction("Delete Clip")
        , targetTrack(track)
        , clipId(clipId)
    {
        // Store clip data for undo
        if (track)
        {
            if (auto* clip = track->getClip(clipId))
            {
                // Serialize clip to var for restoration
                clipData = clip->toVar();
            }
        }
    }

    bool execute() override
    {
        if (!targetTrack) return false;
        targetTrack->removeClip(clipId);
        return true;
    }

    bool undo() override
    {
        if (!targetTrack) return false;

        // Restore clip from serialized data
        auto clip = MidiClip::fromVar(clipData);
        if (clip)
        {
            targetTrack->addClip(std::move(clip));
            return true;
        }
        return false;
    }

private:
    Track* targetTrack;
    juce::Uuid clipId;
    juce::var clipData;
};

//==============================================================================
/**
 * Action for moving a clip (changing start position)
 */
class MoveClipAction : public UndoableAction
{
public:
    MoveClipAction(Track* track, const juce::Uuid& clipId, double newStartBar)
        : UndoableAction("Move Clip")
        , targetTrack(track)
        , clipId(clipId)
        , newStartBar(newStartBar)
    {
        // Store original position for undo
        if (track)
        {
            if (auto* clip = track->getClip(clipId))
                oldStartBar = clip->getStartBar();
        }
    }

    bool execute() override
    {
        return applyPosition(newStartBar);
    }

    bool undo() override
    {
        return applyPosition(oldStartBar);
    }

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* moveAction = dynamic_cast<const MoveClipAction*>(other))
        {
            return moveAction->targetTrack == targetTrack &&
                   moveAction->clipId == clipId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* moveAction = dynamic_cast<const MoveClipAction*>(other))
        {
            newStartBar = moveAction->newStartBar;
        }
    }

private:
    bool applyPosition(double startBar)
    {
        if (!targetTrack) return false;

        auto* clip = targetTrack->getClip(clipId);
        if (!clip) return false;

        clip->setStartBar(startBar);
        return true;
    }

    Track* targetTrack;
    juce::Uuid clipId;
    double oldStartBar = 0.0;
    double newStartBar = 0.0;
};

//==============================================================================
/**
 * Action for resizing a clip (changing duration)
 */
class ResizeClipAction : public UndoableAction
{
public:
    ResizeClipAction(Track* track, const juce::Uuid& clipId, double newDurationBars)
        : UndoableAction("Resize Clip")
        , targetTrack(track)
        , clipId(clipId)
        , newDuration(newDurationBars)
    {
        // Store original duration for undo
        if (track)
        {
            if (auto* clip = track->getClip(clipId))
                oldDuration = clip->getDurationBars();
        }
    }

    bool execute() override
    {
        return applyDuration(newDuration);
    }

    bool undo() override
    {
        return applyDuration(oldDuration);
    }

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* resizeAction = dynamic_cast<const ResizeClipAction*>(other))
        {
            return resizeAction->targetTrack == targetTrack &&
                   resizeAction->clipId == clipId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* resizeAction = dynamic_cast<const ResizeClipAction*>(other))
        {
            newDuration = resizeAction->newDuration;
        }
    }

private:
    bool applyDuration(double duration)
    {
        if (!targetTrack) return false;

        auto* clip = targetTrack->getClip(clipId);
        if (!clip) return false;

        clip->setDurationBars(duration);
        return true;
    }

    Track* targetTrack;
    juce::Uuid clipId;
    double oldDuration = 4.0;
    double newDuration = 4.0;
};

//==============================================================================
/**
 * Action for renaming a clip
 */
class RenameClipAction : public UndoableAction
{
public:
    RenameClipAction(Track* track, const juce::Uuid& clipId, const juce::String& newName)
        : UndoableAction("Rename Clip")
        , targetTrack(track)
        , clipId(clipId)
        , newName(newName)
    {
        if (track)
        {
            if (auto* clip = track->getClip(clipId))
                oldName = clip->getName();
        }
    }

    bool execute() override
    {
        return applyName(newName);
    }

    bool undo() override
    {
        return applyName(oldName);
    }

private:
    bool applyName(const juce::String& name)
    {
        if (!targetTrack) return false;

        auto* clip = targetTrack->getClip(clipId);
        if (!clip) return false;

        clip->setName(name);
        return true;
    }

    Track* targetTrack;
    juce::Uuid clipId;
    juce::String oldName;
    juce::String newName;
};

//==============================================================================
/**
 * Action for changing clip color
 */
class ChangeClipColorAction : public UndoableAction
{
public:
    ChangeClipColorAction(Track* track, const juce::Uuid& clipId, juce::Colour newColour)
        : UndoableAction("Change Clip Color")
        , targetTrack(track)
        , clipId(clipId)
        , newColour(newColour)
    {
        if (track)
        {
            if (auto* clip = track->getClip(clipId))
                oldColour = clip->getColour();
        }
    }

    bool execute() override
    {
        return applyColour(newColour);
    }

    bool undo() override
    {
        return applyColour(oldColour);
    }

private:
    bool applyColour(juce::Colour colour)
    {
        if (!targetTrack) return false;

        auto* clip = targetTrack->getClip(clipId);
        if (!clip) return false;

        clip->setColour(colour);
        return true;
    }

    Track* targetTrack;
    juce::Uuid clipId;
    juce::Colour oldColour;
    juce::Colour newColour;
};

//==============================================================================
/**
 * Action for duplicating a clip
 */
class DuplicateClipAction : public UndoableAction
{
public:
    DuplicateClipAction(Track* track, const juce::Uuid& sourceClipId, double newStartBar)
        : UndoableAction("Duplicate Clip")
        , targetTrack(track)
        , sourceClipId(sourceClipId)
        , newStartBar(newStartBar)
    {
    }

    bool execute() override
    {
        if (!targetTrack) return false;

        auto* sourceClip = targetTrack->getClip(sourceClipId);
        if (!sourceClip) return false;

        // Create new clip as copy
        auto* newClip = targetTrack->addClip(newStartBar, sourceClip->getDurationBars());
        if (!newClip) return false;

        newClipId = newClip->getId();
        newClip->setName(sourceClip->getName() + " (copy)");
        newClip->setColour(sourceClip->getColour());

        // Copy all notes
        for (const auto& note : sourceClip->getNotes())
        {
            newClip->addNote(note.midiNote, note.startBeat, note.durationBeats, note.velocity);
        }

        return true;
    }

    bool undo() override
    {
        if (!targetTrack) return false;
        targetTrack->removeClip(newClipId);
        return true;
    }

    const juce::Uuid& getNewClipId() const { return newClipId; }

private:
    Track* targetTrack;
    juce::Uuid sourceClipId;
    juce::Uuid newClipId;
    double newStartBar;
};
