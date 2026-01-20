#define TSF_IMPLEMENTATION
#include "External/tsf.h"

#include "SoundFontPlayer.h"

//==============================================================================
// GM Instrument Names (General MIDI standard)
const char* SoundFontPlayer::gmInstrumentNames[128] = {
    // Piano (0-7)
    "Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano",
    "Honky-tonk Piano", "Electric Piano 1", "Electric Piano 2", "Harpsichord", "Clavinet",
    // Chromatic Percussion (8-15)
    "Celesta", "Glockenspiel", "Music Box", "Vibraphone",
    "Marimba", "Xylophone", "Tubular Bells", "Dulcimer",
    // Organ (16-23)
    "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ",
    "Reed Organ", "Accordion", "Harmonica", "Tango Accordion",
    // Guitar (24-31)
    "Acoustic Guitar (nylon)", "Acoustic Guitar (steel)", "Electric Guitar (jazz)",
    "Electric Guitar (clean)", "Electric Guitar (muted)", "Overdriven Guitar",
    "Distortion Guitar", "Guitar Harmonics",
    // Bass (32-39)
    "Acoustic Bass", "Electric Bass (finger)", "Electric Bass (pick)", "Fretless Bass",
    "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2",
    // Strings (40-47)
    "Violin", "Viola", "Cello", "Contrabass",
    "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani",
    // Ensemble (48-55)
    "String Ensemble 1", "String Ensemble 2", "Synth Strings 1", "Synth Strings 2",
    "Choir Aahs", "Voice Oohs", "Synth Choir", "Orchestra Hit",
    // Brass (56-63)
    "Trumpet", "Trombone", "Tuba", "Muted Trumpet",
    "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2",
    // Reed (64-71)
    "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax",
    "Oboe", "English Horn", "Bassoon", "Clarinet",
    // Pipe (72-79)
    "Piccolo", "Flute", "Recorder", "Pan Flute",
    "Blown Bottle", "Shakuhachi", "Whistle", "Ocarina",
    // Synth Lead (80-87)
    "Lead 1 (square)", "Lead 2 (sawtooth)", "Lead 3 (calliope)", "Lead 4 (chiff)",
    "Lead 5 (charang)", "Lead 6 (voice)", "Lead 7 (fifths)", "Lead 8 (bass + lead)",
    // Synth Pad (88-95)
    "Pad 1 (new age)", "Pad 2 (warm)", "Pad 3 (polysynth)", "Pad 4 (choir)",
    "Pad 5 (bowed)", "Pad 6 (metallic)", "Pad 7 (halo)", "Pad 8 (sweep)",
    // Synth Effects (96-103)
    "FX 1 (rain)", "FX 2 (soundtrack)", "FX 3 (crystal)", "FX 4 (atmosphere)",
    "FX 5 (brightness)", "FX 6 (goblins)", "FX 7 (echoes)", "FX 8 (sci-fi)",
    // Ethnic (104-111)
    "Sitar", "Banjo", "Shamisen", "Koto",
    "Kalimba", "Bagpipe", "Fiddle", "Shanai",
    // Percussive (112-119)
    "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock",
    "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal",
    // Sound Effects (120-127)
    "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet",
    "Telephone Ring", "Helicopter", "Applause", "Gunshot"
};

//==============================================================================
SoundFontPlayer::SoundFontPlayer()
{
    // Register parameters
    addParameter("instrument", "Instrument", 0.0f, 0.0f, 127.0f, 1.0f);
    addParameter("bank", "Bank", 0.0f, 0.0f, 128.0f, 1.0f);
    addParameter("volume", "Volume", 0.8f, 0.0f, 1.0f);
    addParameter("pan", "Pan", 0.0f, -1.0f, 1.0f);
    addParameter("pitchBend", "Pitch Bend", 0.5f, 0.0f, 1.0f);
    addParameter("modWheel", "Mod Wheel", 0.0f, 0.0f, 1.0f);
    addParameter("attackOverride", "Attack Override", 0.0f, 0.0f, 1.0f);
    addParameter("releaseOverride", "Release Override", 0.0f, 0.0f, 1.0f);

    // Try to load bundled SoundFont
    loadBundledSoundFont();
}

SoundFontPlayer::~SoundFontPlayer()
{
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);
        soundFont = nullptr;
    }
}

