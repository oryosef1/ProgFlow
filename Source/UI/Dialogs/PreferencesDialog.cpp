#include "PreferencesDialog.h"

//==============================================================================
// Project Tab Component

class ProjectTabComponent : public juce::Component
{
public:
    ProjectTabComponent()
    {
        bpmLabel.setText("Default BPM:", juce::dontSendNotification);
        addAndMakeVisible(bpmLabel);

        bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        bpmSlider.setRange(20.0, 300.0, 1.0);
        bpmSlider.setValue(120.0);
        addAndMakeVisible(bpmSlider);

        timeSigLabel.setText("Default Time Signature:", juce::dontSendNotification);
        addAndMakeVisible(timeSigLabel);

        for (int i = 1; i <= 16; i++)
            timeSigNumCombo.addItem(juce::String(i), i);
        timeSigNumCombo.setSelectedId(4);
        addAndMakeVisible(timeSigNumCombo);

        slashLabel.setText("/", juce::dontSendNotification);
        slashLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(slashLabel);

        timeSigDenomCombo.addItem("4", 4);
        timeSigDenomCombo.addItem("8", 8);
        timeSigDenomCombo.addItem("16", 16);
        timeSigDenomCombo.setSelectedId(4);
        addAndMakeVisible(timeSigDenomCombo);

        autosaveLabel.setText("Autosave:", juce::dontSendNotification);
        addAndMakeVisible(autosaveLabel);

        autosaveToggle.setButtonText("Enabled");
        autosaveToggle.setToggleState(true, juce::dontSendNotification);
        autosaveToggle.onStateChange = [this]() {
            autosaveIntervalSlider.setEnabled(autosaveToggle.getToggleState());
        };
        addAndMakeVisible(autosaveToggle);

        autosaveIntervalSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        autosaveIntervalSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        autosaveIntervalSlider.setRange(1.0, 30.0, 1.0);
        autosaveIntervalSlider.setValue(2.0);
        autosaveIntervalSlider.setTextValueSuffix(" min");
        addAndMakeVisible(autosaveIntervalSlider);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ProgFlowColours::bgSecondary());
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);
        const int rowHeight = 30;
        const int labelWidth = 150;
        const int spacing = 15;

        auto bpmRow = bounds.removeFromTop(rowHeight);
        bpmLabel.setBounds(bpmRow.removeFromLeft(labelWidth));
        bpmSlider.setBounds(bpmRow);
        bounds.removeFromTop(spacing);

        auto tsRow = bounds.removeFromTop(rowHeight);
        timeSigLabel.setBounds(tsRow.removeFromLeft(labelWidth));
        timeSigNumCombo.setBounds(tsRow.removeFromLeft(60));
        slashLabel.setBounds(tsRow.removeFromLeft(20));
        timeSigDenomCombo.setBounds(tsRow.removeFromLeft(60));
        bounds.removeFromTop(spacing);

        auto asRow = bounds.removeFromTop(rowHeight);
        autosaveLabel.setBounds(asRow.removeFromLeft(labelWidth));
        autosaveToggle.setBounds(asRow.removeFromLeft(100));
        asRow.removeFromLeft(10);
        autosaveIntervalSlider.setBounds(asRow);
    }

    juce::Label bpmLabel;
    juce::Slider bpmSlider;
    juce::Label timeSigLabel;
    juce::ComboBox timeSigNumCombo;
    juce::Label slashLabel;
    juce::ComboBox timeSigDenomCombo;
    juce::Label autosaveLabel;
    juce::ToggleButton autosaveToggle;
    juce::Slider autosaveIntervalSlider;
};

//==============================================================================
// UI Tab Component

class UITabComponent : public juce::Component
{
public:
    UITabComponent()
    {
        themeLabel.setText("Theme:", juce::dontSendNotification);
        addAndMakeVisible(themeLabel);

        themeCombo.addItem("Dark", 1);
        themeCombo.addItem("Light", 2);
        themeCombo.setSelectedId(1);
        addAndMakeVisible(themeCombo);

        meterRateLabel.setText("Meter Refresh Rate:", juce::dontSendNotification);
        addAndMakeVisible(meterRateLabel);

        meterRateSlider.setSliderStyle(juce::Slider::LinearHorizontal);
        meterRateSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
        meterRateSlider.setRange(10.0, 60.0, 1.0);
        meterRateSlider.setValue(30.0);
        meterRateSlider.setTextValueSuffix(" Hz");
        addAndMakeVisible(meterRateSlider);

        tooltipsToggle.setButtonText("Show Tooltips");
        tooltipsToggle.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(tooltipsToggle);

        cpuMeterToggle.setButtonText("Show CPU Meter");
        cpuMeterToggle.setToggleState(true, juce::dontSendNotification);
        addAndMakeVisible(cpuMeterToggle);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ProgFlowColours::bgSecondary());
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);
        const int rowHeight = 30;
        const int labelWidth = 150;
        const int spacing = 15;

        auto themeRow = bounds.removeFromTop(rowHeight);
        themeLabel.setBounds(themeRow.removeFromLeft(labelWidth));
        themeCombo.setBounds(themeRow.removeFromLeft(120));
        bounds.removeFromTop(spacing);

        auto mrRow = bounds.removeFromTop(rowHeight);
        meterRateLabel.setBounds(mrRow.removeFromLeft(labelWidth));
        meterRateSlider.setBounds(mrRow);
        bounds.removeFromTop(spacing);

        tooltipsToggle.setBounds(bounds.removeFromTop(rowHeight));
        bounds.removeFromTop(spacing);

        cpuMeterToggle.setBounds(bounds.removeFromTop(rowHeight));
    }

    juce::Label themeLabel;
    juce::ComboBox themeCombo;
    juce::Label meterRateLabel;
    juce::Slider meterRateSlider;
    juce::ToggleButton tooltipsToggle;
    juce::ToggleButton cpuMeterToggle;
};

