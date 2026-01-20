#include "NoteGridComponent.h"
#include "../LookAndFeel.h"

NoteGridComponent::NoteGridComponent()
{
}

void NoteGridComponent::setClip(MidiClip* newClip)
{
    clip = newClip;
    selectedNotes.clear();
    updateSize();
    repaint();
}

void NoteGridComponent::setDimensions(int bw, int kh)
{
    beatWidth = std::max(10, bw);
    keyHeight = std::max(8, kh);
    updateSize();
    repaint();
}

void NoteGridComponent::setSnapBeats(double snap)
{
    snapBeats = std::max(0.0625, snap);  // Minimum 1/16 note
}

void NoteGridComponent::setTool(PianoRollTool t)
{
    tool = t;

    // Update cursor based on tool
    switch (tool)
    {
        case PianoRollTool::Draw:
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            break;
        case PianoRollTool::Select:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
        case PianoRollTool::Erase:
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            break;
        case PianoRollTool::Slice:
            setMouseCursor(juce::MouseCursor::CrosshairCursor);
            break;
    }
}

void NoteGridComponent::setSelectedNotes(const std::set<juce::Uuid>& selected)
{
    selectedNotes = selected;
    repaint();
}

void NoteGridComponent::setTrackColour(juce::Colour colour)
{
    trackColour = colour;
    repaint();
}

void NoteGridComponent::setShowGhostNotes(bool show)
{
    showGhostNotes = show;
    repaint();
}

void NoteGridComponent::setGhostNotes(const std::vector<Note>& notes)
{
    ghostNotes = notes;
    if (showGhostNotes)
        repaint();
}

void NoteGridComponent::updateSize()
{
    if (!clip) return;

    int width = static_cast<int>(clip->getDurationBeats() * beatWidth);
    int height = TOTAL_KEYS * keyHeight;
    setSize(width, height);
}

void NoteGridComponent::paint(juce::Graphics& g)
{
    drawGrid(g);
    if (showGhostNotes)
        drawGhostNotes(g);
    drawNotes(g);
    drawSelectionRect(g);
    drawPreviewNote(g);
}

void NoteGridComponent::drawGrid(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background
    g.setColour(ProgFlowColours::bgPrimary());
    g.fillRect(bounds);

    // Horizontal lines (key rows) - alternate shading for black keys
    for (int note = 0; note < TOTAL_KEYS; ++note)
    {
        int y = midiNoteToY(note);
        int noteInOctave = note % 12;

        // Darker background for black key rows
        bool isBlackKey = (noteInOctave == 1 || noteInOctave == 3 ||
                          noteInOctave == 6 || noteInOctave == 8 || noteInOctave == 10);

        if (isBlackKey)
        {
            g.setColour(ProgFlowColours::bgSecondary().withAlpha(0.3f));
            g.fillRect(0, y, bounds.getWidth(), keyHeight);
        }

        // Stronger line at C notes
        if (noteInOctave == 0)
        {
            g.setColour(ProgFlowColours::bgTertiary());
        }
        else
        {
            g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.5f));
        }
        g.drawHorizontalLine(y + keyHeight, 0.0f, static_cast<float>(bounds.getWidth()));
    }

    // Vertical lines (beats)
    if (!clip) return;

    int beatsPerBar = 4;  // Assuming 4/4
    double totalBeats = clip->getDurationBeats();

    for (double beat = 0; beat <= totalBeats; beat += 0.25)
    {
        int x = beatToX(beat);
        int beatIndex = static_cast<int>(beat * 4);  // 16th note index

        // Stronger line at bar boundaries
        if (beatIndex % 16 == 0)
        {
            g.setColour(ProgFlowColours::bgTertiary());
        }
        else if (beatIndex % 4 == 0)
        {
            g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.7f));
        }
        else
        {
            g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.3f));
        }

        g.drawVerticalLine(x, 0.0f, static_cast<float>(bounds.getHeight()));
    }
}

