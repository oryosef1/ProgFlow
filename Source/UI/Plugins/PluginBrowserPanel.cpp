#include "PluginBrowserPanel.h"
#include "../LookAndFeel.h"

PluginBrowserPanel::PluginBrowserPanel(PluginHost& host)
    : pluginHost(host)
{
    // Title
    titleLabel = std::make_unique<juce::Label>("", "Plugin Browser");
    titleLabel->setFont(juce::FontOptions(16.0f).withStyle("Bold"));
    titleLabel->setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    addAndMakeVisible(*titleLabel);

    // Search box
    searchBox = std::make_unique<juce::TextEditor>();
    searchBox->setTextToShowWhenEmpty("Search plugins...", ProgFlowColours::textSecondary());
    searchBox->addListener(this);
    searchBox->setColour(juce::TextEditor::backgroundColourId, ProgFlowColours::bgSecondary());
    searchBox->setColour(juce::TextEditor::textColourId, ProgFlowColours::textPrimary());
    searchBox->setColour(juce::TextEditor::outlineColourId, ProgFlowColours::bgTertiary());
    addAndMakeVisible(*searchBox);

    // Filter buttons
    allButton = std::make_unique<juce::TextButton>("All");
    allButton->setClickingTogglesState(true);
    allButton->setToggleState(true, juce::dontSendNotification);
    allButton->onClick = [this]() { setFilterMode(FilterMode::All); };
    addAndMakeVisible(*allButton);

    instrumentsButton = std::make_unique<juce::TextButton>("Instruments");
    instrumentsButton->setClickingTogglesState(true);
    instrumentsButton->onClick = [this]() { setFilterMode(FilterMode::Instruments); };
    addAndMakeVisible(*instrumentsButton);

    effectsButton = std::make_unique<juce::TextButton>("Effects");
    effectsButton->setClickingTogglesState(true);
    effectsButton->onClick = [this]() { setFilterMode(FilterMode::Effects); };
    addAndMakeVisible(*effectsButton);

    // Plugin list
    pluginList = std::make_unique<juce::ListBox>("Plugins", this);
    pluginList->setColour(juce::ListBox::backgroundColourId, ProgFlowColours::bgSecondary());
    pluginList->setRowHeight(40);
    pluginList->setMultipleSelectionEnabled(false);
    addAndMakeVisible(*pluginList);

    // Rescan button
    rescanButton = std::make_unique<juce::TextButton>("Rescan Plugins");
    rescanButton->onClick = [this]() { startRescan(); };
    addAndMakeVisible(*rescanButton);

    // Status label
    statusLabel = std::make_unique<juce::Label>();
    statusLabel->setFont(juce::FontOptions(11.0f));
    statusLabel->setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    addAndMakeVisible(*statusLabel);

    // Connect to plugin host callbacks
    pluginHost.onPluginListChanged = [this]() {
        juce::MessageManager::callAsync([this]() {
            refreshPluginList();
        });
    };

    pluginHost.onScanProgress = [this](const juce::String& pluginName) {
        juce::MessageManager::callAsync([this, pluginName]() {
            statusLabel->setText("Scanning: " + pluginName, juce::dontSendNotification);
        });
    };

    // Initial load
    refreshPluginList();
}

PluginBrowserPanel::~PluginBrowserPanel()
{
    // Clear callbacks
    pluginHost.onPluginListChanged = nullptr;
    pluginHost.onScanProgress = nullptr;
}

void PluginBrowserPanel::setFilterMode(FilterMode mode)
{
    filterMode = mode;
    updateFilterButtons();
    updateFilteredList();
}

void PluginBrowserPanel::refreshPluginList()
{
    updateFilteredList();

    int numPlugins = static_cast<int>(pluginHost.getKnownPlugins().getNumTypes());
    int numInstruments = static_cast<int>(pluginHost.getInstruments().size());
    int numEffects = static_cast<int>(pluginHost.getEffects().size());

    statusLabel->setText(juce::String(numPlugins) + " plugins (" +
                         juce::String(numInstruments) + " instruments, " +
                         juce::String(numEffects) + " effects)",
                         juce::dontSendNotification);
}

const juce::PluginDescription* PluginBrowserPanel::getSelectedPlugin() const
{
    int selectedRow = pluginList->getSelectedRow();
    if (selectedRow >= 0 && selectedRow < static_cast<int>(filteredPlugins.size()))
    {
        return &filteredPlugins[static_cast<size_t>(selectedRow)];
    }
    return nullptr;
}

void PluginBrowserPanel::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());
}