//==============================================================================
// MIDI Tab Component

class MidiTabComponent : public juce::Component
{
public:
    MidiTabComponent()
    {
        midiInputLabel.setText("MIDI Input Device:", juce::dontSendNotification);
        addAndMakeVisible(midiInputLabel);

        midiInputCombo.addItem("(None)", 1);
        int id = 2;
        for (auto& device : juce::MidiInput::getAvailableDevices())
            midiInputCombo.addItem(device.name, id++);
        midiInputCombo.setSelectedId(1);
        addAndMakeVisible(midiInputCombo);

        midiLearnToggle.setButtonText("Enable MIDI Learn Mode");
        midiLearnToggle.setToggleState(false, juce::dontSendNotification);
        addAndMakeVisible(midiLearnToggle);
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(ProgFlowColours::bgSecondary());
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(20);
        const int rowHeight = 30;
        const int labelWidth = 150;
        const int spacing = 15;

        auto miRow = bounds.removeFromTop(rowHeight);
        midiInputLabel.setBounds(miRow.removeFromLeft(labelWidth));
        midiInputCombo.setBounds(miRow);
        bounds.removeFromTop(spacing);

        midiLearnToggle.setBounds(bounds.removeFromTop(rowHeight));
    }

    juce::Label midiInputLabel;
    juce::ComboBox midiInputCombo;
    juce::ToggleButton midiLearnToggle;
};

//==============================================================================

PreferencesDialog::PreferencesDialog(juce::AudioDeviceManager& dm)
    : deviceManager(dm),
      prefs(PreferencesManager::getInstance())
{
    // Create tabbed component
    tabbedComponent = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    tabbedComponent->setTabBarDepth(30);
    addAndMakeVisible(tabbedComponent.get());

    // Audio tab - use JUCE's built-in audio device selector
    audioDeviceSelector = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        0, 2,   // min/max input channels
        0, 2,   // min/max output channels
        false,  // show MIDI inputs
        false,  // show MIDI outputs
        false,  // show channels as stereo pairs
        false   // hide advanced options
    );
    tabbedComponent->addTab("Audio", ProgFlowColours::bgSecondary(), audioDeviceSelector.get(), false);

    // Create other tabs
    projectTabComp = std::make_unique<ProjectTabComponent>();
    tabbedComponent->addTab("Project", ProgFlowColours::bgSecondary(), projectTabComp.get(), false);

    uiTabComp = std::make_unique<UITabComponent>();
    tabbedComponent->addTab("UI", ProgFlowColours::bgSecondary(), uiTabComp.get(), false);

    midiTabComp = std::make_unique<MidiTabComponent>();
    tabbedComponent->addTab("MIDI", ProgFlowColours::bgSecondary(), midiTabComp.get(), false);

    // OK/Cancel/Reset buttons
    okButton = std::make_unique<juce::TextButton>("OK");
    okButton->onClick = [this]() {
        applySettings();
        closeDialog();
    };
    addAndMakeVisible(okButton.get());

    cancelButton = std::make_unique<juce::TextButton>("Cancel");
    cancelButton->onClick = [this]() { closeDialog(); };
    addAndMakeVisible(cancelButton.get());

    resetButton = std::make_unique<juce::TextButton>("Reset to Defaults");
    resetButton->onClick = [this]() { resetToDefaults(); };
    addAndMakeVisible(resetButton.get());

    // Load current settings
    loadCurrentSettings();

    setSize(550, 450);
}

PreferencesDialog::~PreferencesDialog()
{
}

void PreferencesDialog::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());
    g.setColour(ProgFlowColours::border());
    g.drawRect(getLocalBounds(), 1);
}

