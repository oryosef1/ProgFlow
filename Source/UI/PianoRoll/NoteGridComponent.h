#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/MidiClip.h"
#include <functional>
#include <set>

// Tool modes for piano roll editing
enum class PianoRollTool { Draw, Select, Erase, Slice };

/**
 * NoteGridComponent - The main note editing grid for piano roll
 *
 * Displays a grid where notes can be drawn, selected, moved, and resized.
 * Supports multiple tools: draw, select, erase, slice.
 */
class NoteGridComponent : public juce::Component
{
public:
    NoteGridComponent();

    void setClip(MidiClip* clip);
    MidiClip* getClip() const { return clip; }

    void setDimensions(int beatWidth, int keyHeight);
    int getBeatWidth() const { return beatWidth; }
    int getKeyHeight() const { return keyHeight; }

    void setSnapBeats(double snap);
    double getSnapBeats() const { return snapBeats; }

    void setTool(PianoRollTool tool);
    PianoRollTool getTool() const { return tool; }

    void setSelectedNotes(const std::set<juce::Uuid>& selected);
    const std::set<juce::Uuid>& getSelectedNotes() const { return selectedNotes; }

    void setTrackColour(juce::Colour colour);

    // Ghost notes (notes from adjacent clips shown in lighter color)
    void setShowGhostNotes(bool show);
    bool getShowGhostNotes() const { return showGhostNotes; }
    void setGhostNotes(const std::vector<Note>& notes);

    // Refresh display
    void updateSize();

    void paint(juce::Graphics& g) override;

    // Mouse handling
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void(const Note&)> onNoteAdded;
    std::function<void(const juce::Uuid&, const Note&)> onNoteUpdated;
    std::function<void(const juce::Uuid&)> onNoteDeleted;
    std::function<void(const std::set<juce::Uuid>&)> onSelectionChanged;
    std::function<void(int midiNote, float velocity)> onNotePreview;
    std::function<void(int midiNote)> onNotePreviewEnd;

private:
    MidiClip* clip = nullptr;
    int beatWidth = 40;
    int keyHeight = 16;
    double snapBeats = 0.5;  // 1/8 note default
    PianoRollTool tool = PianoRollTool::Draw;
    std::set<juce::Uuid> selectedNotes;
    juce::Colour trackColour{0xff3b82f6};

    // Ghost notes
    bool showGhostNotes = false;
    std::vector<Note> ghostNotes;

    static constexpr int TOTAL_KEYS = 128;

    // Drag state
    enum class DragMode { None, Draw, Move, ResizeLeft, ResizeRight, Select };
    DragMode dragMode = DragMode::None;
    juce::Uuid dragNoteId;
    double dragStartBeat = 0.0;
    double dragStartDuration = 0.0;
    int dragStartMidiNote = 0;
    juce::Point<int> dragStartPoint;
    juce::Rectangle<int> selectionRect;

    // Preview note while drawing
    bool showPreview = false;
    int previewNote = 60;
    double previewStartBeat = 0.0;
    double previewDuration = 0.5;

    // Coordinate conversion
    int midiNoteToY(int midiNote) const;
    int yToMidiNote(int y) const;
    double xToBeat(int x) const;
    int beatToX(double beat) const;
    double snapBeat(double beat) const;

    // Drawing helpers
    void drawGrid(juce::Graphics& g);
    void drawGhostNotes(juce::Graphics& g);
    void drawNotes(juce::Graphics& g);
    void drawSelectionRect(juce::Graphics& g);
    void drawPreviewNote(juce::Graphics& g);

    // Hit testing
    Note* noteAtPoint(juce::Point<int> point, bool& onLeftEdge, bool& onRightEdge);
    void updateSelectionFromRect();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoteGridComponent)
};
