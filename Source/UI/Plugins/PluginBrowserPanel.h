#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../../Audio/PluginHost.h"
#include <functional>
#include <memory>

/**
 * PluginBrowserPanel - UI for browsing and selecting plugins
 *
 * Features:
 * - Filter between Instruments and Effects
 * - Search by name
 * - Display plugin info (name, manufacturer, format)
 * - Double-click or button to load plugin
 * - Rescan button
 */
class PluginBrowserPanel : public juce::Component,
                           public juce::ListBoxModel,
                           public juce::TextEditor::Listener
{
public:
    PluginBrowserPanel(PluginHost& host);
    ~PluginBrowserPanel() override;

    //==========================================================================
    // Filter modes
    enum class FilterMode { All, Instruments, Effects };
    void setFilterMode(FilterMode mode);
    FilterMode getFilterMode() const { return filterMode; }

    //==========================================================================
    // Refresh from PluginHost
    void refreshPluginList();

    //==========================================================================
    // Selection
    const juce::PluginDescription* getSelectedPlugin() const;

    //==========================================================================
    // Callbacks
    std::function<void(const juce::PluginDescription&)> onPluginSelected;
    std::function<void(const juce::PluginDescription&)> onPluginDoubleClicked;

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g,
                          int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;

    //==========================================================================
    // TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor& editor) override;

private:
    PluginHost& pluginHost;

    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::TextEditor> searchBox;
    std::unique_ptr<juce::TextButton> allButton;
    std::unique_ptr<juce::TextButton> instrumentsButton;
    std::unique_ptr<juce::TextButton> effectsButton;
    std::unique_ptr<juce::ListBox> pluginList;
    std::unique_ptr<juce::TextButton> rescanButton;
    std::unique_ptr<juce::Label> statusLabel;

    // Plugin data
    std::vector<juce::PluginDescription> filteredPlugins;
    FilterMode filterMode = FilterMode::All;
    juce::String searchText;

    // Helper methods
    void updateFilteredList();
    void updateFilterButtons();
    void startRescan();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginBrowserPanel)
};
