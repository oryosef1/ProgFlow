#include "PianoRollEditor.h"
#include "../LookAndFeel.h"
#include <limits>

PianoRollEditor::PianoRollEditor(AudioEngine& engine)
    : audioEngine(engine)
{
    // Create toolbar
    createToolbar();

    // Create piano keyboard
    keyboard = std::make_unique<PianoKeyboard>();
    keyboard->setKeyHeight(getKeyHeight());
    keyboard->onNoteOn = [this](int note, float vel) {
        if (onNotePreview) onNotePreview(note, vel);
        audioEngine.synthNoteOn(note, vel);
    };
    keyboard->onNoteOff = [this](int note) {
        if (onNotePreviewEnd) onNotePreviewEnd(note);
        audioEngine.synthNoteOff(note);
    };
    addAndMakeVisible(*keyboard);

    // Create note grid
    noteGrid = std::make_unique<NoteGridComponent>();
    noteGrid->setDimensions(getBeatWidth(), getKeyHeight());
    noteGrid->setSnapBeats(getSnapInBeats());
    noteGrid->setTool(currentTool);
    noteGrid->onNotePreview = [this](int note, float vel) {
        if (onNotePreview) onNotePreview(note, vel);
        audioEngine.synthNoteOn(note, vel);
    };
    noteGrid->onNotePreviewEnd = [this](int note) {
        if (onNotePreviewEnd) onNotePreviewEnd(note);
        audioEngine.synthNoteOff(note);
    };
    noteGrid->onSelectionChanged = [this](const std::set<juce::Uuid>& sel) {
        selectedNotes = sel;
        velocityLane->setSelectedNotes(sel);
    };

    // Create viewport for grid
    gridViewport = std::make_unique<juce::Viewport>();
    gridViewport->setViewedComponent(noteGrid.get(), false);
    gridViewport->setScrollBarsShown(true, true);
    gridViewport->getVerticalScrollBar().addListener(
        static_cast<juce::ScrollBar::Listener*>(nullptr));  // We'll sync manually
    addAndMakeVisible(*gridViewport);

    // Create velocity lane
    velocityLane = std::make_unique<VelocityLane>();
    velocityLane->setBeatWidth(getBeatWidth());
    addAndMakeVisible(*velocityLane);

    // Register as key listener
    addKeyListener(this);
    setWantsKeyboardFocus(true);
}

PianoRollEditor::~PianoRollEditor()
{
    removeKeyListener(this);
}

void PianoRollEditor::createToolbar()
{
    toolbar = std::make_unique<juce::Component>();
    addAndMakeVisible(*toolbar);

    // Draw tool button
    drawButton = std::make_unique<juce::TextButton>("Draw");
    drawButton->setClickingTogglesState(true);
    drawButton->setToggleState(true, juce::dontSendNotification);
    drawButton->onClick = [this]() { handleToolChange(PianoRollTool::Draw); };
    toolbar->addAndMakeVisible(*drawButton);

    // Select tool button
    selectButton = std::make_unique<juce::TextButton>("Select");
    selectButton->setClickingTogglesState(true);
    selectButton->onClick = [this]() { handleToolChange(PianoRollTool::Select); };
    toolbar->addAndMakeVisible(*selectButton);

    // Erase tool button
    eraseButton = std::make_unique<juce::TextButton>("Erase");
    eraseButton->setClickingTogglesState(true);
    eraseButton->onClick = [this]() { handleToolChange(PianoRollTool::Erase); };
    toolbar->addAndMakeVisible(*eraseButton);

    // Snap combo
    snapCombo = std::make_unique<juce::ComboBox>();
    snapCombo->addItem("Off", 1);
    snapCombo->addItem("1 Bar", 2);
    snapCombo->addItem("1 Beat", 3);
    snapCombo->addItem("1/8", 4);
    snapCombo->addItem("1/16", 5);
    snapCombo->addItem("1/32", 6);
    snapCombo->setSelectedId(4);  // Default to 1/8
    snapCombo->onChange = [this]() { handleSnapChange(); };
    toolbar->addAndMakeVisible(*snapCombo);
}

