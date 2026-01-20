#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/AudioEngine.h"
#include "../../Audio/MidiClip.h"
#include "PianoKeyboard.h"
#include "NoteGridComponent.h"
#include "VelocityLane.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>

// Snap value options
enum class SnapValue { Off, Bar, Beat, Eighth, Sixteenth, ThirtySecond };

/**
 * PianoRollEditor - Main piano roll MIDI editor
 *
 * Contains:
 * - Toolbar with tools, snap, zoom
 * - PianoKeyboard on left
 * - NoteGridComponent in scrollable viewport
 * - VelocityLane below grid
 */
class PianoRollEditor : public juce::Component,
                        public juce::KeyListener
{
public:
    PianoRollEditor(AudioEngine& engine);
    ~PianoRollEditor() override;

    //==========================================================================
    // Clip editing
    void setClip(MidiClip* clip);
    MidiClip* getClip() const { return currentClip; }

    void setTrack(Track* track);
    Track* getTrack() const { return currentTrack; }

    void setTrackColour(juce::Colour colour);

    //==========================================================================
    // Ghost notes (show notes from other clips on same track)
    void setShowGhostNotes(bool show);
    bool getShowGhostNotes() const { return showGhostNotes; }
    void toggleGhostNotes() { setShowGhostNotes(!showGhostNotes); }

    //==========================================================================
    // Tools
    void setTool(PianoRollTool tool);
    PianoRollTool getTool() const { return currentTool; }

    //==========================================================================
    // Snap
    void setSnap(SnapValue snap);
    SnapValue getSnap() const { return currentSnap; }
    double getSnapInBeats() const;

    //==========================================================================
    // Zoom
    void setZoomX(float zoom);  // Horizontal (time)
    void setZoomY(float zoom);  // Vertical (pitch)
    float getZoomX() const { return zoomX; }
    float getZoomY() const { return zoomY; }

    //==========================================================================
    // Selection operations
    const std::set<juce::Uuid>& getSelectedNotes() const { return selectedNotes; }
    void selectAll();
    void deleteSelected();
    void quantizeSelected();
    void transposeSelected(int semitones);

    //==========================================================================
    // Copy/Paste/Duplicate operations
    void copySelected();
    void cutSelected();
    void paste();
    void duplicateSelected();

    //==========================================================================
    // Note preview callback (for synth feedback)
    std::function<void(int midiNote, float velocity)> onNotePreview;
    std::function<void(int midiNote)> onNotePreviewEnd;

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // KeyListener
    bool keyPressed(const juce::KeyPress& key, juce::Component* origin) override;

private:
    AudioEngine& audioEngine;
    MidiClip* currentClip = nullptr;
    Track* currentTrack = nullptr;
    juce::Colour trackColour{0xff3b82f6};
    bool showGhostNotes = false;

    // Child components
    std::unique_ptr<juce::Component> toolbar;
    std::unique_ptr<PianoKeyboard> keyboard;
    std::unique_ptr<juce::Viewport> gridViewport;
    std::unique_ptr<NoteGridComponent> noteGrid;
    std::unique_ptr<VelocityLane> velocityLane;

    // Tool buttons
    std::unique_ptr<juce::TextButton> drawButton;
    std::unique_ptr<juce::TextButton> selectButton;
    std::unique_ptr<juce::TextButton> eraseButton;
    std::unique_ptr<juce::ComboBox> snapCombo;

    // State
    PianoRollTool currentTool = PianoRollTool::Draw;
    SnapValue currentSnap = SnapValue::Eighth;
    float zoomX = 1.0f;
    float zoomY = 1.0f;
    std::set<juce::Uuid> selectedNotes;

    // Clipboard for copy/paste (stores note data with positions relative to earliest note)
    struct ClipboardNote
    {
        int midiNote;
        double startBeat;  // Relative to earliest note in selection
        double durationBeats;
        float velocity;
    };
    std::vector<ClipboardNote> clipboard;

    // Layout constants
    static constexpr int TOOLBAR_HEIGHT = 36;
    static constexpr int KEYBOARD_WIDTH = 80;
    static constexpr int VELOCITY_LANE_HEIGHT = 60;
    static constexpr int BASE_KEY_HEIGHT = 16;
    static constexpr int BASE_BEAT_WIDTH = 40;

    int getKeyHeight() const;
    int getBeatWidth() const;

    void createToolbar();
    void updateToolButtons();
    void handleToolChange(PianoRollTool tool);
    void handleSnapChange();
    void syncScrollPositions();
    void updateGhostNotes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollEditor)
};
