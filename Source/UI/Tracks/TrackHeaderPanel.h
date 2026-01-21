#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/AudioEngine.h"
#include "TrackHeader.h"
#include "../LookAndFeel.h"

/**
 * TrackHeaderPanel - The track list panel on the left side
 *
 * Contains:
 * - Header with "Tracks" label and add track button
 * - Scrollable list of TrackHeader components
 * - Master track at bottom (optional)
 */
class TrackHeaderPanel : public juce::Component,
                          public juce::Timer
{
public:
    TrackHeaderPanel(AudioEngine& audioEngine);
    ~TrackHeaderPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Rebuild track list from AudioEngine
    void refreshTracks();

    // Callbacks
    std::function<void(Track*)> onTrackSelected;
    std::function<void()> onTracksChanged;  // Called when tracks are added/removed
    std::function<void()> onBackToProjectSelection;  // Called to go back to welcome screen

    // Get currently selected track
    Track* getSelectedTrack() const { return selectedTrack; }

private:
    AudioEngine& audioEngine;
    Track* selectedTrack = nullptr;

    // Header area
    juce::Label titleLabel{"title", "Tracks"};
    juce::TextButton addTrackButton{"+"};

    // Viewport for scrollable track list
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> trackListContainer;
    std::vector<std::unique_ptr<TrackHeader>> trackHeaders;

    // Master meter (optional)
    VerticalMeter masterMeterL;
    VerticalMeter masterMeterR;
    juce::Label masterLabel{"master", "Master"};

    // Home button to go back to project selection
    juce::TextButton homeButton;

    void selectTrack(Track* track);
    void addNewTrack();
    void deleteTrack(Track* track);

    static constexpr int headerHeight = 30;  // Must match TimelinePanel::RULER_HEIGHT for alignment
    static constexpr int masterHeight = 80;  // Includes home button

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderPanel)
};