void PianoRollEditor::setClip(MidiClip* clip)
{
    currentClip = clip;
    selectedNotes.clear();

    noteGrid->setClip(clip);
    noteGrid->setTrackColour(trackColour);
    velocityLane->setClip(clip);
    velocityLane->setSelectedNotes({});

    // Scroll to middle C area
    if (gridViewport && noteGrid)
    {
        int c4Y = noteGrid->getHeight() / 2 - gridViewport->getHeight() / 2;
        gridViewport->setViewPosition(0, c4Y);
        keyboard->setScrollOffset(c4Y);
    }

    repaint();
}

void PianoRollEditor::setTrackColour(juce::Colour colour)
{
    trackColour = colour;
    noteGrid->setTrackColour(colour);
}

void PianoRollEditor::setTrack(Track* track)
{
    currentTrack = track;
    updateGhostNotes();
}

void PianoRollEditor::setShowGhostNotes(bool show)
{
    showGhostNotes = show;
    noteGrid->setShowGhostNotes(show);
    updateGhostNotes();
    DBG("Ghost notes " << (show ? "enabled" : "disabled"));
}

void PianoRollEditor::updateGhostNotes()
{
    std::vector<Note> ghostNotes;

    if (showGhostNotes && currentTrack && currentClip)
    {
        // Get the current clip's position
        double currentClipStart = currentClip->getStartBeat();
        double currentClipEnd = currentClip->getEndBeat();

        // Collect notes from other clips on the same track
        const auto& clips = currentTrack->getClips();
        for (const auto& clip : clips)
        {
            // Skip the current clip
            if (clip.get() == currentClip)
                continue;

            double clipStart = clip->getStartBeat();
            double clipEnd = clip->getEndBeat();

            // Check if clip overlaps or is adjacent (within 16 beats)
            double tolerance = 16.0;
            if (clipEnd < currentClipStart - tolerance || clipStart > currentClipEnd + tolerance)
                continue;

            // Add notes from this clip, adjusted to current clip's timeline
            for (const auto& note : clip->getNotes())
            {
                Note ghostNote = note;
                // Adjust position relative to current clip
                ghostNote.startBeat = (clipStart + note.startBeat) - currentClipStart;
                ghostNotes.push_back(ghostNote);
            }
        }
    }

    noteGrid->setGhostNotes(ghostNotes);
}

void PianoRollEditor::setTool(PianoRollTool tool)
{
    currentTool = tool;
    noteGrid->setTool(tool);
    updateToolButtons();
}

void PianoRollEditor::setSnap(SnapValue snap)
{
    currentSnap = snap;
    noteGrid->setSnapBeats(getSnapInBeats());

    // Update combo box
    int id = static_cast<int>(snap) + 1;
    snapCombo->setSelectedId(id, juce::dontSendNotification);
}

double PianoRollEditor::getSnapInBeats() const
{
    switch (currentSnap)
    {
        case SnapValue::Off:          return 0.0;
        case SnapValue::Bar:          return 4.0;
        case SnapValue::Beat:         return 1.0;
        case SnapValue::Eighth:       return 0.5;
        case SnapValue::Sixteenth:    return 0.25;
        case SnapValue::ThirtySecond: return 0.125;
    }
    return 0.5;
}

void PianoRollEditor::setZoomX(float zoom)
{
    zoomX = juce::jlimit(0.25f, 4.0f, zoom);
    noteGrid->setDimensions(getBeatWidth(), getKeyHeight());
    velocityLane->setBeatWidth(getBeatWidth());
}

void PianoRollEditor::setZoomY(float zoom)
{
    zoomY = juce::jlimit(0.5f, 2.0f, zoom);
    noteGrid->setDimensions(getBeatWidth(), getKeyHeight());
    keyboard->setKeyHeight(getKeyHeight());
}

void PianoRollEditor::selectAll()
{
    if (!currentClip) return;

    selectedNotes.clear();
    for (const auto& note : currentClip->getNotes())
    {
        selectedNotes.insert(note.id);
    }

    noteGrid->setSelectedNotes(selectedNotes);
    velocityLane->setSelectedNotes(selectedNotes);
}

void PianoRollEditor::deleteSelected()
{
    if (!currentClip) return;

    for (const auto& id : selectedNotes)
    {
        currentClip->removeNote(id);
    }

    selectedNotes.clear();
    noteGrid->setSelectedNotes({});
    velocityLane->setSelectedNotes({});
    noteGrid->repaint();
    velocityLane->repaint();
}