//==============================================================================
void SoundFontPlayer::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    SynthBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    // Resize render buffer for stereo interleaved output
    renderBuffer.resize(static_cast<size_t>(newSamplesPerBlock * 2));

    // Configure TSF output
    if (soundFont != nullptr)
    {
        tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, static_cast<int>(newSampleRate), 0.0f);
    }
}

void SoundFontPlayer::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    // Process MIDI messages
    processMidiMessages(midiMessages);

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (soundFont == nullptr || numSamples == 0)
    {
        buffer.clear();
        return;
    }

    // Ensure render buffer is large enough
    if (renderBuffer.size() < static_cast<size_t>(numSamples * 2))
    {
        renderBuffer.resize(static_cast<size_t>(numSamples * 2));
    }

    // Clear render buffer
    std::fill(renderBuffer.begin(), renderBuffer.end(), 0.0f);

    // Render audio from TinySoundFont
    tsf_render_float(soundFont, renderBuffer.data(), numSamples, 0);

    // Apply volume
    float volume = getParameter("volume");

    // De-interleave to output buffer
    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = renderBuffer[i * 2] * volume;
        if (rightChannel != nullptr)
            rightChannel[i] = renderBuffer[i * 2 + 1] * volume;
    }

    // Apply pan
    float pan = getParameter("pan");
    if (std::abs(pan) > 0.001f && rightChannel != nullptr)
    {
        float leftGain = pan < 0 ? 1.0f : 1.0f - pan;
        float rightGain = pan > 0 ? 1.0f : 1.0f + pan;

        for (int i = 0; i < numSamples; ++i)
        {
            leftChannel[i] *= leftGain;
            rightChannel[i] *= rightGain;
        }
    }

    // Update active notes tracking
    numActiveVoices = tsf_active_voice_count(soundFont);
}

void SoundFontPlayer::releaseResources()
{
    SynthBase::releaseResources();
    renderBuffer.clear();
}

//==============================================================================
void SoundFontPlayer::noteOn(int midiNote, float velocity, int /*sampleOffset*/)
{
    // Track active notes regardless of SoundFont state
    activeNotes.insert(midiNote);

    if (soundFont == nullptr)
        return;

    // Get current instrument settings
    int program = static_cast<int>(getParameter("instrument"));
    int bank = static_cast<int>(getParameter("bank"));

    // Trigger note in TSF
    tsf_note_on(soundFont, tsf_get_presetindex(soundFont, bank, program), midiNote, velocity);
}

void SoundFontPlayer::noteOff(int midiNote, int /*sampleOffset*/)
{
    // Remove from active notes tracking
    activeNotes.erase(midiNote);

    if (soundFont == nullptr)
        return;

    int program = static_cast<int>(getParameter("instrument"));
    int bank = static_cast<int>(getParameter("bank"));

    tsf_note_off(soundFont, tsf_get_presetindex(soundFont, bank, program), midiNote);
}

void SoundFontPlayer::allNotesOff()
{
    if (soundFont != nullptr)
    {
        tsf_note_off_all(soundFont);
    }
    activeNotes.clear();
}

//==============================================================================
bool SoundFontPlayer::loadSoundFont(const juce::String& path)
{
    // Close existing SoundFont
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);
        soundFont = nullptr;
    }

    // Load new SoundFont
    soundFont = tsf_load_filename(path.toRawUTF8());

    if (soundFont != nullptr)
    {
        currentSoundFontPath = path;

        // Configure output if already prepared
        if (sampleRate > 0)
        {
            tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, static_cast<int>(sampleRate), 0.0f);
        }

        return true;
    }

    return false;
}

bool SoundFontPlayer::loadSoundFontFromMemory(const void* data, size_t size)
{
    // Close existing SoundFont
    if (soundFont != nullptr)
    {
        tsf_close(soundFont);
        soundFont = nullptr;
    }

    // Load from memory
    soundFont = tsf_load_memory(data, static_cast<int>(size));

    if (soundFont != nullptr)
    {
        currentSoundFontPath = "(memory)";

        // Configure output if already prepared
        if (sampleRate > 0)
        {
            tsf_set_output(soundFont, TSF_STEREO_INTERLEAVED, static_cast<int>(sampleRate), 0.0f);
        }

        return true;
    }

    return false;
}

