#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Audio/AudioEngine.h"
#include "../Audio/Track.h"
#include "../Audio/MidiClip.h"

/**
 * ProjectSerializer - Handles JSON serialization of project data
 *
 * Serializes/deserializes:
 * - Project metadata (name, bpm, time signature)
 * - Tracks with clips and notes
 * - Synth parameters
 * - Plugin state (base64-encoded)
 * - Effect chain configuration
 *
 * File format is compatible with Electron ProgFlow (.progflow files)
 * Version 1 = Electron format
 * Version 2 = JUCE extensions (plugin state)
 */
class ProjectSerializer
{
public:
    // Current file format version
    static constexpr int CURRENT_VERSION = 2;

    //==========================================================================
    // Project data structure (for serialization)
    struct ProjectData
    {
        int version = CURRENT_VERSION;
        juce::String name = "Untitled";
        double bpm = 120.0;
        int timeSignatureNum = 4;
        int timeSignatureDen = 4;
        juce::Array<juce::var> tracks;
        juce::Array<juce::var> markers;
    };

    //==========================================================================
    // Serialize project to JSON string
    static juce::String serialize(const ProjectData& project);

    // Serialize from AudioEngine state
    static juce::String serializeFromEngine(AudioEngine& engine,
                                            const juce::String& projectName,
                                            double bpm,
                                            int timeSigNum = 4,
                                            int timeSigDen = 4);

    //==========================================================================
    // Deserialize project from JSON string
    static bool deserialize(const juce::String& jsonString, ProjectData& outProject);

    // Deserialize and load into AudioEngine
    static bool deserializeToEngine(const juce::String& jsonString,
                                    AudioEngine& engine,
                                    juce::String& outProjectName,
                                    double& outBpm);

    //==========================================================================
    // Individual serialization methods
    static juce::var serializeTrack(const Track& track);
    static juce::var serializeClip(const MidiClip& clip);
    static juce::var serializeNote(const Note& note);
    static juce::var serializePlugin(juce::AudioPluginInstance* plugin,
                                     const juce::PluginDescription* desc);

    //==========================================================================
    // Individual deserialization methods
    static std::unique_ptr<Track> deserializeTrack(const juce::var& data);
    static std::unique_ptr<MidiClip> deserializeClip(const juce::var& data);
    static Note deserializeNote(const juce::var& data);

    //==========================================================================
    // Base64 encoding/decoding for plugin state
    static juce::String encodePluginState(juce::AudioPluginInstance* plugin);
    static juce::MemoryBlock decodePluginState(const juce::String& base64State);

    //==========================================================================
    // Color conversion helpers (hex string <-> Colour)
    static juce::String colourToHex(juce::Colour colour);
    static juce::Colour hexToColour(const juce::String& hex);

private:
    ProjectSerializer() = delete; // Static-only class
};
