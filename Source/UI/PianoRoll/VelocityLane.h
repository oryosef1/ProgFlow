#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/MidiClip.h"
#include <functional>
#include <set>

/**
 * VelocityLane - Velocity editing component below piano roll grid
 *
 * Shows velocity bars for each note in the clip.
 * Drag to edit velocity values.
 */
class VelocityLane : public juce::Component
{
public:
    VelocityLane();

    void setClip(MidiClip* clip);
    MidiClip* getClip() const { return clip; }

    void setBeatWidth(int width);
    int getBeatWidth() const { return beatWidth; }

    void setSelectedNotes(const std::set<juce::Uuid>& selected);

    void paint(juce::Graphics& g) override;

    // Drag to edit velocity
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Callback when velocity changed
    std::function<void(const juce::Uuid&, float)> onVelocityChanged;

private:
    MidiClip* clip = nullptr;
    int beatWidth = 40;
    std::set<juce::Uuid> selectedNotes;

    juce::Uuid dragNoteId;
    bool isDragging = false;

    // Coordinate conversion
    double xToBeat(int x) const;
    float yToVelocity(int y) const;

    // Find note at x position
    Note* noteAtX(int x);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityLane)
};
