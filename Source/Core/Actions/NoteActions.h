#pragma once

#include "../UndoableAction.h"
#include "../../Audio/MidiClip.h"
#include <juce_core/juce_core.h>

class MidiClip;

//==============================================================================
/**
 * Action for adding a note to a clip
 */
class AddNoteAction : public UndoableAction
{
public:
    AddNoteAction(MidiClip* clip, const Note& note)
        : UndoableAction("Add Note")
        , targetClip(clip)
        , noteData(note)
    {
    }

    bool execute() override
    {
        if (!targetClip) return false;
        targetClip->addNote(noteData);
        return true;
    }

    bool undo() override
    {
        if (!targetClip) return false;
        targetClip->removeNote(noteData.id);
        return true;
    }

private:
    MidiClip* targetClip;
    Note noteData;
};

//==============================================================================
/**
 * Action for deleting a note from a clip
 */
class DeleteNoteAction : public UndoableAction
{
public:
    DeleteNoteAction(MidiClip* clip, const juce::Uuid& noteId)
        : UndoableAction("Delete Note")
        , targetClip(clip)
        , noteId(noteId)
    {
        // Store the note data for undo
        if (clip)
        {
            if (auto* note = clip->findNote(noteId))
                noteData = *note;
        }
    }

    bool execute() override
    {
        if (!targetClip) return false;
        targetClip->removeNote(noteId);
        return true;
    }

    bool undo() override
    {
        if (!targetClip) return false;
        targetClip->addNote(noteData);
        return true;
    }

private:
    MidiClip* targetClip;
    juce::Uuid noteId;
    Note noteData;
};

//==============================================================================
/**
 * Action for moving a note (changing position and/or pitch)
 */
class MoveNoteAction : public UndoableAction
{
public:
    MoveNoteAction(MidiClip* clip, const juce::Uuid& noteId,
                   int newMidiNote, double newStartBeat)
        : UndoableAction("Move Note")
        , targetClip(clip)
        , noteId(noteId)
        , newMidiNote(newMidiNote)
        , newStartBeat(newStartBeat)
    {
        // Store original values for undo
        if (clip)
        {
            if (auto* note = clip->findNote(noteId))
            {
                oldMidiNote = note->midiNote;
                oldStartBeat = note->startBeat;
            }
        }
    }

    bool execute() override
    {
        return applyValues(newMidiNote, newStartBeat);
    }

    bool undo() override
    {
        return applyValues(oldMidiNote, oldStartBeat);
    }

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* moveAction = dynamic_cast<const MoveNoteAction*>(other))
        {
            return moveAction->targetClip == targetClip &&
                   moveAction->noteId == noteId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* moveAction = dynamic_cast<const MoveNoteAction*>(other))
        {
            // Keep our old values, take the new values from the merged action
            newMidiNote = moveAction->newMidiNote;
            newStartBeat = moveAction->newStartBeat;
        }
    }

private:
    bool applyValues(int midiNote, double startBeat)
    {
        if (!targetClip) return false;

        auto* note = targetClip->findNote(noteId);
        if (!note) return false;

        note->midiNote = midiNote;
        note->startBeat = startBeat;
        return true;
    }

    MidiClip* targetClip;
    juce::Uuid noteId;

    int oldMidiNote = 60;
    double oldStartBeat = 0.0;

    int newMidiNote = 60;
    double newStartBeat = 0.0;
};

//==============================================================================
/**
 * Action for resizing a note (changing duration)
 */
