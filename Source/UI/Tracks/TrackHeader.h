#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Track.h"
#include "../LookAndFeel.h"
#include "../Common/RotaryKnob.h"
#include "../Common/VerticalMeter.h"

/**
 * TrackHeader - A single track row in the track list
 *
 * Shows:
 * - Track color indicator
 * - Track name (editable)
 * - Mute (M) and Solo (S) buttons
 * - Volume fader
 * - Pan knob
 * - VU meter
 */
class TrackHeader : public juce::Component,
                    public juce::Timer
{
public:
    TrackHeader(Track& track);
    ~TrackHeader() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // Selection
    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    // Callbacks
    std::function<void(Track&)> onTrackSelected;
    std::function<void(Track&)> onTrackDeleted;
    std::function<void(Track&, bool expanded)> onAutomationExpandToggled;
    std::function<void(Track&)> onSynthTypeChanged;

    // Get associated track
    Track& getTrack() { return track; }

    // Height constant - must match TimelinePanel::TRACK_HEIGHT for alignment
    static constexpr int defaultHeight = 100;

private:
    Track& track;
    bool selected = false;

    // UI Components
    juce::Label nameLabel;
    juce::ComboBox synthSelector;
    juce::TextButton muteButton{"M"};
    juce::TextButton soloButton{"S"};
    juce::TextButton armButton{"R"};
    juce::TextButton autoButton{"A"};
    juce::TextButton deleteButton{"×"};
    bool automationExpanded = false;
    RotaryKnob volumeKnob;
    RotaryKnob panKnob;
    VerticalMeter meter;

    // Color indicator
    juce::Rectangle<int> colorIndicatorBounds;

    void updateMuteButtonAppearance();
    void updateSoloButtonAppearance();
    void updateArmButtonAppearance();
    void updateAutoButtonAppearance();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeader)
};