void PianoRollEditor::quantizeSelected()
{
    if (!currentClip) return;

    double snap = getSnapInBeats();
    if (snap <= 0) return;

    for (const auto& id : selectedNotes)
    {
        Note* note = currentClip->findNote(id);
        if (note)
        {
            note->startBeat = std::round(note->startBeat / snap) * snap;
            note->durationBeats = std::max(snap,
                std::round(note->durationBeats / snap) * snap);
        }
    }

    noteGrid->repaint();
}

void PianoRollEditor::transposeSelected(int semitones)
{
    if (!currentClip) return;

    for (const auto& id : selectedNotes)
    {
        Note* note = currentClip->findNote(id);
        if (note)
        {
            note->midiNote = juce::jlimit(0, 127, note->midiNote + semitones);
        }
    }

    noteGrid->repaint();
}

void PianoRollEditor::copySelected()
{
    if (!currentClip || selectedNotes.empty()) return;

    clipboard.clear();

    // Find the earliest start beat to use as reference point
    double earliestBeat = std::numeric_limits<double>::max();
    for (const auto& id : selectedNotes)
    {
        Note* note = currentClip->findNote(id);
        if (note && note->startBeat < earliestBeat)
            earliestBeat = note->startBeat;
    }

    // Store notes with positions relative to earliest note
    for (const auto& id : selectedNotes)
    {
        Note* note = currentClip->findNote(id);
        if (note)
        {
            ClipboardNote cn;
            cn.midiNote = note->midiNote;
            cn.startBeat = note->startBeat - earliestBeat;  // Relative position
            cn.durationBeats = note->durationBeats;
            cn.velocity = note->velocity;
            clipboard.push_back(cn);
        }
    }
}

void PianoRollEditor::cutSelected()
{
    copySelected();
    deleteSelected();
}

void PianoRollEditor::paste()
{
    if (!currentClip || clipboard.empty()) return;

    // Paste at current playhead position (or use 0 if not available)
    double pastePosition = audioEngine.getPositionInBeats();

    // Clear current selection and prepare for new notes
    selectedNotes.clear();

    // Create new notes from clipboard
    for (const auto& cn : clipboard)
    {
        Note newNote;
        newNote.id = juce::Uuid();
        newNote.midiNote = cn.midiNote;
        newNote.startBeat = pastePosition + cn.startBeat;
        newNote.durationBeats = cn.durationBeats;
        newNote.velocity = cn.velocity;

        currentClip->addNote(newNote);
        selectedNotes.insert(newNote.id);
    }

    noteGrid->setSelectedNotes(selectedNotes);
    velocityLane->setSelectedNotes(selectedNotes);
    noteGrid->repaint();
    velocityLane->repaint();
}

void PianoRollEditor::duplicateSelected()
{
    if (!currentClip || selectedNotes.empty()) return;

    // Find the selection bounds
    double earliestBeat = std::numeric_limits<double>::max();
    double latestEndBeat = 0.0;

    for (const auto& id : selectedNotes)
    {
        Note* note = currentClip->findNote(id);
        if (note)
        {
            if (note->startBeat < earliestBeat)
                earliestBeat = note->startBeat;
            if (note->getEndBeat() > latestEndBeat)
                latestEndBeat = note->getEndBeat();
        }
    }

    double selectionDuration = latestEndBeat - earliestBeat;

    // Store the old selection to copy from
    std::vector<Note> notesToDuplicate;
    for (const auto& id : selectedNotes)
    {
        Note* note = currentClip->findNote(id);
        if (note)
            notesToDuplicate.push_back(*note);
    }

    // Clear selection and add duplicated notes
    selectedNotes.clear();

    for (const auto& original : notesToDuplicate)
    {
        Note newNote;
        newNote.id = juce::Uuid();
        newNote.midiNote = original.midiNote;
        newNote.startBeat = original.startBeat + selectionDuration;  // Place right after original
        newNote.durationBeats = original.durationBeats;
        newNote.velocity = original.velocity;

        currentClip->addNote(newNote);
        selectedNotes.insert(newNote.id);
    }

    noteGrid->setSelectedNotes(selectedNotes);
    velocityLane->setSelectedNotes(selectedNotes);
    noteGrid->repaint();
    velocityLane->repaint();
}

void PianoRollEditor::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());
}

