#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>
#include <algorithm>

/**
 * A single MIDI note event.
 * Uses MIDI note numbers (60 = C4) for efficient computation.
 */
struct Note
{
    juce::Uuid id;              // Unique identifier for UI selection
    int midiNote = 60;          // 0-127, middle C = 60
    double startBeat = 0.0;     // Position within clip (0-based)
    double durationBeats = 1.0; // Length in beats
    float velocity = 0.8f;      // 0.0 - 1.0

    // Computed end position
    double getEndBeat() const { return startBeat + durationBeats; }

    // Convert MIDI note to string for display (e.g., "C4", "F#5")
    static juce::String midiNoteToString(int midiNote)
    {
        static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        int octave = (midiNote / 12) - 1;
        int noteIndex = midiNote % 12;
        return juce::String(noteNames[noteIndex]) + juce::String(octave);
    }

    static int stringToMidiNote(const juce::String& str)
    {
        static const juce::StringArray noteNames = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

        if (str.isEmpty()) return 60;

        // Parse note name and octave
        juce::String notePart;
        int octave = 4;

        // Handle sharps/flats
        if (str.length() >= 2 && (str[1] == '#' || str[1] == 'b'))
        {
            notePart = str.substring(0, 2);
            if (str.length() > 2)
                octave = str.substring(2).getIntValue();
        }
        else
        {
            notePart = str.substring(0, 1);
            if (str.length() > 1)
                octave = str.substring(1).getIntValue();
        }

        // Handle flats by converting to sharps
        if (notePart.endsWithChar('b'))
        {
            int baseIndex = noteNames.indexOf(notePart.substring(0, 1));
            if (baseIndex > 0)
            {
                notePart = noteNames[baseIndex - 1] + "#";
            }
        }

        int noteIndex = noteNames.indexOf(notePart.toUpperCase());
        if (noteIndex < 0) noteIndex = 0;

        return (octave + 1) * 12 + noteIndex;
    }

    // Comparison for sorting
    bool operator<(const Note& other) const
    {
        return startBeat < other.startBeat;
    }
};

/**
 * A container of MIDI notes that lives on a track timeline.
 * Position is in bars (relative to project start).
 */
class MidiClip
{
public:
    MidiClip(const juce::String& name = "Clip");
    ~MidiClip() = default;

    // Allow move operations
    MidiClip(MidiClip&&) = default;
    MidiClip& operator=(MidiClip&&) = default;

    //==========================================================================
    // Identity
    const juce::Uuid& getId() const { return id; }
    juce::String getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }

    juce::Colour getColour() const { return colour; }
    void setColour(juce::Colour newColour) { colour = newColour; }

    //==========================================================================
    // Position & Duration (in bars, project-relative)
    double getStartBar() const { return startBar; }
    void setStartBar(double bar) { startBar = bar; }

    double getDurationBars() const { return durationBars; }
    void setDurationBars(double bars) { durationBars = std::max(0.25, bars); }

    double getEndBar() const { return startBar + durationBars; }

    // Convert bars to beats (assuming 4/4 time)
    double getStartBeat() const { return startBar * 4.0; }
    double getDurationBeats() const { return durationBars * 4.0; }
    double getEndBeat() const { return getEndBar() * 4.0; }

    //==========================================================================
    // Note Management
    void addNote(const Note& note);
    void addNote(int midiNote, double startBeat, double durationBeats, float velocity = 0.8f);
    void removeNote(const juce::Uuid& noteId);
    void updateNote(const juce::Uuid& noteId, const Note& newNote);
    Note* findNote(const juce::Uuid& noteId);
    const Note* findNote(const juce::Uuid& noteId) const;

    const std::vector<Note>& getNotes() const { return notes; }
    std::vector<Note>& getNotes() { return notes; }

    // Clear all notes
    void clear();

    // Get note count
    size_t getNumNotes() const { return notes.size(); }

    //==========================================================================
    // Playback Query
    // Get notes that should start within a beat range (relative to clip start)
    // Used by playback engine to schedule notes
    void getNotesInRange(double startBeat, double endBeat,
                         std::vector<const Note*>& result) const;

    // Get notes that are active (sounding) at a specific beat
    void getActiveNotesAt(double beat, std::vector<const Note*>& result) const;

    //==========================================================================
    // Editing Operations

    /**
     * Quantize notes to grid
     * @param snapBeats Grid size in beats (e.g., 0.25 for 1/16th)
     * @param strength Quantize strength 0.0 (no change) to 1.0 (full quantize)
     */
    void quantizeNotes(double snapBeats, double strength = 1.0);

    /**
     * Quantize selected notes
     */
    void quantizeNotes(const std::vector<juce::Uuid>& noteIds, double snapBeats, double strength = 1.0);

    void transposeNotes(int semitones);
    void transposeNotes(const std::vector<juce::Uuid>& noteIds, int semitones);

    /**
     * Split clip at a beat position (relative to clip start)
     * Returns a new clip containing notes from splitBeat onward.
     * Original clip is shortened and notes after splitBeat are removed.
     * Notes that span the split point are truncated in the first clip
     * and start at beat 0 in the new clip.
     * @param splitBeat Position in beats (relative to clip start) to split at
     * @param beatsPerBar Number of beats per bar (default 4.0 for 4/4 time)
     */
    std::unique_ptr<MidiClip> splitAt(double splitBeat, double beatsPerBar = 4.0);

    //==========================================================================
    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<MidiClip> fromVar(const juce::var& data);

private:
    juce::Uuid id;
    juce::String name;
    juce::Colour colour{0xff3b82f6};  // Default accent blue

    double startBar = 0.0;
    double durationBars = 4.0;        // Default 4 bars

    std::vector<Note> notes;

    // Keep notes sorted by startBeat for efficient range queries
    void sortNotes();

    // Disallow copy (use unique_ptr for ownership)
    MidiClip(const MidiClip&) = delete;
    MidiClip& operator=(const MidiClip&) = delete;
};
