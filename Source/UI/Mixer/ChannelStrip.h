#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Track.h"
#include "../../Audio/AudioEngine.h"
#include "../LookAndFeel.h"
#include "../Common/RotaryKnob.h"
#include "../Common/VerticalMeter.h"

/**
 * ChannelStrip - A vertical mixer channel strip
 *
 * Layout (top to bottom):
 * - Track name
 * - Pan knob
 * - Mute/Solo buttons
 * - Volume fader
 * - Meter
 */
class ChannelStrip : public juce::Component,
                      public juce::Timer
{
public:
    /** Constructor for track channel strip */
    ChannelStrip(Track& track);

    /** Constructor for master channel strip */
    ChannelStrip(AudioEngine& engine);

    ~ChannelStrip() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Selection
    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    // Get associated track (nullptr for master)
    Track* getTrack() { return track; }

    // Callbacks
    std::function<void(Track*)> onTrackSelected;

    // Dimensions
    static constexpr int defaultWidth = 80;

private:
    Track* track = nullptr;
    AudioEngine* audioEngine = nullptr;
    bool isMaster = false;
    bool selected = false;

    // UI Components
    juce::Label nameLabel;
    RotaryKnob panKnob;
    juce::TextButton muteButton{"M"};
    juce::TextButton soloButton{"S"};
    juce::Slider volumeFader;
    VerticalMeter meterL;
    VerticalMeter meterR;

    void setupComponents();
    void updateMuteButtonAppearance();
    void updateSoloButtonAppearance();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};
