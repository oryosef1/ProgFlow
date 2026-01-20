#include "TrackActions.h"

// Helper to find track by ID
static Track* findTrackById(AudioEngine* engine, const juce::Uuid& trackId)
{
    if (!engine) return nullptr;

    for (int i = 0; i < engine->getNumTracks(); ++i)
    {
        if (auto* track = engine->getTrack(i))
        {
            if (track->getId() == trackId)
                return track;
        }
    }
    return nullptr;
}

// Helper to find track index by ID
static int findTrackIndex(AudioEngine* engine, const juce::Uuid& trackId)
{
    if (!engine) return -1;

    for (int i = 0; i < engine->getNumTracks(); ++i)
    {
        if (auto* track = engine->getTrack(i))
        {
            if (track->getId() == trackId)
                return i;
        }
    }
    return -1;
}

//==============================================================================
// AddTrackAction
//==============================================================================

bool AddTrackAction::execute()
{
    if (!audioEngine) return false;

    auto track = std::make_unique<Track>(trackName);
    track->setSynthType(synthType);

    trackId = track->getId();

    if (onTrackCreated)
        onTrackCreated(track.get());

    audioEngine->addTrack(std::move(track));
    return true;
}

bool AddTrackAction::undo()
{
    if (!audioEngine) return false;

    int index = findTrackIndex(audioEngine, trackId);
    if (index < 0) return false;

    audioEngine->removeTrack(index);
    return true;
}

//==============================================================================
// DeleteTrackAction
//==============================================================================

bool DeleteTrackAction::execute()
{
    if (!audioEngine) return false;

    // Find and store track data before deletion
    int index = findTrackIndex(audioEngine, trackId);
    if (index < 0) return false;

    auto* track = audioEngine->getTrack(index);
    if (!track) return false;

    // Store state for undo
    trackName = track->getName();
    trackColour = track->getColour();
    synthType = track->getSynthType();
    volume = track->getVolume();
    pan = track->getPan();
    muted = track->isMuted();
    soloed = track->isSoloed();
    trackIndex = index;

    // Serialize clips
    juce::Array<juce::var> clipsArray;
    for (const auto& clip : track->getClips())
    {
        clipsArray.add(clip->toVar());
    }
    clipsData = clipsArray;

    // Remove the track
    audioEngine->removeTrack(index);
    return true;
}

bool DeleteTrackAction::undo()
{
    if (!audioEngine) return false;

    // Recreate track with stored state
    auto track = std::make_unique<Track>(trackName);
    track->setColour(trackColour);
    track->setSynthType(synthType);
    track->setVolume(volume);
    track->setPan(pan);
    track->setMuted(muted);
    track->setSoloed(soloed);

    // Restore clips
    if (clipsData.isArray())
    {
        for (const auto& clipVar : *clipsData.getArray())
        {
            auto clip = MidiClip::fromVar(clipVar);
            if (clip)
                track->addClip(std::move(clip));
        }
    }

    // Note: This adds at the end, not at the original index
    // For proper restore, would need insertTrack at index
    audioEngine->addTrack(std::move(track));
    return true;
}

//==============================================================================
// RenameTrackAction
//==============================================================================

bool RenameTrackAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    oldName = track->getName();
    track->setName(newName);
    return true;
}

bool RenameTrackAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setName(oldName);
    return true;
}

//==============================================================================
// ChangeTrackColorAction
//==============================================================================

bool ChangeTrackColorAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    oldColour = track->getColour();
    track->setColour(newColour);
    return true;
}

bool ChangeTrackColorAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setColour(oldColour);
    return true;
}

//==============================================================================
// ChangeTrackVolumeAction
//==============================================================================

bool ChangeTrackVolumeAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    oldVolume = track->getVolume();
    track->setVolume(newVolume);
    return true;
}

bool ChangeTrackVolumeAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setVolume(oldVolume);
    return true;
}

//==============================================================================
// ChangeTrackPanAction
//==============================================================================

bool ChangeTrackPanAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    oldPan = track->getPan();
    track->setPan(newPan);
    return true;
}

bool ChangeTrackPanAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setPan(oldPan);
    return true;
}

//==============================================================================
// ToggleTrackMuteAction
//==============================================================================

bool ToggleTrackMuteAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    wasMuted = track->isMuted();
    track->setMuted(!wasMuted);
    return true;
}

bool ToggleTrackMuteAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setMuted(wasMuted);
    return true;
}

//==============================================================================
// ToggleTrackSoloAction
//==============================================================================

bool ToggleTrackSoloAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    wasSoloed = track->isSoloed();
    track->setSoloed(!wasSoloed);
    return true;
}

bool ToggleTrackSoloAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setSoloed(wasSoloed);
    return true;
}

//==============================================================================
// ChangeSynthTypeAction
//==============================================================================

bool ChangeSynthTypeAction::execute()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    oldType = track->getSynthType();
    track->setSynthType(newType);
    return true;
}

bool ChangeSynthTypeAction::undo()
{
    auto* track = findTrackById(audioEngine, trackId);
    if (!track) return false;

    track->setSynthType(oldType);
    return true;
}