bool SoundFontPlayer::isSoundFontLoaded() const
{
    return soundFont != nullptr;
}

juce::String SoundFontPlayer::getCurrentSoundFontPath() const
{
    return currentSoundFontPath;
}

//==============================================================================
juce::String SoundFontPlayer::getInstrumentName(int programNumber)
{
    if (programNumber >= 0 && programNumber < 128)
    {
        return juce::String(gmInstrumentNames[programNumber]);
    }
    return "Unknown";
}

juce::StringArray SoundFontPlayer::getAllInstrumentNames()
{
    juce::StringArray names;
    for (int i = 0; i < 128; ++i)
    {
        names.add(gmInstrumentNames[i]);
    }
    return names;
}

juce::String SoundFontPlayer::getInstrumentCategory(int programNumber)
{
    if (programNumber < 0 || programNumber > 127)
        return "Unknown";

    int category = programNumber / 8;
    static const char* categories[16] = {
        "Piano", "Chromatic Percussion", "Organ", "Guitar",
        "Bass", "Strings", "Ensemble", "Brass",
        "Reed", "Pipe", "Synth Lead", "Synth Pad",
        "Synth Effects", "Ethnic", "Percussive", "Sound Effects"
    };

    return juce::String(categories[category]);
}

//==============================================================================
std::vector<SynthPreset> SoundFontPlayer::getPresets() const
{
    std::vector<SynthPreset> presets;

    // Create presets for each GM category
    static const char* categories[16] = {
        "Piano", "Chromatic Percussion", "Organ", "Guitar",
        "Bass", "Strings", "Ensemble", "Brass",
        "Reed", "Pipe", "Synth Lead", "Synth Pad",
        "Synth Effects", "Ethnic", "Percussive", "Sound Effects"
    };

    // Add first instrument from each category as a preset
    for (int cat = 0; cat < 16; ++cat)
    {
        SynthPreset preset;
        preset.name = gmInstrumentNames[cat * 8];
        preset.category = categories[cat];
        preset.values["instrument"] = static_cast<float>(cat * 8);
        preset.values["bank"] = 0.0f;
        preset.values["volume"] = 0.8f;
        preset.values["pan"] = 0.0f;
        presets.push_back(preset);
    }

    return presets;
}

//==============================================================================
void SoundFontPlayer::onParameterChanged(const juce::String& name, float value)
{
    if (soundFont == nullptr)
        return;

    if (name == "instrument" || name == "bank")
    {
        currentProgram = static_cast<int>(getParameter("instrument"));
        currentBank = static_cast<int>(getParameter("bank"));
    }
    else if (name == "pitchBend")
    {
        // Convert 0-1 to pitch bend range (typically +/- 2 semitones)
        float bendAmount = (value - 0.5f) * 2.0f; // -1 to +1
        int bendCents = static_cast<int>(bendAmount * 200); // +/- 200 cents
        tsf_channel_set_pitchwheel(soundFont, 0, 8192 + bendCents * 8192 / 200);
    }
}

//==============================================================================
void SoundFontPlayer::loadBundledSoundFont()
{
    // Try to find bundled SoundFont in Resources
    juce::File appDir = juce::File::getSpecialLocation(
        juce::File::currentApplicationFile);

    // Try common locations for bundled SoundFont
    juce::StringArray searchPaths = {
        appDir.getChildFile("Contents/Resources/GeneralUser.sf2").getFullPathName(),
        appDir.getChildFile("Contents/Resources/gm.sf2").getFullPathName(),
        appDir.getSiblingFile("Resources/GeneralUser.sf2").getFullPathName(),
        "/usr/share/sounds/sf2/FluidR3_GM.sf2",  // Linux
        "/usr/share/soundfonts/FluidR3_GM.sf2"   // Linux alt
    };

    for (const auto& path : searchPaths)
    {
        juce::File sf2File(path);
        if (sf2File.existsAsFile())
        {
            if (loadSoundFont(path))
            {
                DBG("Loaded bundled SoundFont: " + path);
                return;
            }
        }
    }

    DBG("No bundled SoundFont found - SoundFontPlayer will be silent until SF2 loaded");
}

void SoundFontPlayer::updateTSFSettings()
{
    // Future: apply envelope overrides, etc.
}