void NoteGridComponent::drawGhostNotes(juce::Graphics& g)
{
    if (ghostNotes.empty()) return;

    // Ghost notes are drawn in a lighter, more transparent color
    juce::Colour ghostColour = trackColour.withAlpha(0.25f);

    for (const auto& note : ghostNotes)
    {
        int x = beatToX(note.startBeat);
        int y = midiNoteToY(note.midiNote);
        int width = std::max(4, static_cast<int>(note.durationBeats * beatWidth));
        int height = keyHeight - 1;

        // Ghost note body (semi-transparent)
        g.setColour(ghostColour);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(width), static_cast<float>(height), 2.0f);

        // Ghost note border (very subtle)
        g.setColour(ghostColour.darker(0.2f));
        g.drawRoundedRectangle(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f,
                               static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f,
                               2.0f, 1.0f);
    }
}

void NoteGridComponent::drawNotes(juce::Graphics& g)
{
    if (!clip) return;

    const auto& notes = clip->getNotes();

    for (const auto& note : notes)
    {
        int x = beatToX(note.startBeat);
        int y = midiNoteToY(note.midiNote);
        int width = std::max(4, static_cast<int>(note.durationBeats * beatWidth));
        int height = keyHeight - 1;

        bool isSelected = selectedNotes.count(note.id) > 0;

        // Note body
        juce::Colour noteColour = trackColour;
        if (isSelected)
            noteColour = noteColour.brighter(0.3f);

        // Velocity affects brightness
        noteColour = noteColour.withMultipliedBrightness(0.5f + note.velocity * 0.5f);

        g.setColour(noteColour);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                               static_cast<float>(width), static_cast<float>(height), 2.0f);

        // Note border
        g.setColour(isSelected ? ProgFlowColours::accentBlue() : noteColour.darker(0.3f));
        g.drawRoundedRectangle(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f,
                               static_cast<float>(width) - 1.0f, static_cast<float>(height) - 1.0f,
                               2.0f, isSelected ? 2.0f : 1.0f);
    }
}

void NoteGridComponent::drawSelectionRect(juce::Graphics& g)
{
    if (dragMode == DragMode::Select && !selectionRect.isEmpty())
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.2f));
        g.fillRect(selectionRect);

        g.setColour(ProgFlowColours::accentBlue());
        g.drawRect(selectionRect, 1);
    }
}

void NoteGridComponent::drawPreviewNote(juce::Graphics& g)
{
    if (!showPreview) return;

    int x = beatToX(previewStartBeat);
    int y = midiNoteToY(previewNote);
    int width = std::max(4, static_cast<int>(previewDuration * beatWidth));
    int height = keyHeight - 1;

    g.setColour(trackColour.withAlpha(0.5f));
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(width), static_cast<float>(height), 2.0f);
}