void PianoRollEditor::resized()
{
    auto bounds = getLocalBounds();

    // Toolbar at top
    auto toolbarBounds = bounds.removeFromTop(TOOLBAR_HEIGHT);
    toolbar->setBounds(toolbarBounds);

    // Layout toolbar contents
    int buttonWidth = 60;
    int margin = 8;
    drawButton->setBounds(margin, 4, buttonWidth, TOOLBAR_HEIGHT - 8);
    selectButton->setBounds(margin + buttonWidth + 4, 4, buttonWidth, TOOLBAR_HEIGHT - 8);
    eraseButton->setBounds(margin + 2 * (buttonWidth + 4), 4, buttonWidth, TOOLBAR_HEIGHT - 8);
    snapCombo->setBounds(margin + 3 * (buttonWidth + 4) + 20, 4, 80, TOOLBAR_HEIGHT - 8);

    // Velocity lane at bottom
    auto velocityBounds = bounds.removeFromBottom(VELOCITY_LANE_HEIGHT);
    velocityBounds.removeFromLeft(KEYBOARD_WIDTH);
    velocityLane->setBounds(velocityBounds);

    // Keyboard on left
    keyboard->setBounds(bounds.removeFromLeft(KEYBOARD_WIDTH));

    // Grid viewport takes remaining space
    gridViewport->setBounds(bounds);

    // Sync scroll
    syncScrollPositions();
}

bool PianoRollEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    // Tool shortcuts
    if (key == juce::KeyPress('d'))
    {
        setTool(PianoRollTool::Draw);
        return true;
    }
    if (key == juce::KeyPress('s'))
    {
        setTool(PianoRollTool::Select);
        return true;
    }
    if (key == juce::KeyPress('e'))
    {
        setTool(PianoRollTool::Erase);
        return true;
    }

    // Delete
    if (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)
    {
        deleteSelected();
        return true;
    }

    // Select all (Cmd+A)
    if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0))
    {
        selectAll();
        return true;
    }

    // Copy (Cmd+C)
    if (key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
    {
        copySelected();
        return true;
    }

    // Cut (Cmd+X)
    if (key == juce::KeyPress('x', juce::ModifierKeys::commandModifier, 0))
    {
        cutSelected();
        return true;
    }

    // Paste (Cmd+V)
    if (key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0))
    {
        paste();
        return true;
    }

    // Duplicate (Cmd+D)
    if (key == juce::KeyPress('d', juce::ModifierKeys::commandModifier, 0))
    {
        duplicateSelected();
        return true;
    }

    // Transpose
    if (key == juce::KeyPress::upKey)
    {
        transposeSelected(key.getModifiers().isShiftDown() ? 12 : 1);
        return true;
    }
    if (key == juce::KeyPress::downKey)
    {
        transposeSelected(key.getModifiers().isShiftDown() ? -12 : -1);
        return true;
    }

    // Quantize
    if (key == juce::KeyPress('q'))
    {
        quantizeSelected();
        return true;
    }

    // Ghost notes toggle
    if (key == juce::KeyPress('g'))
    {
        toggleGhostNotes();
        return true;
    }

    return false;
}

int PianoRollEditor::getKeyHeight() const
{
    return static_cast<int>(BASE_KEY_HEIGHT * zoomY);
}

int PianoRollEditor::getBeatWidth() const
{
    return static_cast<int>(BASE_BEAT_WIDTH * zoomX);
}

void PianoRollEditor::updateToolButtons()
{
    drawButton->setToggleState(currentTool == PianoRollTool::Draw, juce::dontSendNotification);
    selectButton->setToggleState(currentTool == PianoRollTool::Select, juce::dontSendNotification);
    eraseButton->setToggleState(currentTool == PianoRollTool::Erase, juce::dontSendNotification);
}

void PianoRollEditor::handleToolChange(PianoRollTool tool)
{
    setTool(tool);
}

void PianoRollEditor::handleSnapChange()
{
    int id = snapCombo->getSelectedId();
    if (id >= 1 && id <= 6)
    {
        currentSnap = static_cast<SnapValue>(id - 1);
        noteGrid->setSnapBeats(getSnapInBeats());
    }
}

void PianoRollEditor::syncScrollPositions()
{
    // Sync keyboard scroll with grid viewport
    if (gridViewport && keyboard)
    {
        int yOffset = gridViewport->getViewPositionY();
        keyboard->setScrollOffset(yOffset);
    }
}
