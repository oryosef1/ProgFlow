#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <set>

/**
 * VirtualKeyboardPanel - On-screen clickable piano keyboard
 *
 * Horizontal piano keyboard for triggering notes via mouse clicks.
 * Can be toggled visible/hidden with K key.
 */
class VirtualKeyboardPanel : public juce::Component
{
public:
    VirtualKeyboardPanel();

    // Set octave range to display
    void setOctaveRange(int startOctave, int numOctaves);
    int getStartOctave() const { return startOctave; }
    int getNumOctaves() const { return numOctaves; }

    // Shift octaves up/down
    void shiftOctaveUp();
    void shiftOctaveDown();

    // Show note as pressed (for external MIDI input visualization)
    void setNoteActive(int midiNote, bool active);
    void clearActiveNotes();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(int midiNote, float velocity)> onNoteOn;
    std::function<void(int midiNote)> onNoteOff;

private:
    int startOctave = 3;   // C3
    int numOctaves = 3;    // 3 octaves (C3-B5)
    int pressedNote = -1;
    std::set<int> activeNotes;  // Notes currently being played (for visualization)

    static constexpr int WHITE_KEY_WIDTH = 30;
    static constexpr int BLACK_KEY_WIDTH = 20;
    static constexpr int WHITE_KEY_HEIGHT = 100;
    static constexpr int BLACK_KEY_HEIGHT = 60;

    // Note layout helpers
    bool isBlackKey(int noteInOctave) const;
    int getWhiteKeyIndex(int noteInOctave) const;
    int getMidiNote(int x, int y) const;
    juce::Rectangle<int> getKeyRect(int midiNote) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VirtualKeyboardPanel)
};
