#include "ProjectSerializer.h"
#include "../Audio/AutomationLane.h"

//==============================================================================
// Serialize project to JSON string

juce::String ProjectSerializer::serialize(const ProjectData& project)
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty("version", project.version);
    obj->setProperty("name", project.name);
    obj->setProperty("bpm", project.bpm);

    // Time signature as array [num, den]
    juce::Array<juce::var> timeSig;
    timeSig.add(project.timeSignatureNum);
    timeSig.add(project.timeSignatureDen);
    obj->setProperty("timeSignature", timeSig);

    obj->setProperty("tracks", project.tracks);
    obj->setProperty("markers", project.markers);

    juce::var projectVar(obj);
    return juce::JSON::toString(projectVar, true); // Pretty print
}

juce::String ProjectSerializer::serializeFromEngine(AudioEngine& engine,
                                                     const juce::String& projectName,
                                                     double bpm,
                                                     int timeSigNum,
                                                     int timeSigDen)
{
    ProjectData project;
    project.version = CURRENT_VERSION;
    project.name = projectName;
    project.bpm = bpm;
    project.timeSignatureNum = timeSigNum;
    project.timeSignatureDen = timeSigDen;

    // Serialize all tracks
    for (int i = 0; i < engine.getNumTracks(); ++i)
    {
        if (auto* track = engine.getTrack(i))
        {
            project.tracks.add(serializeTrack(*track));
        }
    }

    return serialize(project);
}

//==============================================================================
// Deserialize project from JSON string

bool ProjectSerializer::deserialize(const juce::String& jsonString, ProjectData& outProject)
{
    auto parsed = juce::JSON::parse(jsonString);

    if (!parsed.isObject())
    {
        DBG("ProjectSerializer: Failed to parse JSON");
        return false;
    }

    // Version check
    outProject.version = parsed.getProperty("version", 1);
    if (outProject.version > CURRENT_VERSION)
    {
        DBG("ProjectSerializer: File version " << outProject.version << " is newer than supported " << CURRENT_VERSION);
        // Continue anyway, might still work
    }

    outProject.name = parsed.getProperty("name", "Untitled").toString();
    outProject.bpm = static_cast<double>(parsed.getProperty("bpm", 120.0));

    // Time signature
    if (parsed.hasProperty("timeSignature") && parsed["timeSignature"].isArray())
    {
        const auto* timeSig = parsed["timeSignature"].getArray();
        if (timeSig && timeSig->size() >= 2)
        {
            outProject.timeSignatureNum = static_cast<int>((*timeSig)[0]);
            outProject.timeSignatureDen = static_cast<int>((*timeSig)[1]);
        }
    }

    // Tracks
    outProject.tracks.clear();
    if (parsed.hasProperty("tracks") && parsed["tracks"].isArray())
    {
        const auto* tracksArray = parsed["tracks"].getArray();
        if (tracksArray)
        {
            for (const auto& trackVar : *tracksArray)
            {
                outProject.tracks.add(trackVar);
            }
        }
    }

    // Markers
    outProject.markers.clear();
    if (parsed.hasProperty("markers") && parsed["markers"].isArray())
    {
        const auto* markersArray = parsed["markers"].getArray();
        if (markersArray)
        {
            for (const auto& markerVar : *markersArray)
            {
                outProject.markers.add(markerVar);
            }
        }
    }

    return true;
}

bool ProjectSerializer::deserializeToEngine(const juce::String& jsonString,
                                            AudioEngine& engine,
                                            juce::String& outProjectName,
                                            double& outBpm)
{
    ProjectData project;
    if (!deserialize(jsonString, project))
        return false;

    outProjectName = project.name;
    outBpm = project.bpm;

    // Clear existing tracks
    while (engine.getNumTracks() > 0)
    {
        engine.removeTrack(0);
    }

    // Load tracks
    for (const auto& trackVar : project.tracks)
    {
        auto track = deserializeTrack(trackVar);
        if (track)
        {
            engine.addTrack(std::move(track));
        }
    }

    engine.setBpm(project.bpm);

    return true;
}

//==============================================================================
// Track serialization

