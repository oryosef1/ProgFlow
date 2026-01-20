#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Project/PreferencesManager.h"
#include "../LookAndFeel.h"

/**
 * PreferencesDialog - Settings dialog with tabbed interface
 *
 * Tabs:
 * - Audio: Device selection, sample rate, buffer size
 * - Project: Default BPM, time signature, autosave settings
 * - UI: Theme, meter refresh rate, tooltips
 * - MIDI: Input device, MIDI learn settings
 */
class PreferencesDialog : public juce::Component
{
public:
    PreferencesDialog(juce::AudioDeviceManager& deviceManager);
    ~PreferencesDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Static helper to show dialog
    static void show(juce::AudioDeviceManager& deviceManager, juce::Component* parent);

private:
    juce::AudioDeviceManager& deviceManager;
    PreferencesManager& prefs;

    // Tab bar
    std::unique_ptr<juce::TabbedComponent> tabbedComponent;

    // Audio tab - JUCE's built-in device selector
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioDeviceSelector;

    // Tab components (defined in .cpp file)
    std::unique_ptr<juce::Component> projectTabComp;
    std::unique_ptr<juce::Component> uiTabComp;
    std::unique_ptr<juce::Component> midiTabComp;

    // Buttons
    std::unique_ptr<juce::TextButton> okButton;
    std::unique_ptr<juce::TextButton> cancelButton;
    std::unique_ptr<juce::TextButton> resetButton;

    void loadCurrentSettings();
    void applySettings();
    void resetToDefaults();
    void closeDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreferencesDialog)
};