void NoteGridComponent::mouseDown(const juce::MouseEvent& e)
{
    if (!clip) return;

    dragStartPoint = e.getPosition();

    bool onLeftEdge = false, onRightEdge = false;
    Note* hitNote = noteAtPoint(e.getPosition(), onLeftEdge, onRightEdge);

    if (tool == PianoRollTool::Erase)
    {
        // Erase tool: delete note under cursor
        if (hitNote)
        {
            if (onNoteDeleted)
                onNoteDeleted(hitNote->id);

            clip->removeNote(hitNote->id);
            selectedNotes.erase(hitNote->id);
            repaint();
        }
        return;
    }

    if (tool == PianoRollTool::Draw)
    {
        if (hitNote)
        {
            // Clicked on existing note - select and prepare for move/resize
            selectedNotes.clear();
            selectedNotes.insert(hitNote->id);

            if (onRightEdge)
            {
                dragMode = DragMode::ResizeRight;
            }
            else if (onLeftEdge)
            {
                dragMode = DragMode::ResizeLeft;
            }
            else
            {
                dragMode = DragMode::Move;
            }

            dragNoteId = hitNote->id;
            dragStartBeat = hitNote->startBeat;
            dragStartDuration = hitNote->durationBeats;
            dragStartMidiNote = hitNote->midiNote;

            if (onSelectionChanged)
                onSelectionChanged(selectedNotes);
        }
        else
        {
            // Draw new note
            dragMode = DragMode::Draw;

            previewNote = yToMidiNote(e.y);
            previewStartBeat = snapBeat(xToBeat(e.x));
            previewDuration = snapBeats;
            showPreview = true;

            if (onNotePreview)
                onNotePreview(previewNote, 0.8f);
        }
        repaint();
        return;
    }

    if (tool == PianoRollTool::Select)
    {
        if (hitNote)
        {
            // Toggle selection with shift, otherwise replace
            if (e.mods.isShiftDown())
            {
                if (selectedNotes.count(hitNote->id))
                    selectedNotes.erase(hitNote->id);
                else
                    selectedNotes.insert(hitNote->id);
            }
            else
            {
                if (!selectedNotes.count(hitNote->id))
                {
                    selectedNotes.clear();
                    selectedNotes.insert(hitNote->id);
                }
                dragMode = DragMode::Move;
                dragNoteId = hitNote->id;
                dragStartBeat = hitNote->startBeat;
                dragStartDuration = hitNote->durationBeats;
                dragStartMidiNote = hitNote->midiNote;
            }

            if (onSelectionChanged)
                onSelectionChanged(selectedNotes);
        }
        else
        {
            // Start selection rectangle
            if (!e.mods.isShiftDown())
                selectedNotes.clear();

            dragMode = DragMode::Select;
            selectionRect = juce::Rectangle<int>();
        }
        repaint();
    }
}

void NoteGridComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (!clip) return;

    if (dragMode == DragMode::Draw)
    {
        // Update preview duration
        double endBeat = snapBeat(xToBeat(e.x));
        previewDuration = std::max(snapBeats, endBeat - previewStartBeat);
        repaint();
    }
    else if (dragMode == DragMode::Move)
    {
        // Move selected notes
        double deltaBeat = snapBeat(xToBeat(e.x)) - snapBeat(xToBeat(dragStartPoint.x));
        int deltaNote = yToMidiNote(e.y) - yToMidiNote(dragStartPoint.y);

        for (const auto& noteId : selectedNotes)
        {
            Note* note = clip->findNote(noteId);
            if (note)
            {
                if (noteId == dragNoteId)
                {
                    note->startBeat = std::max(0.0, dragStartBeat + deltaBeat);
                    note->midiNote = juce::jlimit(0, 127, dragStartMidiNote + deltaNote);
                }
            }
        }
        repaint();
    }
    else if (dragMode == DragMode::ResizeRight)
    {
        Note* note = clip->findNote(dragNoteId);
        if (note)
        {
            double endBeat = snapBeat(xToBeat(e.x));
            note->durationBeats = std::max(snapBeats, endBeat - note->startBeat);
            repaint();
        }
    }
    else if (dragMode == DragMode::ResizeLeft)
    {
        Note* note = clip->findNote(dragNoteId);
        if (note)
        {
            double newStart = snapBeat(xToBeat(e.x));
            double endBeat = dragStartBeat + dragStartDuration;
            newStart = std::min(newStart, endBeat - snapBeats);
            newStart = std::max(0.0, newStart);

            note->startBeat = newStart;
            note->durationBeats = endBeat - newStart;
            repaint();
        }
    }
    else if (dragMode == DragMode::Select)
    {
        // Update selection rectangle
        int x1 = std::min(dragStartPoint.x, e.x);
        int y1 = std::min(dragStartPoint.y, e.y);
        int x2 = std::max(dragStartPoint.x, e.x);
        int y2 = std::max(dragStartPoint.y, e.y);
        selectionRect = juce::Rectangle<int>(x1, y1, x2 - x1, y2 - y1);

        updateSelectionFromRect();
        repaint();
    }
}

