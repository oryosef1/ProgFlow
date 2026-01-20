#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

/**
 * PianoKeyboard - Vertical keyboard for piano roll
 *
 * Displays piano keys vertically with C notes labeled.
 * Click to preview notes.
 */
class PianoKeyboard : public juce::Component
{
public:
    PianoKeyboard();

    void setKeyHeight(int height);
    int getKeyHeight() const { return keyHeight; }

    void setScrollOffset(int offset);  // Sync with grid scroll (in pixels)
    int getScrollOffset() const { return scrollOffset; }

    void paint(juce::Graphics& g) override;

    // Click to preview note
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(int midiNote, float velocity)> onNoteOn;
    std::function<void(int midiNote)> onNoteOff;

private:
    int keyHeight = 16;
    int scrollOffset = 0;
    int pressedNote = -1;

    static constexpr int TOTAL_KEYS = 128;  // Full MIDI range

    int yToMidiNote(int y) const;
    bool isBlackKey(int midiNote) const;
    juce::String getNoteName(int midiNote) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboard)
};
