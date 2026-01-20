#pragma once

#include "UndoableAction.h"
#include <juce_core/juce_core.h>
#include <vector>
#include <functional>

/**
 * Manages the undo/redo history for the application.
 *
 * Singleton that maintains two stacks:
 * - Undo stack: Actions that can be undone
 * - Redo stack: Actions that have been undone and can be redone
 *
 * When a new action is performed, the redo stack is cleared.
 * Supports action coalescing for rapid parameter changes.
 */
class UndoManager
{
public:
    /**
     * Get the singleton instance
     */
    static UndoManager& getInstance()
    {
        static UndoManager instance;
        return instance;
    }

    /**
     * Maximum number of actions to keep in history
     */
    static constexpr int MAX_UNDO_HISTORY = 100;

    /**
     * Time window for coalescing similar actions (milliseconds)
     */
    static constexpr juce::int64 COALESCE_WINDOW_MS = 500;

    //==========================================================================
    // Action Management

    /**
     * Perform an action and add it to the undo stack.
     * Executes the action immediately and clears the redo stack.
     * @param action The action to perform
     * @return true if the action executed successfully
     */
    bool performAction(UndoableActionPtr action);

    /**
     * Undo the most recent action.
     * @return true if an action was undone
     */
    bool undo();

    /**
     * Redo the most recently undone action.
     * @return true if an action was redone
     */
    bool redo();

    //==========================================================================
    // State Queries

    /**
     * Check if there are actions that can be undone
     */
    bool canUndo() const { return !undoStack.empty(); }

    /**
     * Check if there are actions that can be redone
     */
    bool canRedo() const { return !redoStack.empty(); }

    /**
     * Get description of the action that would be undone
     */
    juce::String getUndoDescription() const;

    /**
     * Get description of the action that would be redone
     */
    juce::String getRedoDescription() const;

    /**
     * Get the number of actions in the undo stack
     */
    int getNumUndoActions() const { return static_cast<int>(undoStack.size()); }

    /**
     * Get the number of actions in the redo stack
     */
    int getNumRedoActions() const { return static_cast<int>(redoStack.size()); }

    //==========================================================================
    // History Management

    /**
     * Clear all undo/redo history
     */
    void clearHistory();

    /**
     * Begin a compound action group.
     * All actions performed until endCompoundAction() will be
     * treated as a single undoable unit.
     * @param description Description for the compound action
     */
    void beginCompoundAction(const juce::String& description);

    /**
     * End a compound action group.
     */
    void endCompoundAction();

    /**
     * Check if currently recording a compound action
     */
    bool isRecordingCompound() const { return compoundDepth > 0; }

    //==========================================================================
    // Listeners

    /**
     * Listener interface for undo state changes
     */
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void undoStateChanged() = 0;
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    UndoManager() = default;
    ~UndoManager() = default;

    // Delete copy/move
    UndoManager(const UndoManager&) = delete;
    UndoManager& operator=(const UndoManager&) = delete;
    UndoManager(UndoManager&&) = delete;
    UndoManager& operator=(UndoManager&&) = delete;

    // Undo/Redo stacks
    std::vector<UndoableActionPtr> undoStack;
    std::vector<UndoableActionPtr> redoStack;

    // Compound action support
    std::vector<UndoableActionPtr> compoundActions;
    juce::String compoundDescription;
    int compoundDepth = 0;

    // Listeners
    juce::ListenerList<Listener> listeners;

    // Trim undo stack to max size
    void trimUndoStack();

    // Notify listeners of state change
    void notifyListeners();

    // Try to coalesce action with the previous one
    bool tryCoalesce(UndoableAction* action);
};