void NoteGridComponent::mouseUp(const juce::MouseEvent& e)
{
    if (!clip) return;

    if (dragMode == DragMode::Draw && showPreview)
    {
        // Create the note
        Note newNote;
        newNote.id = juce::Uuid();
        newNote.midiNote = previewNote;
        newNote.startBeat = previewStartBeat;
        newNote.durationBeats = previewDuration;
        newNote.velocity = 0.8f;

        clip->addNote(newNote);

        if (onNoteAdded)
            onNoteAdded(newNote);

        if (onNotePreviewEnd)
            onNotePreviewEnd(previewNote);

        showPreview = false;
    }
    else if (dragMode == DragMode::Move || dragMode == DragMode::ResizeLeft ||
             dragMode == DragMode::ResizeRight)
    {
        // Notify about updates
        for (const auto& noteId : selectedNotes)
        {
            Note* note = clip->findNote(noteId);
            if (note && onNoteUpdated)
                onNoteUpdated(noteId, *note);
        }
    }

    dragMode = DragMode::None;
    selectionRect = juce::Rectangle<int>();
    repaint();
}

void NoteGridComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (!clip) return;

    bool onLeft, onRight;
    Note* hitNote = noteAtPoint(e.getPosition(), onLeft, onRight);

    if (hitNote)
    {
        // Delete note on double-click
        if (onNoteDeleted)
            onNoteDeleted(hitNote->id);

        clip->removeNote(hitNote->id);
        selectedNotes.erase(hitNote->id);
        repaint();
    }
}

int NoteGridComponent::midiNoteToY(int midiNote) const
{
    return (TOTAL_KEYS - 1 - midiNote) * keyHeight;
}

int NoteGridComponent::yToMidiNote(int y) const
{
    return (TOTAL_KEYS - 1) - (y / keyHeight);
}

double NoteGridComponent::xToBeat(int x) const
{
    return static_cast<double>(x) / beatWidth;
}

int NoteGridComponent::beatToX(double beat) const
{
    return static_cast<int>(beat * beatWidth);
}

double NoteGridComponent::snapBeat(double beat) const
{
    if (snapBeats <= 0) return beat;
    return std::round(beat / snapBeats) * snapBeats;
}

Note* NoteGridComponent::noteAtPoint(juce::Point<int> point, bool& onLeftEdge, bool& onRightEdge)
{
    onLeftEdge = false;
    onRightEdge = false;

    if (!clip) return nullptr;

    auto& notes = clip->getNotes();
    int clickedNote = yToMidiNote(point.y);

    for (auto& note : notes)
    {
        if (note.midiNote != clickedNote) continue;

        int x1 = beatToX(note.startBeat);
        int x2 = beatToX(note.getEndBeat());

        if (point.x >= x1 && point.x < x2)
        {
            const int edgeWidth = 8;
            onLeftEdge = (point.x - x1) < edgeWidth;
            onRightEdge = (x2 - point.x) < edgeWidth;
            return &note;
        }
    }

    return nullptr;
}

void NoteGridComponent::updateSelectionFromRect()
{
    if (!clip) return;

    selectedNotes.clear();

    double startBeat = xToBeat(selectionRect.getX());
    double endBeat = xToBeat(selectionRect.getRight());
    int topNote = yToMidiNote(selectionRect.getY());
    int bottomNote = yToMidiNote(selectionRect.getBottom());

    for (const auto& note : clip->getNotes())
    {
        if (note.midiNote >= bottomNote && note.midiNote <= topNote)
        {
            if (note.getEndBeat() > startBeat && note.startBeat < endBeat)
            {
                selectedNotes.insert(note.id);
            }
        }
    }

    if (onSelectionChanged)
        onSelectionChanged(selectedNotes);
}