class ResizeNoteAction : public UndoableAction
{
public:
    ResizeNoteAction(MidiClip* clip, const juce::Uuid& noteId, double newDuration)
        : UndoableAction("Resize Note")
        , targetClip(clip)
        , noteId(noteId)
        , newDuration(newDuration)
    {
        // Store original duration for undo
        if (clip)
        {
            if (auto* note = clip->findNote(noteId))
                oldDuration = note->durationBeats;
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
        if (auto* resizeAction = dynamic_cast<const ResizeNoteAction*>(other))
        {
            return resizeAction->targetClip == targetClip &&
                   resizeAction->noteId == noteId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* resizeAction = dynamic_cast<const ResizeNoteAction*>(other))
        {
            newDuration = resizeAction->newDuration;
        }
    }

private:
    bool applyDuration(double duration)
    {
        if (!targetClip) return false;

        auto* note = targetClip->findNote(noteId);
        if (!note) return false;

        note->durationBeats = duration;
        return true;
    }

    MidiClip* targetClip;
    juce::Uuid noteId;
    double oldDuration = 1.0;
    double newDuration = 1.0;
};

//==============================================================================
/**
 * Action for changing note velocity
 */
class ChangeNoteVelocityAction : public UndoableAction
{
public:
    ChangeNoteVelocityAction(MidiClip* clip, const juce::Uuid& noteId, float newVelocity)
        : UndoableAction("Change Velocity")
        , targetClip(clip)
        , noteId(noteId)
        , newVelocity(newVelocity)
    {
        if (clip)
        {
            if (auto* note = clip->findNote(noteId))
                oldVelocity = note->velocity;
        }
    }

    bool execute() override
    {
        return applyVelocity(newVelocity);
    }

    bool undo() override
    {
        return applyVelocity(oldVelocity);
    }

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* velAction = dynamic_cast<const ChangeNoteVelocityAction*>(other))
        {
            return velAction->targetClip == targetClip &&
                   velAction->noteId == noteId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* velAction = dynamic_cast<const ChangeNoteVelocityAction*>(other))
        {
            newVelocity = velAction->newVelocity;
        }
    }

private:
    bool applyVelocity(float velocity)
    {
        if (!targetClip) return false;

        auto* note = targetClip->findNote(noteId);
        if (!note) return false;

        note->velocity = velocity;
        return true;
    }

    MidiClip* targetClip;
    juce::Uuid noteId;
    float oldVelocity = 0.8f;
    float newVelocity = 0.8f;
};

//==============================================================================
/**
 * Action for deleting multiple notes at once
 */
class DeleteMultipleNotesAction : public UndoableAction
{
public:
    DeleteMultipleNotesAction(MidiClip* clip, const std::vector<juce::Uuid>& noteIds)
        : UndoableAction("Delete Notes")
        , targetClip(clip)
    {
        // Store all note data for undo
        if (clip)
        {
            for (const auto& id : noteIds)
            {
                if (auto* note = clip->findNote(id))
                    deletedNotes.push_back(*note);
            }
        }
    }

    bool execute() override
    {
        if (!targetClip) return false;

        for (const auto& note : deletedNotes)
            targetClip->removeNote(note.id);

        return true;
    }

    bool undo() override
    {
        if (!targetClip) return false;

        for (const auto& note : deletedNotes)
            targetClip->addNote(note);

        return true;
    }

private:
    MidiClip* targetClip;
    std::vector<Note> deletedNotes;
};

//==============================================================================
/**
 * Action for transposing multiple notes
 */
class TransposeNotesAction : public UndoableAction
{
public:
    TransposeNotesAction(MidiClip* clip, const std::vector<juce::Uuid>& noteIds, int semitones)
        : UndoableAction("Transpose Notes")
        , targetClip(clip)
        , noteIds(noteIds)
        , semitones(semitones)
    {
    }

    bool execute() override
    {
        return applyTranspose(semitones);
    }

    bool undo() override
    {
        return applyTranspose(-semitones);
    }

private:
    bool applyTranspose(int amount)
    {
        if (!targetClip) return false;

        for (const auto& id : noteIds)
        {
            if (auto* note = targetClip->findNote(id))
            {
                note->midiNote = juce::jlimit(0, 127, note->midiNote + amount);
            }
        }
        return true;
    }

    MidiClip* targetClip;
    std::vector<juce::Uuid> noteIds;
    int semitones;
};
