#pragma once

#include <juce_core/juce_core.h>
#include <memory>

/**
 * Base class for all undoable actions in ProgFlow.
 *
 * Implements the Command pattern for undo/redo functionality.
 * Each action knows how to execute itself, undo itself, and redo itself.
 */
class UndoableAction
{
public:
    UndoableAction(const juce::String& description = "Action")
        : actionDescription(description) {}

    virtual ~UndoableAction() = default;

    /**
     * Execute the action for the first time.
     * Called when the action is initially performed.
     * @return true if successful, false otherwise
     */
    virtual bool execute() = 0;

    /**
     * Undo the action, restoring the previous state.
     * @return true if successful, false otherwise
     */
    virtual bool undo() = 0;

    /**
     * Redo the action after it has been undone.
     * Default implementation calls execute(), but subclasses
     * can override for optimization.
     * @return true if successful, false otherwise
     */
    virtual bool redo() { return execute(); }

    /**
     * Get a human-readable description of this action.
     * Used for UI display (e.g., "Undo Add Note").
     */
    const juce::String& getDescription() const { return actionDescription; }

    /**
     * Check if this action can be merged with another action.
     * Used for coalescing rapid parameter changes.
     * @param other The action to potentially merge with
     * @return true if the actions can be merged
     */
    virtual bool canMergeWith(const UndoableAction* other) const
    {
        juce::ignoreUnused(other);
        return false;
    }

    /**
     * Merge another action into this one.
     * Called only if canMergeWith returned true.
     * @param other The action to merge
     */
    virtual void mergeWith(const UndoableAction* other)
    {
        juce::ignoreUnused(other);
    }

    /**
     * Get the timestamp when this action was created.
     * Used for coalescing decisions.
     */
    juce::int64 getTimestamp() const { return timestamp; }

protected:
    juce::String actionDescription;
    juce::int64 timestamp = juce::Time::currentTimeMillis();

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UndoableAction)
};

/**
 * Type alias for action pointers
 */
using UndoableActionPtr = std::unique_ptr<UndoableAction>;
