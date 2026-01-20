#include "PluginEditorWindow.h"
#include "../LookAndFeel.h"

//==============================================================================
// PluginEditorWindow

PluginEditorWindow::PluginEditorWindow(juce::AudioPluginInstance* plugin,
                                       const juce::String& title)
    : DocumentWindow(title,
                     ProgFlowColours::bgPrimary(),
                     DocumentWindow::closeButton | DocumentWindow::minimiseButton),
      pluginInstance(plugin)
{
    if (pluginInstance && pluginInstance->hasEditor())
    {
        editor.reset(pluginInstance->createEditor());

        if (editor)
        {
            // Check if editor is resizable
            bool editorIsResizable = editor->isResizable();

            setContentOwned(editor.release(), true);

            // Set resizability based on editor
            setResizable(editorIsResizable, false);

            // Constrain to reasonable sizes
            if (auto* constrainer = getConstrainer())
            {
                constrainer->setMinimumSize(200, 150);
                constrainer->setMaximumSize(2000, 1500);
            }
        }
    }
    else
    {
        // No editor available - show a placeholder
        auto* placeholder = new juce::Label();
        placeholder->setText("No editor available for this plugin",
                             juce::dontSendNotification);
        placeholder->setJustificationType(juce::Justification::centred);
        placeholder->setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
        placeholder->setSize(300, 100);
        setContentOwned(placeholder, true);
        setResizable(false, false);
    }

    // Position window
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
    setAlwaysOnTop(false);  // Can be changed to true if desired
    toFront(true);
}

PluginEditorWindow::~PluginEditorWindow()
{
    // Clear the content before destruction
    clearContentComponent();
}

void PluginEditorWindow::closeButtonPressed()
{
    if (onClose)
        onClose();

    // The manager will handle deletion
    setVisible(false);
}

//==============================================================================
// PluginEditorManager

PluginEditorManager::~PluginEditorManager()
{
    closeAllEditors();
}

void PluginEditorManager::showEditorForPlugin(juce::AudioPluginInstance* plugin,
                                               const juce::String& title)
{
    if (!plugin) return;

    // Check if editor already exists
    if (auto* existing = findEditorForPlugin(plugin))
    {
        existing->toFront(true);
        return;
    }

    // Create new editor window
    auto window = std::make_unique<PluginEditorWindow>(plugin, title);

    // Set up close callback to remove from our list
    PluginEditorWindow* windowPtr = window.get();
    window->onClose = [this, windowPtr]() {
        // Find and remove this window
        openEditors.erase(
            std::remove_if(openEditors.begin(), openEditors.end(),
                [windowPtr](const std::unique_ptr<PluginEditorWindow>& w) {
                    return w.get() == windowPtr;
                }),
            openEditors.end()
        );
    };

    openEditors.push_back(std::move(window));
}

void PluginEditorManager::closeEditorForPlugin(juce::AudioPluginInstance* plugin)
{
    openEditors.erase(
        std::remove_if(openEditors.begin(), openEditors.end(),
            [plugin](const std::unique_ptr<PluginEditorWindow>& w) {
                return w->getPlugin() == plugin;
            }),
        openEditors.end()
    );
}

void PluginEditorManager::closeAllEditors()
{
    openEditors.clear();
}

bool PluginEditorManager::isEditorOpen(juce::AudioPluginInstance* plugin) const
{
    return findEditorForPlugin(plugin) != nullptr;
}

PluginEditorWindow* PluginEditorManager::findEditorForPlugin(juce::AudioPluginInstance* plugin) const
{
    for (const auto& window : openEditors)
    {
        if (window->getPlugin() == plugin)
            return window.get();
    }
    return nullptr;
}
