#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <functional>
#include <memory>

/**
 * PluginEditorWindow - A window for hosting plugin editor GUIs
 *
 * Creates a floating window that contains a plugin's native UI.
 * Handles resizing, closing, and proper cleanup.
 */
class PluginEditorWindow : public juce::DocumentWindow
{
public:
    PluginEditorWindow(juce::AudioPluginInstance* plugin,
                       const juce::String& title = "Plugin Editor");
    ~PluginEditorWindow() override;

    // Get the plugin this window is editing
    juce::AudioPluginInstance* getPlugin() const { return pluginInstance; }

    // Callback when window is closed
    std::function<void()> onClose;

    // DocumentWindow overrides
    void closeButtonPressed() override;

private:
    juce::AudioPluginInstance* pluginInstance;
    std::unique_ptr<juce::AudioProcessorEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorWindow)
};

/**
 * PluginEditorManager - Manages multiple plugin editor windows
 *
 * Ensures only one editor window per plugin instance,
 * handles bringing existing windows to front, etc.
 */
class PluginEditorManager
{
public:
    PluginEditorManager() = default;
    ~PluginEditorManager();

    // Show editor for a plugin (creates window if needed, brings to front if exists)
    void showEditorForPlugin(juce::AudioPluginInstance* plugin,
                             const juce::String& title = "Plugin Editor");

    // Close editor for a plugin
    void closeEditorForPlugin(juce::AudioPluginInstance* plugin);

    // Close all editors
    void closeAllEditors();

    // Check if editor is open for a plugin
    bool isEditorOpen(juce::AudioPluginInstance* plugin) const;

    // Get number of open editors
    int getNumOpenEditors() const { return static_cast<int>(openEditors.size()); }

private:
    std::vector<std::unique_ptr<PluginEditorWindow>> openEditors;

    PluginEditorWindow* findEditorForPlugin(juce::AudioPluginInstance* plugin) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditorManager)
};