void PreferencesDialog::resized()
{
    auto bounds = getLocalBounds().reduced(10);

    // Buttons at bottom
    auto buttonArea = bounds.removeFromBottom(35);
    bounds.removeFromBottom(10);

    okButton->setBounds(buttonArea.removeFromRight(80));
    buttonArea.removeFromRight(10);
    cancelButton->setBounds(buttonArea.removeFromRight(80));
    buttonArea.removeFromRight(20);
    resetButton->setBounds(buttonArea.removeFromLeft(140));

    // Tabs fill the rest
    tabbedComponent->setBounds(bounds);
}

void PreferencesDialog::loadCurrentSettings()
{
    auto* projTab = static_cast<ProjectTabComponent*>(projectTabComp.get());
    projTab->bpmSlider.setValue(prefs.getDefaultBpm());
    projTab->timeSigNumCombo.setSelectedId(prefs.getDefaultTimeSignatureNumerator());
    projTab->timeSigDenomCombo.setSelectedId(prefs.getDefaultTimeSignatureDenominator());
    projTab->autosaveToggle.setToggleState(prefs.getAutosaveEnabled(), juce::dontSendNotification);
    projTab->autosaveIntervalSlider.setValue(prefs.getAutosaveIntervalMinutes());
    projTab->autosaveIntervalSlider.setEnabled(prefs.getAutosaveEnabled());

    auto* uiTab = static_cast<UITabComponent*>(uiTabComp.get());
    uiTab->themeCombo.setSelectedId(static_cast<int>(prefs.getTheme()) + 1);
    uiTab->meterRateSlider.setValue(prefs.getMeterRefreshRateHz());
    uiTab->tooltipsToggle.setToggleState(prefs.getShowTooltips(), juce::dontSendNotification);
    uiTab->cpuMeterToggle.setToggleState(prefs.getShowCpuMeter(), juce::dontSendNotification);

    auto* midiTab = static_cast<MidiTabComponent*>(midiTabComp.get());
    auto currentMidiDevice = prefs.getMidiInputDevice();
    if (currentMidiDevice.isEmpty())
    {
        midiTab->midiInputCombo.setSelectedId(1);
    }
    else
    {
        int id = 2;
        for (auto& device : juce::MidiInput::getAvailableDevices())
        {
            if (device.name == currentMidiDevice)
            {
                midiTab->midiInputCombo.setSelectedId(id);
                break;
            }
            id++;
        }
    }
    midiTab->midiLearnToggle.setToggleState(prefs.getMidiLearnEnabled(), juce::dontSendNotification);
}

void PreferencesDialog::applySettings()
{
    auto* projTab = static_cast<ProjectTabComponent*>(projectTabComp.get());
    prefs.setDefaultBpm(projTab->bpmSlider.getValue());
    prefs.setDefaultTimeSignatureNumerator(projTab->timeSigNumCombo.getSelectedId());
    prefs.setDefaultTimeSignatureDenominator(projTab->timeSigDenomCombo.getSelectedId());
    prefs.setAutosaveEnabled(projTab->autosaveToggle.getToggleState());
    prefs.setAutosaveIntervalMinutes(static_cast<int>(projTab->autosaveIntervalSlider.getValue()));

    auto* uiTab = static_cast<UITabComponent*>(uiTabComp.get());
    prefs.setTheme(static_cast<PreferencesManager::Theme>(uiTab->themeCombo.getSelectedId() - 1));
    prefs.setMeterRefreshRateHz(static_cast<int>(uiTab->meterRateSlider.getValue()));
    prefs.setShowTooltips(uiTab->tooltipsToggle.getToggleState());
    prefs.setShowCpuMeter(uiTab->cpuMeterToggle.getToggleState());

    auto* midiTab = static_cast<MidiTabComponent*>(midiTabComp.get());
    int midiIdx = midiTab->midiInputCombo.getSelectedId();
    if (midiIdx == 1)
    {
        prefs.setMidiInputDevice("");
    }
    else
    {
        auto devices = juce::MidiInput::getAvailableDevices();
        if (midiIdx - 2 < static_cast<int>(devices.size()))
            prefs.setMidiInputDevice(devices[midiIdx - 2].name);
    }
    prefs.setMidiLearnEnabled(midiTab->midiLearnToggle.getToggleState());

    prefs.saveIfNeeded();
}

void PreferencesDialog::resetToDefaults()
{
    prefs.resetToDefaults();
    loadCurrentSettings();
}

void PreferencesDialog::closeDialog()
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
        dw->exitModalState(0);
}

//==============================================================================
// Static show method

void PreferencesDialog::show(juce::AudioDeviceManager& deviceManager, juce::Component* parent)
{
    auto* dialog = new PreferencesDialog(deviceManager);

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Preferences";
    options.dialogBackgroundColour = ProgFlowColours::bgPrimary();
    options.content.setOwned(dialog);
    options.componentToCentreAround = parent;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}
