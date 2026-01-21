#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <set>
#include <random>
#include <vector>
#include "Audio/AudioEngine.h"
#include "Audio/PluginHost.h"
#include "Project/ProjectManager.h"
#include "UI/LookAndFeel.h"
#include "UI/TransportBar.h"
#include "UI/Synths/AnalogSynthEditor.h"
#include "UI/Synths/FMSynthEditor.h"
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
                              public ThemeManager::Listener,
                              public juce::Timer
{
public:
    MainContentComponent();
    ~MainContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

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

    // Timer for background animations
    void timerCallback() override;

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
    // Background animation system
    struct Particle {
        float x, y;       // Position (0-1 normalized)
        float vx, vy;     // Velocity
        float size;       // Radius
        float alpha;      // Opacity
    };
    std::vector<Particle> bgParticles;
    float animationTime = 0.0f;
    std::mt19937 rng{std::random_device{}()};

    void initBackgroundAnimation();
    void updateBackgroundAnimation();
    void drawBackgroundAnimation(juce::Graphics& g);

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
    juce::TooltipWindow tooltipWindow{this, 150};  // 150ms delay - faster tooltips

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

    // Resizable bottom panel
    int bottomPanelHeight = 350;  // Taller default to fit synth editors
    static constexpr int MIN_BOTTOM_PANEL_HEIGHT = 350;  // Min needed for ProSynth layout
    static constexpr int MAX_BOTTOM_PANEL_HEIGHT = 600;
    static constexpr int RESIZE_HANDLE_HEIGHT = 8;

    // Resize handle component
    class ResizeHandle : public juce::Component
    {
    public:
        std::function<void(int)> onResize;  // Called during drag with delta from last position

        ResizeHandle() { setMouseCursor(juce::MouseCursor::UpDownResizeCursor); }

        void paint(juce::Graphics& g) override
        {
            // Background
            g.setColour(juce::Colour(0xff1a1f26));
            g.fillRect(getLocalBounds());

            // Draw 3 dots as handle indicator
            g.setColour(isDragging ? juce::Colour(0xff4C9EFF) : juce::Colour(0x60888888));
            int centreX = getWidth() / 2;
            int centreY = getHeight() / 2;
            for (int i = -1; i <= 1; ++i)
                g.fillEllipse(float(centreX + i * 12 - 2), float(centreY - 2), 4.0f, 4.0f);
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            isDragging = true;
            lastDragY = e.getScreenY();
            accumulatedDelta = 0;
            repaint();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            // Live resize during drag with threshold to reduce jitter
            if (isDragging && onResize)
            {
                int delta = lastDragY - e.getScreenY();
                accumulatedDelta += delta;
                lastDragY = e.getScreenY();

                // Only update when accumulated delta is significant (reduces flicker)
                if (std::abs(accumulatedDelta) >= 4)
                {
                    onResize(accumulatedDelta);
                    accumulatedDelta = 0;
                }
            }
        }

        void mouseUp(const juce::MouseEvent&) override
        {
            // Apply any remaining delta
            if (isDragging && onResize && accumulatedDelta != 0)
            {
                onResize(accumulatedDelta);
            }
            isDragging = false;
            accumulatedDelta = 0;
            repaint();
        }

    private:
        bool isDragging = false;
        int lastDragY = 0;
        int accumulatedDelta = 0;
    };

    ResizeHandle resizeHandle;

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
    void showWelcomeScreen();
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
