#include "MidiClip.h"

MidiClip::MidiClip(const juce::String& clipName)
    : name(clipName)
{
}

//==============================================================================
// Note Management

void MidiClip::addNote(const Note& note)
{
    notes.push_back(note);
    sortNotes();
}

void MidiClip::addNote(int midiNote, double startBeat, double durationBeats, float velocity)
{
    Note note;
    note.id = juce::Uuid();
    note.midiNote = juce::jlimit(0, 127, midiNote);
    note.startBeat = std::max(0.0, startBeat);
    note.durationBeats = std::max(0.0625, durationBeats);  // Minimum 1/16 beat
    note.velocity = juce::jlimit(0.0f, 1.0f, velocity);

    notes.push_back(note);
    sortNotes();
}

void MidiClip::removeNote(const juce::Uuid& noteId)
{
    notes.erase(
        std::remove_if(notes.begin(), notes.end(),
            [&noteId](const Note& n) { return n.id == noteId; }),
        notes.end()
    );
}

void MidiClip::updateNote(const juce::Uuid& noteId, const Note& newNote)
{
    for (auto& note : notes)
    {
        if (note.id == noteId)
        {
            // Preserve the ID
            note.midiNote = juce::jlimit(0, 127, newNote.midiNote);
            note.startBeat = std::max(0.0, newNote.startBeat);
            note.durationBeats = std::max(0.0625, newNote.durationBeats);
            note.velocity = juce::jlimit(0.0f, 1.0f, newNote.velocity);
            sortNotes();
            return;
        }
    }
}

Note* MidiClip::findNote(const juce::Uuid& noteId)
{
    for (auto& note : notes)
    {
        if (note.id == noteId)
            return &note;
    }
    return nullptr;
}

const Note* MidiClip::findNote(const juce::Uuid& noteId) const
{
    for (const auto& note : notes)
    {
        if (note.id == noteId)
            return &note;
    }
    return nullptr;
}

void MidiClip::clear()
{
    notes.clear();
}

//==============================================================================
// Playback Query

void MidiClip::getNotesInRange(double startBeat, double endBeat,
                                std::vector<const Note*>& result) const
{
    result.clear();

    // Binary search for first note that could be in range
    // (notes are sorted by startBeat)
    auto it = std::lower_bound(notes.begin(), notes.end(), startBeat,
        [](const Note& note, double beat) { return note.startBeat < beat; });

    // Step back to include notes that started before but might still be relevant
    if (it != notes.begin())
        --it;

    // Collect notes that start within the range
    for (; it != notes.end(); ++it)
    {
        if (it->startBeat >= endBeat)
            break;  // Past the range, done

        if (it->startBeat >= startBeat && it->startBeat < endBeat)
        {
            result.push_back(&(*it));
        }
    }
}

void MidiClip::getActiveNotesAt(double beat, std::vector<const Note*>& result) const
{
    result.clear();

    for (const auto& note : notes)
    {
        if (note.startBeat <= beat && note.getEndBeat() > beat)
        {
            result.push_back(&note);
        }
    }
}

//==============================================================================
// Editing Operations

void MidiClip::quantizeNotes(double snapBeats, double strength)
{
    if (snapBeats <= 0.0) return;
    strength = juce::jlimit(0.0, 1.0, strength);

    for (auto& note : notes)
    {
        // Calculate quantized position
        double quantizedStart = std::round(note.startBeat / snapBeats) * snapBeats;
        double quantizedDuration = std::max(snapBeats,
            std::round(note.durationBeats / snapBeats) * snapBeats);

        // Interpolate between original and quantized based on strength
        note.startBeat = note.startBeat + (quantizedStart - note.startBeat) * strength;
        note.durationBeats = note.durationBeats + (quantizedDuration - note.durationBeats) * strength;
    }

    sortNotes();
}

void MidiClip::quantizeNotes(const std::vector<juce::Uuid>& noteIds, double snapBeats, double strength)
{
    if (snapBeats <= 0.0) return;
    strength = juce::jlimit(0.0, 1.0, strength);

    for (auto& note : notes)
    {
        // Check if this note is in the selection
        bool isSelected = false;
        for (const auto& id : noteIds)
        {
            if (note.id == id)
            {
                isSelected = true;
                break;
            }
        }

        if (isSelected)
        {
            // Calculate quantized position
            double quantizedStart = std::round(note.startBeat / snapBeats) * snapBeats;
            double quantizedDuration = std::max(snapBeats,
                std::round(note.durationBeats / snapBeats) * snapBeats);

            // Interpolate between original and quantized based on strength
            note.startBeat = note.startBeat + (quantizedStart - note.startBeat) * strength;
            note.durationBeats = note.durationBeats + (quantizedDuration - note.durationBeats) * strength;
        }
    }

    sortNotes();
}

void MidiClip::transposeNotes(int semitones)
{
    for (auto& note : notes)
    {
        note.midiNote = juce::jlimit(0, 127, note.midiNote + semitones);
    }
}