juce::var ProjectSerializer::serializeTrack(const Track& track)
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty("id", track.getId().toString());
    obj->setProperty("name", track.getName());
    obj->setProperty("color", colourToHex(track.getColour()));
    obj->setProperty("volume", static_cast<double>(track.getVolume()));
    obj->setProperty("pan", static_cast<double>(track.getPan()));
    obj->setProperty("muted", track.isMuted());
    obj->setProperty("soloed", track.isSoloed());

    // Synth type
    obj->setProperty("synthType", SynthFactory::getSynthName(track.getSynthType()));

    // Synth parameters (from the synth's current state)
    if (const auto* synth = track.getSynth())
    {
        auto* paramsObj = new juce::DynamicObject();

        // Get current preset name if set
        paramsObj->setProperty("preset", synth->getCurrentPresetName());

        // Serialize all synth parameters
        for (const auto& param : synth->getParameters())
        {
            paramsObj->setProperty(param.first, param.second);
        }

        obj->setProperty("synthParams", juce::var(paramsObj));
    }

    // Clips
    juce::Array<juce::var> clipsArray;
    for (const auto& clip : track.getClips())
    {
        clipsArray.add(clip->toVar());
    }
    obj->setProperty("clips", clipsArray);

    // Plugin instrument (JUCE-specific, version 2+)
    if (track.hasPluginInstrument())
    {
        obj->setProperty("pluginInstrument",
            serializePlugin(const_cast<Track&>(track).getPluginInstrument(),
                           track.getPluginInstrumentDescription()));
    }
    else
    {
        obj->setProperty("pluginInstrument", juce::var());
    }

    // Plugin effects
    juce::Array<juce::var> pluginEffectsArray;
    for (int i = 0; i < Track::MAX_PLUGIN_EFFECTS; ++i)
    {
        if (auto* plugin = const_cast<Track&>(track).getPluginEffect(i))
        {
            pluginEffectsArray.add(serializePlugin(plugin, track.getPluginEffectDescription(i)));
        }
    }
    obj->setProperty("pluginEffects", pluginEffectsArray);

    // Automation
    auto autoMode = track.getAutomationMode();
    juce::String modeStr;
    switch (autoMode)
    {
        case AutomationMode::Off:   modeStr = "off"; break;
        case AutomationMode::Read:  modeStr = "read"; break;
        case AutomationMode::Write: modeStr = "write"; break;
        case AutomationMode::Touch: modeStr = "touch"; break;
        case AutomationMode::Latch: modeStr = "latch"; break;
    }
    obj->setProperty("automationMode", modeStr);

    juce::Array<juce::var> automationArray;
    for (const auto& lane : track.getAutomationLanes())
    {
        automationArray.add(lane->toVar());
    }
    obj->setProperty("automationLanes", automationArray);

    return juce::var(obj);
}

std::unique_ptr<Track> ProjectSerializer::deserializeTrack(const juce::var& data)
{
    if (!data.isObject())
        return nullptr;

    juce::String name = data.getProperty("name", "Track").toString();
    auto track = std::make_unique<Track>(name);

    // Color
    if (data.hasProperty("color"))
    {
        track->setColour(hexToColour(data["color"].toString()));
    }

    // Mixing params
    track->setVolume(static_cast<float>(data.getProperty("volume", 0.8)));
    track->setPan(static_cast<float>(data.getProperty("pan", 0.0)));
    track->setMuted(static_cast<bool>(data.getProperty("muted", false)));
    track->setSoloed(static_cast<bool>(data.getProperty("soloed", false)));

    // Synth type
    if (data.hasProperty("synthType"))
    {
        juce::String synthTypeName = data["synthType"].toString();
        SynthType synthType = SynthFactory::getSynthType(synthTypeName);
        track->setSynthType(synthType);
    }

    // Synth parameters
    if (data.hasProperty("synthParams") && data["synthParams"].isObject())
    {
        if (auto* synth = track->getSynth())
        {
            const auto& params = data["synthParams"];

            // Load preset first if specified
            if (params.hasProperty("preset"))
            {
                juce::String presetName = params["preset"].toString();
                synth->loadPreset(presetName);
            }

            // Then apply individual parameters (overrides preset values)
            if (auto* dynObj = params.getDynamicObject())
            {
                for (const auto& prop : dynObj->getProperties())
                {
                    if (prop.name.toString() != "preset")
                    {
                        synth->setParameter(prop.name.toString(),
                                           static_cast<float>(prop.value));
                    }
                }
            }
        }
    }

    // Clips
    if (data.hasProperty("clips") && data["clips"].isArray())
    {
        const auto* clipsArray = data["clips"].getArray();
        if (clipsArray)
        {
            for (const auto& clipVar : *clipsArray)
            {
                auto clip = MidiClip::fromVar(clipVar);
                if (clip)
                {
                    track->addClip(std::move(clip));
                }
            }
        }
    }

    // Note: Plugin loading requires PluginHost integration
    // Will be handled by ProjectManager after plugin scanning

    // Automation mode
    if (data.hasProperty("automationMode"))
    {
        juce::String modeStr = data["automationMode"].toString();
        if (modeStr == "off")        track->setAutomationMode(AutomationMode::Off);
        else if (modeStr == "read")  track->setAutomationMode(AutomationMode::Read);
        else if (modeStr == "write") track->setAutomationMode(AutomationMode::Write);
        else if (modeStr == "touch") track->setAutomationMode(AutomationMode::Touch);
        else if (modeStr == "latch") track->setAutomationMode(AutomationMode::Latch);
    }

    // Automation lanes
    if (data.hasProperty("automationLanes") && data["automationLanes"].isArray())
    {
        const auto* lanesArray = data["automationLanes"].getArray();
        if (lanesArray)
        {
            for (const auto& laneVar : *lanesArray)
            {
                auto lane = AutomationLane::fromVar(laneVar);
                if (lane)
                {
                    // Copy points to track's lane
                    auto* trackLane = track->getOrCreateAutomationLane(lane->getParameterId());
                    for (const auto& pt : lane->getPoints())
                    {
                        trackLane->addPoint(pt.timeInBeats, pt.value, pt.curve);
                    }
                }
            }
        }
    }

    return track;
}

