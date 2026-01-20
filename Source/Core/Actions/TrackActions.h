#pragma once

#include "../UndoableAction.h"
#include "../../Audio/AudioEngine.h"
#include "../../Audio/Track.h"
#include <juce_core/juce_core.h>
#include <functional>

class AudioEngine;
class Track;

//==============================================================================
/**
 * Action for adding a new track
 */
class AddTrackAction : public UndoableAction
{
public:
    using TrackCreatedCallback = std::function<void(Track*)>;

    AddTrackAction(AudioEngine* engine,
                   const juce::String& name = "Track",
                   SynthType synthType = SynthType::Analog,
                   TrackCreatedCallback callback = nullptr)
        : UndoableAction("Add Track")
        , audioEngine(engine)
        , trackName(name)
        , synthType(synthType)
        , onTrackCreated(callback)
    {
    }

    bool execute() override;
    bool undo() override;

    const juce::Uuid& getTrackId() const { return trackId; }

private:
    AudioEngine* audioEngine;
    juce::String trackName;
    SynthType synthType;
    juce::Uuid trackId;
    TrackCreatedCallback onTrackCreated;
};

//==============================================================================
/**
 * Action for deleting a track
 */
class DeleteTrackAction : public UndoableAction
{
public:
    DeleteTrackAction(AudioEngine* engine, const juce::Uuid& trackId)
        : UndoableAction("Delete Track")
        , audioEngine(engine)
        , trackId(trackId)
    {
    }

    bool execute() override;
    bool undo() override;

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;

    // Store track state for undo
    juce::String trackName;
    juce::Colour trackColour;
    SynthType synthType;
    float volume = 1.0f;
    float pan = 0.0f;
    bool muted = false;
    bool soloed = false;
    int trackIndex = 0;

    // Store serialized clips
    juce::var clipsData;
};

//==============================================================================
/**
 * Action for renaming a track
 */
class RenameTrackAction : public UndoableAction
{
public:
    RenameTrackAction(AudioEngine* engine, const juce::Uuid& trackId,
                      const juce::String& newName)
        : UndoableAction("Rename Track")
        , audioEngine(engine)
        , trackId(trackId)
        , newName(newName)
    {
    }

    bool execute() override;
    bool undo() override;

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    juce::String oldName;
    juce::String newName;
};

//==============================================================================
/**
 * Action for changing track color
 */
class ChangeTrackColorAction : public UndoableAction
{
public:
    ChangeTrackColorAction(AudioEngine* engine, const juce::Uuid& trackId,
                           juce::Colour newColour)
        : UndoableAction("Change Track Color")
        , audioEngine(engine)
        , trackId(trackId)
        , newColour(newColour)
    {
    }

    bool execute() override;
    bool undo() override;

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    juce::Colour oldColour;
    juce::Colour newColour;
};

//==============================================================================
/**
 * Action for changing track volume
 */
class ChangeTrackVolumeAction : public UndoableAction
{
public:
    ChangeTrackVolumeAction(AudioEngine* engine, const juce::Uuid& trackId,
                            float newVolume)
        : UndoableAction("Change Volume")
        , audioEngine(engine)
        , trackId(trackId)
        , newVolume(newVolume)
    {
    }

    bool execute() override;
    bool undo() override;

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* volAction = dynamic_cast<const ChangeTrackVolumeAction*>(other))
        {
            return volAction->audioEngine == audioEngine &&
                   volAction->trackId == trackId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* volAction = dynamic_cast<const ChangeTrackVolumeAction*>(other))
        {
            newVolume = volAction->newVolume;
        }
    }

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    float oldVolume = 1.0f;
    float newVolume = 1.0f;
};

//==============================================================================
/**
 * Action for changing track pan
 */
class ChangeTrackPanAction : public UndoableAction
{
public:
    ChangeTrackPanAction(AudioEngine* engine, const juce::Uuid& trackId,
                         float newPan)
        : UndoableAction("Change Pan")
        , audioEngine(engine)
        , trackId(trackId)
        , newPan(newPan)
    {
    }

    bool execute() override;
    bool undo() override;

    bool canMergeWith(const UndoableAction* other) const override
    {
        if (auto* panAction = dynamic_cast<const ChangeTrackPanAction*>(other))
        {
            return panAction->audioEngine == audioEngine &&
                   panAction->trackId == trackId;
        }
        return false;
    }

    void mergeWith(const UndoableAction* other) override
    {
        if (auto* panAction = dynamic_cast<const ChangeTrackPanAction*>(other))
        {
            newPan = panAction->newPan;
        }
    }

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    float oldPan = 0.0f;
    float newPan = 0.0f;
};

//==============================================================================
/**
 * Action for toggling track mute
 */
class ToggleTrackMuteAction : public UndoableAction
{
public:
    ToggleTrackMuteAction(AudioEngine* engine, const juce::Uuid& trackId)
        : UndoableAction("Toggle Mute")
        , audioEngine(engine)
        , trackId(trackId)
    {
    }

    bool execute() override;
    bool undo() override;

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    bool wasMuted = false;
};

//==============================================================================
/**
 * Action for toggling track solo
 */
class ToggleTrackSoloAction : public UndoableAction
{
public:
    ToggleTrackSoloAction(AudioEngine* engine, const juce::Uuid& trackId)
        : UndoableAction("Toggle Solo")
        , audioEngine(engine)
        , trackId(trackId)
    {
    }

    bool execute() override;
    bool undo() override;

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    bool wasSoloed = false;
};

//==============================================================================
/**
 * Action for changing track synth type
 */
class ChangeSynthTypeAction : public UndoableAction
{
public:
    ChangeSynthTypeAction(AudioEngine* engine, const juce::Uuid& trackId,
                          SynthType newType)
        : UndoableAction("Change Instrument")
        , audioEngine(engine)
        , trackId(trackId)
        , newType(newType)
    {
    }

    bool execute() override;
    bool undo() override;

private:
    AudioEngine* audioEngine;
    juce::Uuid trackId;
    SynthType oldType = SynthType::Analog;
    SynthType newType;
};