void MidiClip::transposeNotes(const std::vector<juce::Uuid>& noteIds, int semitones)
{
    for (auto& note : notes)
    {
        for (const auto& id : noteIds)
        {
            if (note.id == id)
            {
                note.midiNote = juce::jlimit(0, 127, note.midiNote + semitones);
                break;
            }
        }
    }
}

std::unique_ptr<MidiClip> MidiClip::splitAt(double splitBeat, double beatsPerBar)
{
    // Validate split position
    double clipDurationBeats = getDurationBeats();
    if (splitBeat <= 0.0 || splitBeat >= clipDurationBeats)
        return nullptr;

    // Create new clip for the second half
    auto newClip = std::make_unique<MidiClip>(name + " (split)");
    newClip->setColour(colour);

    // Calculate positions using the provided beats per bar
    double splitBar = splitBeat / beatsPerBar;  // Convert beats to bars
    double newClipStartBar = startBar + splitBar;
    double newClipDurationBars = durationBars - splitBar;

    newClip->setStartBar(newClipStartBar);
    newClip->setDurationBars(newClipDurationBars);

    // Separate notes into two groups
    std::vector<Note> notesForOriginal;
    std::vector<Note> notesForNew;

    for (const auto& note : notes)
    {
        if (note.startBeat >= splitBeat)
        {
            // Note starts in second half - move to new clip
            Note newNote = note;
            newNote.id = juce::Uuid();  // New ID
            newNote.startBeat = note.startBeat - splitBeat;  // Adjust position
            notesForNew.push_back(newNote);
        }
        else if (note.getEndBeat() > splitBeat)
        {
            // Note spans the split point - truncate in original, continue in new
            Note truncatedNote = note;
            truncatedNote.durationBeats = splitBeat - note.startBeat;
            notesForOriginal.push_back(truncatedNote);

            // Create continuation in new clip
            Note continuationNote;
            continuationNote.id = juce::Uuid();
            continuationNote.midiNote = note.midiNote;
            continuationNote.startBeat = 0.0;
            continuationNote.durationBeats = note.getEndBeat() - splitBeat;
            continuationNote.velocity = note.velocity;
            notesForNew.push_back(continuationNote);
        }
        else
        {
            // Note is entirely in first half - keep in original
            notesForOriginal.push_back(note);
        }
    }

    // Update original clip
    notes = notesForOriginal;
    durationBars = splitBar;
    sortNotes();

    // Add notes to new clip
    for (const auto& note : notesForNew)
    {
        newClip->addNote(note);
    }

    return newClip;
}

//==============================================================================
// Serialization

juce::var MidiClip::toVar() const
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty("id", id.toString());
    obj->setProperty("name", name);
    obj->setProperty("colour", static_cast<juce::int64>(colour.getARGB()));
    obj->setProperty("startBar", startBar);
    obj->setProperty("durationBars", durationBars);

    juce::Array<juce::var> notesArray;
    for (const auto& note : notes)
    {
        auto* noteObj = new juce::DynamicObject();
        noteObj->setProperty("id", note.id.toString());
        noteObj->setProperty("midiNote", note.midiNote);
        noteObj->setProperty("startBeat", note.startBeat);
        noteObj->setProperty("durationBeats", note.durationBeats);
        noteObj->setProperty("velocity", note.velocity);
        notesArray.add(juce::var(noteObj));
    }
    obj->setProperty("notes", notesArray);

    return juce::var(obj);
}

std::unique_ptr<MidiClip> MidiClip::fromVar(const juce::var& data)
{
    if (!data.isObject())
        return nullptr;

    auto clip = std::make_unique<MidiClip>();

    if (data.hasProperty("id"))
        clip->id = juce::Uuid(data["id"].toString());
    if (data.hasProperty("name"))
        clip->name = data["name"].toString();
    if (data.hasProperty("colour"))
        clip->colour = juce::Colour(static_cast<juce::uint32>(static_cast<juce::int64>(data["colour"])));
    if (data.hasProperty("startBar"))
        clip->startBar = static_cast<double>(data["startBar"]);
    if (data.hasProperty("durationBars"))
        clip->durationBars = static_cast<double>(data["durationBars"]);

    if (data.hasProperty("notes") && data["notes"].isArray())
    {
        const auto* notesArray = data["notes"].getArray();
        for (const auto& noteVar : *notesArray)
        {
            if (noteVar.isObject())
            {
                Note note;
                if (noteVar.hasProperty("id"))
                    note.id = juce::Uuid(noteVar["id"].toString());
                if (noteVar.hasProperty("midiNote"))
                    note.midiNote = static_cast<int>(noteVar["midiNote"]);
                if (noteVar.hasProperty("startBeat"))
                    note.startBeat = static_cast<double>(noteVar["startBeat"]);
                if (noteVar.hasProperty("durationBeats"))
                    note.durationBeats = static_cast<double>(noteVar["durationBeats"]);
                if (noteVar.hasProperty("velocity"))
                    note.velocity = static_cast<float>(noteVar["velocity"]);

                clip->notes.push_back(note);
            }
        }
    }

    return clip;
}

//==============================================================================
// Private

void MidiClip::sortNotes()
{
    std::stable_sort(notes.begin(), notes.end());
}
