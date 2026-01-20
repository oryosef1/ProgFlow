#include "PianoKeyboard.h"
#include "../LookAndFeel.h"

PianoKeyboard::PianoKeyboard()
{
}

void PianoKeyboard::setKeyHeight(int height)
{
    keyHeight = std::max(8, height);
    repaint();
}

void PianoKeyboard::setScrollOffset(int offset)
{
    scrollOffset = offset;
    repaint();
}

void PianoKeyboard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(ProgFlowColours::bgSecondary());
    g.fillRect(bounds);

    // Draw keys from top (high notes) to bottom (low notes)
    int totalHeight = TOTAL_KEYS * keyHeight;
    int startY = -scrollOffset;

    for (int note = TOTAL_KEYS - 1; note >= 0; --note)
    {
        int y = startY + (TOTAL_KEYS - 1 - note) * keyHeight;

        // Skip if not visible
        if (y + keyHeight < 0 || y > bounds.getHeight())
            continue;

        bool black = isBlackKey(note);
        bool isC = (note % 12) == 0;
        bool isPressed = (note == pressedNote);

        // Key background
        if (isPressed)
        {
            g.setColour(ProgFlowColours::accentBlue().withAlpha(0.6f));
        }
        else if (black)
        {
            g.setColour(juce::Colour(0xff2a2a2a));
        }
        else
        {
            g.setColour(juce::Colour(0xffe0e0e0));
        }

        g.fillRect(0, y, bounds.getWidth(), keyHeight);

        // Key border
        g.setColour(ProgFlowColours::bgTertiary());
        g.drawHorizontalLine(y + keyHeight - 1, 0.0f, static_cast<float>(bounds.getWidth()));

        // C note labels
        if (isC)
        {
            int octave = (note / 12) - 1;
            g.setColour(black ? ProgFlowColours::textPrimary() : ProgFlowColours::bgPrimary());
            g.setFont(10.0f);
            g.drawText("C" + juce::String(octave), 4, y, bounds.getWidth() - 8, keyHeight,
                      juce::Justification::centredLeft, false);
        }
    }

    // Right border
    g.setColour(ProgFlowColours::bgTertiary());
    g.drawVerticalLine(bounds.getWidth() - 1, 0.0f, static_cast<float>(bounds.getHeight()));
}

void PianoKeyboard::mouseDown(const juce::MouseEvent& e)
{
    int note = yToMidiNote(e.y);
    if (note >= 0 && note < 128)
    {
        pressedNote = note;
        repaint();

        if (onNoteOn)
            onNoteOn(note, 0.8f);
    }
}

void PianoKeyboard::mouseDrag(const juce::MouseEvent& e)
{
    int note = yToMidiNote(e.y);
    if (note != pressedNote && note >= 0 && note < 128)
    {
        // Note off for previous
        if (pressedNote >= 0 && onNoteOff)
            onNoteOff(pressedNote);

        // Note on for new
        pressedNote = note;
        repaint();

        if (onNoteOn)
            onNoteOn(note, 0.8f);
    }
}

void PianoKeyboard::mouseUp(const juce::MouseEvent&)
{
    if (pressedNote >= 0)
    {
        if (onNoteOff)
            onNoteOff(pressedNote);

        pressedNote = -1;
        repaint();
    }
}

int PianoKeyboard::yToMidiNote(int y) const
{
    int adjustedY = y + scrollOffset;
    int noteFromTop = adjustedY / keyHeight;
    return (TOTAL_KEYS - 1) - noteFromTop;
}

bool PianoKeyboard::isBlackKey(int midiNote) const
{
    int noteInOctave = midiNote % 12;
    // Black keys: C#, D#, F#, G#, A# (1, 3, 6, 8, 10)
    return noteInOctave == 1 || noteInOctave == 3 ||
           noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
}

juce::String PianoKeyboard::getNoteName(int midiNote) const
{
    static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    int octave = (midiNote / 12) - 1;
    int noteIndex = midiNote % 12;
    return juce::String(noteNames[noteIndex]) + juce::String(octave);
}
