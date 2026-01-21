#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Audio/AudioEngine.h"
#include "LookAndFeel.h"

/**
 * TransportBar - Top bar with transport controls
 *
 * Contains:
 * - Play/Stop/Record buttons
 * - BPM control
 * - Time signature display
 * - Position display
 * - Metronome toggle
 * - Loop toggle
 * - CPU meter
 */
class TransportBar : public juce::Component,
                     private juce::Timer
{
public:
    explicit TransportBar(AudioEngine& engine);

    // Set the audio device manager for CPU monitoring
    void setAudioDeviceManager(juce::AudioDeviceManager* manager) { deviceManager = manager; }

    // Set project name for display
    void setProjectName(const juce::String& name);
    void setProjectDirty(bool dirty) { projectDirty = dirty; setProjectName(projectName); }

    // Callback for project rename
    std::function<void(const juce::String&)> onProjectRename;

    // Tap tempo (callable from keyboard shortcut)
    void tap() { tapTempoClicked(); }

    // Callback for navigating to project selection
    std::function<void()> onBackToProjectSelection;

    ~TransportBar() override;

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    // Callbacks
    void playClicked();
    void stopClicked();
    void bpmChanged();
    void tapTempoClicked();

    // Update position display
    void updatePositionDisplay();

    // Draw meter helper
    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level, float peak);

    AudioEngine& audioEngine;

    // Transport buttons
    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton recordButton{"Rec"};

    // BPM control
    juce::Label bpmLabel{"bpmLabel", "BPM:"};
    juce::Slider bpmSlider;
    juce::TextButton tapTempoButton{"Tap"};

    // Tap tempo state
    std::vector<juce::int64> tapTimes;
    static constexpr int MAX_TAPS = 8;
    static constexpr juce::int64 TAP_RESET_MS = 2000;  // Reset if >2 seconds between taps

    // Time signature selector
    juce::ComboBox timeSigSelector;
    void timeSigChanged();

    // Position display
    juce::Label positionLabel{"positionLabel", "1:1:000"};

    // Toggles
    juce::ToggleButton metronomeButton{"Metro"};
    juce::ToggleButton countInButton{"Count"};
    juce::ToggleButton loopButton{"Loop"};

    // Master meters
    float meterLevelL = 0.0f;
    float meterLevelR = 0.0f;
    float peakLevelL = 0.0f;
    float peakLevelR = 0.0f;
    int peakHoldCounter = 0;
    static constexpr int PEAK_HOLD_FRAMES = 30;  // Hold peak for ~0.5 seconds at 60fps

    // CPU meter
    juce::AudioDeviceManager* deviceManager = nullptr;
    float cpuUsage = 0.0f;
    juce::Label cpuLabel{"cpuLabel", "CPU: 0%"};

    // Home button (back to project selection)
    juce::TextButton homeButton;

    // Project info
    juce::String projectName = "Untitled";
    bool projectDirty = false;
    juce::Label projectNameLabel;
    void showRenameDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};
