#include "VirtualKeyboardPanel.h"
#include "LookAndFeel.h"

VirtualKeyboardPanel::VirtualKeyboardPanel()
{
}

void VirtualKeyboardPanel::setOctaveRange(int start, int num)
{
    startOctave = juce::jlimit(0, 8, start);
    numOctaves = juce::jlimit(1, 5, num);
    repaint();
}

void VirtualKeyboardPanel::shiftOctaveUp()
{
    if (startOctave < 7)
    {
        startOctave++;
        repaint();
    }
}

void VirtualKeyboardPanel::shiftOctaveDown()
{
    if (startOctave > 0)
    {
        startOctave--;
        repaint();
    }
}

void VirtualKeyboardPanel::setNoteActive(int midiNote, bool active)
{
    if (active)
        activeNotes.insert(midiNote);
    else
        activeNotes.erase(midiNote);
    repaint();
}

void VirtualKeyboardPanel::clearActiveNotes()
{
    activeNotes.clear();
    repaint();
}

bool VirtualKeyboardPanel::isBlackKey(int noteInOctave) const
{
    // Black keys: C#, D#, F#, G#, A# (1, 3, 6, 8, 10)
    return noteInOctave == 1 || noteInOctave == 3 ||
           noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10;
}

int VirtualKeyboardPanel::getWhiteKeyIndex(int noteInOctave) const
{
    // Map note to white key position (0-6)
    static const int map[] = { 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 };
    return map[noteInOctave];
}

juce::Rectangle<int> VirtualKeyboardPanel::getKeyRect(int midiNote) const
{
    int noteInOctave = midiNote % 12;
    int octave = midiNote / 12;
    int octaveOffset = octave - startOctave;

    if (octaveOffset < 0 || octaveOffset >= numOctaves)
        return {};

    int whiteKeyIndex = getWhiteKeyIndex(noteInOctave);
    int baseX = octaveOffset * 7 * WHITE_KEY_WIDTH;

    if (isBlackKey(noteInOctave))
    {
        // Black key position - offset from white key
        int whiteX = baseX + whiteKeyIndex * WHITE_KEY_WIDTH;
        int blackX = whiteX + WHITE_KEY_WIDTH - BLACK_KEY_WIDTH / 2;
        return juce::Rectangle<int>(blackX, 0, BLACK_KEY_WIDTH, BLACK_KEY_HEIGHT);
    }
    else
    {
        // White key
        return juce::Rectangle<int>(baseX + whiteKeyIndex * WHITE_KEY_WIDTH, 0,
                                    WHITE_KEY_WIDTH, WHITE_KEY_HEIGHT);
    }
}

int VirtualKeyboardPanel::getMidiNote(int x, int y) const
{
    int totalWhiteKeys = numOctaves * 7;

    // First check black keys (they're on top)
    if (y < BLACK_KEY_HEIGHT)
    {
        for (int oct = 0; oct < numOctaves; ++oct)
        {
            int baseMidi = (startOctave + oct) * 12;
            for (int note = 0; note < 12; ++note)
            {
                if (isBlackKey(note))
                {
                    auto rect = getKeyRect(baseMidi + note);
                    if (rect.contains(x, y))
                        return baseMidi + note;
                }
            }
        }
    }

    // Check white keys
    int whiteKeyIndex = x / WHITE_KEY_WIDTH;
    if (whiteKeyIndex < 0 || whiteKeyIndex >= totalWhiteKeys)
        return -1;

    int octave = whiteKeyIndex / 7;
    int keyInOctave = whiteKeyIndex % 7;

    // Map white key index to note
    static const int whiteToNote[] = { 0, 2, 4, 5, 7, 9, 11 };  // C, D, E, F, G, A, B
    int midiNote = (startOctave + octave) * 12 + whiteToNote[keyInOctave];

    return midiNote;
}

void VirtualKeyboardPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(ProgFlowColours::bgSecondary());
    g.fillRect(bounds);

    int totalWhiteKeys = numOctaves * 7;

    // Draw white keys first
    for (int oct = 0; oct < numOctaves; ++oct)
    {
        int baseMidi = (startOctave + oct) * 12;
        for (int note = 0; note < 12; ++note)
        {
            if (!isBlackKey(note))
            {
                int midiNote = baseMidi + note;
                auto rect = getKeyRect(midiNote);
                if (rect.isEmpty()) continue;

                bool isActive = (midiNote == pressedNote) || activeNotes.count(midiNote) > 0;
                bool isC = (note == 0);

                if (isActive)
                    g.setColour(ProgFlowColours::accentBlue().withAlpha(0.7f));
                else
                    g.setColour(juce::Colour(0xfffafafa));

                g.fillRect(rect.reduced(1));

                // Key border
                g.setColour(juce::Colour(0xffcccccc));
                g.drawRect(rect.reduced(1));

                // C note labels
                if (isC)
                {
                    int octaveNum = midiNote / 12 - 1;
                    g.setColour(juce::Colour(0xff666666));
                    g.setFont(11.0f);
                    g.drawText("C" + juce::String(octaveNum),
                              rect.withTrimmedTop(rect.getHeight() - 20),
                              juce::Justification::centred, false);
                }
            }
        }
    }

    // Draw black keys on top
    for (int oct = 0; oct < numOctaves; ++oct)
    {
        int baseMidi = (startOctave + oct) * 12;
        for (int note = 0; note < 12; ++note)
        {
            if (isBlackKey(note))
            {
                int midiNote = baseMidi + note;
                auto rect = getKeyRect(midiNote);
                if (rect.isEmpty()) continue;

                bool isActive = (midiNote == pressedNote) || activeNotes.count(midiNote) > 0;

                if (isActive)
                    g.setColour(ProgFlowColours::accentBlue().withAlpha(0.8f));
                else
                    g.setColour(juce::Colour(0xff1a1a1a));

                g.fillRect(rect);

                // Subtle highlight at top
                g.setColour(juce::Colour(0xff3a3a3a));
                g.fillRect(rect.withHeight(3));
            }
        }
    }

    // Draw octave indicator and controls
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(12.0f);

    int controlAreaX = totalWhiteKeys * WHITE_KEY_WIDTH + 10;
    g.drawText("Oct: " + juce::String(startOctave),
              controlAreaX, 10, 60, 20, juce::Justification::left, false);

    // Octave shift buttons (visual indicator only - actual control via Z/X keys)
    g.setColour(ProgFlowColours::bgTertiary());
    g.fillRoundedRectangle(static_cast<float>(controlAreaX), 35.0f, 25.0f, 25.0f, 4.0f);
    g.fillRoundedRectangle(static_cast<float>(controlAreaX + 30), 35.0f, 25.0f, 25.0f, 4.0f);

    g.setColour(ProgFlowColours::textPrimary());
    g.drawText("-", controlAreaX, 35, 25, 25, juce::Justification::centred, false);
    g.drawText("+", controlAreaX + 30, 35, 25, 25, juce::Justification::centred, false);

    // Instructions
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(10.0f);
    g.drawText("Z/X: Octave",
              controlAreaX, 65, 60, 16, juce::Justification::left, false);
    g.drawText("K: Hide",
              controlAreaX, 80, 60, 16, juce::Justification::left, false);
}

void VirtualKeyboardPanel::resized()
{
    // Component size is fixed based on octave range
}

void VirtualKeyboardPanel::mouseDown(const juce::MouseEvent& e)
{
    // Check for octave button clicks
    int totalWhiteKeys = numOctaves * 7;
    int controlAreaX = totalWhiteKeys * WHITE_KEY_WIDTH + 10;

    if (e.x >= controlAreaX && e.x < controlAreaX + 25 && e.y >= 35 && e.y < 60)
    {
        shiftOctaveDown();
        return;
    }
    if (e.x >= controlAreaX + 30 && e.x < controlAreaX + 55 && e.y >= 35 && e.y < 60)
    {
        shiftOctaveUp();
        return;
    }

    int note = getMidiNote(e.x, e.y);
    if (note >= 0 && note < 128)
    {
        pressedNote = note;
        repaint();

        if (onNoteOn)
            onNoteOn(note, 0.8f);
    }
}

void VirtualKeyboardPanel::mouseDrag(const juce::MouseEvent& e)
{
    int note = getMidiNote(e.x, e.y);
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

void VirtualKeyboardPanel::mouseUp(const juce::MouseEvent&)
{
    if (pressedNote >= 0)
    {
        if (onNoteOff)
            onNoteOff(pressedNote);

        pressedNote = -1;
        repaint();
    }
}
