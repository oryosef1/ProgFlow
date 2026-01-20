#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/AudioEngine.h"
#include "ChannelStrip.h"
#include "../LookAndFeel.h"

/**
 * MixerPanel - Full mixer view with channel strips
 *
 * Layout:
 * - Track channel strips (horizontally scrollable)
 * - Master channel strip on the right
 */
class MixerPanel : public juce::Component,
                    public juce::Timer
{
public:
    MixerPanel(AudioEngine& audioEngine);
    ~MixerPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // Rebuild channel strips from AudioEngine
    void refreshTracks();

    // Selection callback
    std::function<void(Track*)> onTrackSelected;

private:
    AudioEngine& audioEngine;
    Track* selectedTrack = nullptr;

    // Viewport for scrollable track strips
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> stripContainer;
    std::vector<std::unique_ptr<ChannelStrip>> channelStrips;

    // Master channel strip (always visible on right)
    std::unique_ptr<ChannelStrip> masterStrip;

    void selectTrack(Track* track);

    static constexpr int masterStripWidth = 90;
    static constexpr int stripSpacing = 4;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerPanel)
};
