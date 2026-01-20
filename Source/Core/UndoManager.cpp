#include "UndoManager.h"

//==============================================================================
// Compound Action - groups multiple actions into one
//==============================================================================

class CompoundAction : public UndoableAction
{
public:
    CompoundAction(const juce::String& description,
                   std::vector<UndoableActionPtr>&& actions)
        : UndoableAction(description)
        , subActions(std::move(actions))
    {
    }

    bool execute() override
    {
        // Actions were already executed when added
        return true;
    }

    bool undo() override
    {
        // Undo in reverse order
        for (auto it = subActions.rbegin(); it != subActions.rend(); ++it)
        {
            if (!(*it)->undo())
                return false;
        }
        return true;
    }

    bool redo() override
    {
        // Redo in forward order
        for (auto& action : subActions)
        {
            if (!action->redo())
                return false;
        }
        return true;
    }

private:
    std::vector<UndoableActionPtr> subActions;
};

//==============================================================================
// UndoManager Implementation
//==============================================================================

bool UndoManager::performAction(UndoableActionPtr action)
{
    if (!action)
        return false;

    // Execute the action
    if (!action->execute())
        return false;

    // If recording compound, add to compound list
    if (compoundDepth > 0)
    {
        compoundActions.push_back(std::move(action));
        return true;
    }

    // Try to coalesce with previous action
    if (tryCoalesce(action.get()))
    {
        notifyListeners();
        return true;
    }

    // Clear redo stack when new action is performed
    redoStack.clear();

    // Add to undo stack
    undoStack.push_back(std::move(action));
    trimUndoStack();

    notifyListeners();
    return true;
}

bool UndoManager::undo()
{
    if (undoStack.empty())
        return false;

    // Pop from undo stack
    auto action = std::move(undoStack.back());
    undoStack.pop_back();

    // Perform undo
    if (!action->undo())
    {
        // If undo fails, put it back
        undoStack.push_back(std::move(action));
        return false;
    }

    // Push to redo stack
    redoStack.push_back(std::move(action));

    notifyListeners();
    return true;
}

bool UndoManager::redo()
{
    if (redoStack.empty())
        return false;

    // Pop from redo stack
    auto action = std::move(redoStack.back());
    redoStack.pop_back();

    // Perform redo
    if (!action->redo())
    {
        // If redo fails, put it back
        redoStack.push_back(std::move(action));
        return false;
    }

    // Push to undo stack
    undoStack.push_back(std::move(action));

    notifyListeners();
    return true;
}

juce::String UndoManager::getUndoDescription() const
{
    if (undoStack.empty())
        return "Nothing to undo";
    return "Undo " + undoStack.back()->getDescription();
}

juce::String UndoManager::getRedoDescription() const
{
    if (redoStack.empty())
        return "Nothing to redo";
    return "Redo " + redoStack.back()->getDescription();
}

void UndoManager::clearHistory()
{
    undoStack.clear();
    redoStack.clear();
    compoundActions.clear();
    compoundDepth = 0;
    notifyListeners();
}

void UndoManager::beginCompoundAction(const juce::String& description)
{
    if (compoundDepth == 0)
    {
        compoundDescription = description;
        compoundActions.clear();
    }
    compoundDepth++;
}

void UndoManager::endCompoundAction()
{
    if (compoundDepth <= 0)
        return;

    compoundDepth--;

    if (compoundDepth == 0 && !compoundActions.empty())
    {
        // Create compound action from collected actions
        auto compound = std::make_unique<CompoundAction>(
            compoundDescription,
            std::move(compoundActions)
        );

        // Clear redo stack
        redoStack.clear();

        // Add compound to undo stack
        undoStack.push_back(std::move(compound));
        trimUndoStack();

        compoundActions.clear();
        notifyListeners();
    }
}

void UndoManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void UndoManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void UndoManager::trimUndoStack()
{
    while (undoStack.size() > MAX_UNDO_HISTORY)
    {
        undoStack.erase(undoStack.begin());
    }
}

void UndoManager::notifyListeners()
{
    listeners.call([](Listener& l) { l.undoStateChanged(); });
}

bool UndoManager::tryCoalesce(UndoableAction* action)
{
    if (undoStack.empty())
        return false;

    auto* lastAction = undoStack.back().get();

    // Check time window
    auto timeDiff = action->getTimestamp() - lastAction->getTimestamp();
    if (timeDiff > COALESCE_WINDOW_MS)
        return false;

    // Check if actions can merge
    if (!lastAction->canMergeWith(action))
        return false;

    // Merge the actions
    lastAction->mergeWith(action);
    return true;
}
