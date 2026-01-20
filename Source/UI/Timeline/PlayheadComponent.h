#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * PlayheadComponent - Vertical line showing current playback position
 *
 * This is a transparent overlay component that draws the playhead line
 * at the current transport position.
 */
class PlayheadComponent : public juce::Component
{
public:
    PlayheadComponent();

    void setPosition(int xPosition);
    int getPosition() const { return xPos; }

    void paint(juce::Graphics& g) override;

private:
    int xPos = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayheadComponent)
};