//==============================================================================
// Note serialization (individual notes, for compatibility)

juce::var ProjectSerializer::serializeNote(const Note& note)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", note.id.toString());
    obj->setProperty("midiNote", note.midiNote);
    obj->setProperty("startBeat", note.startBeat);
    obj->setProperty("durationBeats", note.durationBeats);
    obj->setProperty("velocity", note.velocity);
    return juce::var(obj);
}

Note ProjectSerializer::deserializeNote(const juce::var& data)
{
    Note note;
    if (data.hasProperty("id"))
        note.id = juce::Uuid(data["id"].toString());
    if (data.hasProperty("midiNote"))
        note.midiNote = static_cast<int>(data["midiNote"]);
    if (data.hasProperty("startBeat"))
        note.startBeat = static_cast<double>(data["startBeat"]);
    if (data.hasProperty("durationBeats"))
        note.durationBeats = static_cast<double>(data["durationBeats"]);
    if (data.hasProperty("velocity"))
        note.velocity = static_cast<float>(data["velocity"]);
    return note;
}

//==============================================================================
// Plugin serialization

juce::var ProjectSerializer::serializePlugin(juce::AudioPluginInstance* plugin,
                                              const juce::PluginDescription* desc)
{
    if (!plugin || !desc)
        return juce::var();

    auto* obj = new juce::DynamicObject();

    obj->setProperty("name", desc->name);
    obj->setProperty("manufacturer", desc->manufacturerName);
    obj->setProperty("format", desc->pluginFormatName);
    obj->setProperty("uid", desc->uniqueId);
    obj->setProperty("fileOrIdentifier", desc->fileOrIdentifier);

    // Encode plugin state as base64
    obj->setProperty("state", encodePluginState(plugin));

    return juce::var(obj);
}

juce::String ProjectSerializer::encodePluginState(juce::AudioPluginInstance* plugin)
{
    if (!plugin)
        return {};

    juce::MemoryBlock state;
    plugin->getStateInformation(state);

    return juce::Base64::toBase64(state.getData(), state.getSize());
}

juce::MemoryBlock ProjectSerializer::decodePluginState(const juce::String& base64State)
{
    juce::MemoryBlock decoded;

    if (base64State.isNotEmpty())
    {
        juce::MemoryOutputStream stream(decoded, false);
        juce::Base64::convertFromBase64(stream, base64State);
    }

    return decoded;
}

//==============================================================================
// Color conversion helpers

juce::String ProjectSerializer::colourToHex(juce::Colour colour)
{
    return "#" + colour.toDisplayString(false); // Without alpha
}

juce::Colour ProjectSerializer::hexToColour(const juce::String& hex)
{
    juce::String cleanHex = hex.trimCharactersAtStart("#");

    if (cleanHex.length() == 6)
    {
        // RGB format - add alpha
        cleanHex = "ff" + cleanHex;
    }

    return juce::Colour(static_cast<juce::uint32>(cleanHex.getHexValue32()));
}
