#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <set>
#include "Audio/AudioEngine.h"
#include "Audio/PluginHost.h"
#include "Project/ProjectManager.h"
#include "UI/LookAndFeel.h"
#include "UI/TransportBar.h"
#include "UI/Synths/AnalogSynthEditor.h"
#include "UI/Synths/FMSynthEditor.h"
#include "UI/Synths/PolyPadSynthEditor.h"
#include "UI/Synths/OrganSynthEditor.h"
#include "UI/Synths/StringSynthEditor.h"
#include "UI/Synths/ProSynthEditor.h"
#include "UI/Synths/SamplerEditor.h"
#include "UI/Synths/SoundFontPlayerEditor.h"
#include "UI/Synths/DrumSynthEditor.h"
#include "Audio/Synths/SynthFactory.h"
#include "UI/Timeline/TimelinePanel.h"
#include "UI/PianoRoll/PianoRollEditor.h"
#include "UI/Tracks/TrackHeaderPanel.h"
#include "UI/Mixer/MixerPanel.h"
#include "UI/Plugins/PluginBrowserPanel.h"
#include "UI/Plugins/PluginEditorWindow.h"
#include "UI/Dialogs/ExportDialog.h"
#include "UI/Dialogs/PreferencesDialog.h"
#include "UI/WelcomeScreen.h"
#include "UI/VirtualKeyboardPanel.h"
#include "UI/ToastManager.h"

// Forward declaration
class MainWindow;

class MainContentComponent : public juce::Component,
                              public juce::KeyListener,
                              public juce::MenuBarModel,
                              public ProjectManager::Listener,
                              public ThemeManager::Listener
{
public:
    MainContentComponent();
    ~MainContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;

    // MenuBarModel
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    // ProjectManager::Listener
    void projectStateChanged() override;
    void projectLoaded() override;
    void projectSaved() override;

    // ThemeManager::Listener
    void themeChanged() override;

    // Set parent window reference for title updates
    void setParentWindow(MainWindow* window) { parentWindow = window; }

    // Project manager access
    ProjectManager& getProjectManager() { return *projectManager; }

    // File operations (for menu and shortcuts)
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void exportAudio();

private:
    // Keyboard to MIDI mapping (QWERTY piano)
    int keyToMidiNote(int keyCode) const;
    std::set<int> keysDown; // Track which keys are held
    // Audio
    AudioEngine audioEngine;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;

    // UI Components
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<TimelinePanel> timelinePanel;
    std::unique_ptr<PianoRollEditor> pianoRollEditor;
    std::unique_ptr<juce::Component> synthEditor;  // Generic synth editor (type depends on selected track)
    juce::TooltipWindow tooltipWindow{this, 500};  // 500ms delay before showing

    // Track list panel
    std::unique_ptr<TrackHeaderPanel> trackHeaderPanel;

    // Currently selected track for synth editing
    Track* selectedTrack = nullptr;
    SynthType currentSynthEditorType = SynthType::Analog;

    // Mixer panel
    std::unique_ptr<MixerPanel> mixerPanel;

    // Plugin hosting
    std::unique_ptr<PluginHost> pluginHost;
    std::unique_ptr<PluginBrowserPanel> pluginBrowser;
    std::unique_ptr<PluginEditorManager> pluginEditorManager;
    bool showingPluginBrowser = false;

    // Welcome screen (shown when no project is loaded)
    std::unique_ptr<WelcomeScreen> welcomeScreen;
    bool showingWelcomeScreen = true;

    // Virtual keyboard (toggled with K key)
    std::unique_ptr<VirtualKeyboardPanel> virtualKeyboard;
    bool showingVirtualKeyboard = false;

    // Toast notifications
    std::unique_ptr<ToastManager> toastManager;

    // Main view mode: Arrange or Mixer
    enum class MainViewMode { Arrange, Mixer };
    MainViewMode mainViewMode = MainViewMode::Arrange;

    // Bottom panel mode: synth editor or piano roll (when in Arrange view)
    enum class BottomPanelMode { SynthEditor, PianoRoll };
    BottomPanelMode bottomPanelMode = BottomPanelMode::SynthEditor;

    // Custom look and feel
    ProgFlowLookAndFeel lookAndFeel;

    // Project management
    std::unique_ptr<ProjectManager> projectManager;
    MainWindow* parentWindow = nullptr;

    // Menu item IDs
    enum MenuItemIDs
    {
        NewProject = 1,
        OpenProject,
        OpenRecentBase = 100, // 100-109 reserved for recent files
        Save = 200,
        SaveAs,
        ExportAudio,
        Undo = 250,
        Redo,
        Preferences = 300,
        Quit
    };

    // Helpers
    void hideWelcomeScreen();
    void addNewTrack();
    void openPianoRoll(MidiClip* clip);
    void showSynthEditor();
    void toggleMixerView();
    void showArrangeView();
    void showMixerView();
    void selectTrack(Track* track);
    void updateSynthEditorForTrack(Track* track);

    // Plugin helpers
    void togglePluginBrowser();
    void showPluginBrowser();
    void hidePluginBrowser();
    void loadPluginOnSelectedTrack(const juce::PluginDescription& desc);
    void showPluginEditor(juce::AudioPluginInstance* plugin, const juce::String& name);

    // Virtual keyboard helpers
    void toggleVirtualKeyboard();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};

class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow(const juce::String& name);
    ~MainWindow() override = default;

    void closeButtonPressed() override;

    // Update window title from project state
    void updateTitle();

    // Access to content component
    MainContentComponent* getMainComponent() { return mainComponent; }

private:
    MainContentComponent* mainComponent = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};
