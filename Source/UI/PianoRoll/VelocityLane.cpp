#include "VelocityLane.h"
#include "../LookAndFeel.h"

VelocityLane::VelocityLane()
{
}

void VelocityLane::setClip(MidiClip* newClip)
{
    clip = newClip;
    repaint();
}

void VelocityLane::setBeatWidth(int width)
{
    beatWidth = std::max(10, width);
    repaint();
}

void VelocityLane::setSelectedNotes(const std::set<juce::Uuid>& selected)
{
    selectedNotes = selected;
    repaint();
}

void VelocityLane::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(ProgFlowColours::bgSecondary());
    g.fillRect(bounds);

    // Top border
    g.setColour(ProgFlowColours::bgTertiary());
    g.drawHorizontalLine(0, 0.0f, static_cast<float>(bounds.getWidth()));

    if (!clip) return;

    // Draw velocity grid lines
    for (float vel = 0.25f; vel < 1.0f; vel += 0.25f)
    {
        int y = bounds.getHeight() - static_cast<int>(vel * bounds.getHeight());
        g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.3f));
        g.drawHorizontalLine(y, 0.0f, static_cast<float>(bounds.getWidth()));
    }

    // Draw velocity bars for each note
    const auto& notes = clip->getNotes();

    for (const auto& note : notes)
    {
        int x = static_cast<int>(note.startBeat * beatWidth);
        int barWidth = std::max(4, static_cast<int>(note.durationBeats * beatWidth) - 2);
        int barHeight = static_cast<int>(note.velocity * (bounds.getHeight() - 4));

        bool isSelected = selectedNotes.count(note.id) > 0;

        // Bar
        juce::Colour barColour = isSelected ?
            ProgFlowColours::accentBlue() : ProgFlowColours::accentGreen();

        g.setColour(barColour.withAlpha(0.8f));
        g.fillRect(x + 1, bounds.getHeight() - barHeight - 2, barWidth, barHeight);

        // Bar outline
        g.setColour(barColour);
        g.drawRect(x + 1, bounds.getHeight() - barHeight - 2, barWidth, barHeight, 1);
    }
}

void VelocityLane::mouseDown(const juce::MouseEvent& e)
{
    if (!clip) return;

    Note* note = noteAtX(e.x);
    if (note)
    {
        dragNoteId = note->id;
        isDragging = true;

        // Update velocity based on y position
        float newVelocity = yToVelocity(e.y);
        note->velocity = juce::jlimit(0.0f, 1.0f, newVelocity);

        if (onVelocityChanged)
            onVelocityChanged(note->id, note->velocity);

        repaint();
    }
}

void VelocityLane::mouseDrag(const juce::MouseEvent& e)
{
    if (!clip || !isDragging) return;

    Note* note = clip->findNote(dragNoteId);
    if (note)
    {
        float newVelocity = yToVelocity(e.y);
        note->velocity = juce::jlimit(0.0f, 1.0f, newVelocity);

        if (onVelocityChanged)
            onVelocityChanged(note->id, note->velocity);

        repaint();
    }
}

void VelocityLane::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;
}

double VelocityLane::xToBeat(int x) const
{
    return static_cast<double>(x) / beatWidth;
}

float VelocityLane::yToVelocity(int y) const
{
    return 1.0f - (static_cast<float>(y) / getHeight());
}

Note* VelocityLane::noteAtX(int x)
{
    if (!clip) return nullptr;

    double beat = xToBeat(x);
    auto& notes = clip->getNotes();

    for (auto& note : notes)
    {
        if (beat >= note.startBeat && beat < note.getEndBeat())
        {
            return &note;
        }
    }

    return nullptr;
}