void PluginBrowserPanel::resized()
{
    auto bounds = getLocalBounds().reduced(8);

    // Title row
    auto titleRow = bounds.removeFromTop(24);
    titleLabel->setBounds(titleRow);
    bounds.removeFromTop(8);

    // Search box
    searchBox->setBounds(bounds.removeFromTop(28));
    bounds.removeFromTop(8);

    // Filter buttons row
    auto filterRow = bounds.removeFromTop(28);
    int buttonWidth = (filterRow.getWidth() - 8) / 3;
    allButton->setBounds(filterRow.removeFromLeft(buttonWidth));
    filterRow.removeFromLeft(4);
    instrumentsButton->setBounds(filterRow.removeFromLeft(buttonWidth));
    filterRow.removeFromLeft(4);
    effectsButton->setBounds(filterRow);
    bounds.removeFromTop(8);

    // Bottom row with rescan button and status
    auto bottomRow = bounds.removeFromBottom(28);
    rescanButton->setBounds(bottomRow.removeFromLeft(120));
    bottomRow.removeFromLeft(8);
    statusLabel->setBounds(bottomRow);
    bounds.removeFromBottom(8);

    // Plugin list takes remaining space
    pluginList->setBounds(bounds);
}

int PluginBrowserPanel::getNumRows()
{
    return static_cast<int>(filteredPlugins.size());
}

void PluginBrowserPanel::paintListBoxItem(int rowNumber, juce::Graphics& g,
                                          int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= static_cast<int>(filteredPlugins.size()))
        return;

    const auto& plugin = filteredPlugins[static_cast<size_t>(rowNumber)];

    // Background
    if (rowIsSelected)
        g.fillAll(ProgFlowColours::accentBlue().withAlpha(0.3f));
    else if (rowNumber % 2 == 1)
        g.fillAll(ProgFlowColours::bgTertiary().withAlpha(0.3f));

    // Plugin name
    g.setColour(ProgFlowColours::textPrimary());
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(plugin.name, 8, 2, width - 16, 18, juce::Justification::left);

    // Plugin info (manufacturer, format, type)
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(juce::FontOptions(11.0f));

    juce::String info = plugin.manufacturerName;
    if (info.isNotEmpty())
        info += " | ";
    info += plugin.pluginFormatName;
    info += plugin.isInstrument ? " | Instrument" : " | Effect";

    g.drawText(info, 8, 20, width - 16, 16, juce::Justification::left);

    // Bottom separator line
    g.setColour(ProgFlowColours::bgTertiary());
    g.drawHorizontalLine(height - 1, 0.0f, static_cast<float>(width));
}

void PluginBrowserPanel::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < static_cast<int>(filteredPlugins.size()))
    {
        if (onPluginSelected)
            onPluginSelected(filteredPlugins[static_cast<size_t>(lastRowSelected)]);
    }
}

void PluginBrowserPanel::listBoxItemDoubleClicked(int row, const juce::MouseEvent&)
{
    if (row >= 0 && row < static_cast<int>(filteredPlugins.size()))
    {
        if (onPluginDoubleClicked)
            onPluginDoubleClicked(filteredPlugins[static_cast<size_t>(row)]);
    }
}

void PluginBrowserPanel::textEditorTextChanged(juce::TextEditor& editor)
{
    if (&editor == searchBox.get())
    {
        searchText = editor.getText();
        updateFilteredList();
    }
}

void PluginBrowserPanel::updateFilteredList()
{
    filteredPlugins.clear();

    // Get base list based on filter mode
    std::vector<juce::PluginDescription> baseList;
    switch (filterMode)
    {
        case FilterMode::All:
            baseList = pluginHost.getAllPlugins();
            break;
        case FilterMode::Instruments:
            baseList = pluginHost.getInstruments();
            break;
        case FilterMode::Effects:
            baseList = pluginHost.getEffects();
            break;
    }

    // Apply search filter
    if (searchText.isEmpty())
    {
        filteredPlugins = baseList;
    }
    else
    {
        juce::String search = searchText.toLowerCase();
        for (const auto& plugin : baseList)
        {
            if (plugin.name.toLowerCase().contains(search) ||
                plugin.manufacturerName.toLowerCase().contains(search))
            {
                filteredPlugins.push_back(plugin);
            }
        }
    }

    pluginList->updateContent();
    pluginList->repaint();
}

void PluginBrowserPanel::updateFilterButtons()
{
    allButton->setToggleState(filterMode == FilterMode::All, juce::dontSendNotification);
    instrumentsButton->setToggleState(filterMode == FilterMode::Instruments, juce::dontSendNotification);
    effectsButton->setToggleState(filterMode == FilterMode::Effects, juce::dontSendNotification);
}

void PluginBrowserPanel::startRescan()
{
    rescanButton->setEnabled(false);
    statusLabel->setText("Scanning for plugins...", juce::dontSendNotification);

    pluginHost.scanForPluginsAsync([this]() {
        juce::MessageManager::callAsync([this]() {
            rescanButton->setEnabled(true);
            refreshPluginList();
        });
    });
}
