#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include "MidiClip.h"
#include "AudioClip.h"
#include "AutomationLane.h"
#include "Synths/SynthFactory.h"
#include <array>
#include <memory>
#include <vector>

/**
 * AutomationMode - Controls how automation is processed
 */
enum class AutomationMode
{
    Off,    // Automation ignored
    Read,   // Playback only
    Write,  // Record everything (destructive)
    Touch,  // Record while touching control, snap back on release
    Latch   // Record while touching, hold value after release
};

/**
 * Track - Base class for all track types
 *
 * A track contains:
 * - An instrument (synth, sampler, or plugin)
 * - An effect chain
 * - Volume, pan, mute, solo controls
 * - Clips with MIDI/audio data
 * - Automation lanes
 */
class Track
{
public:
    Track(const juce::String& name = "Track");
    virtual ~Track() = default;

    // Unique identifier
    const juce::Uuid& getId() const { return id; }

    //==========================================================================
    // Audio processing
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);
    virtual void processBlock(juce::AudioBuffer<float>& buffer, int numSamples,
                              double positionInBeats, double bpm);
    virtual void releaseResources();

    //==========================================================================
    // Track properties
    const juce::String& getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }

    juce::Colour getColour() const { return colour; }
    void setColour(juce::Colour newColour) { colour = newColour; }

    //==========================================================================
    // Mixing controls (atomic for thread-safe access from UI)
    void setVolume(float volume) { this->volume.store(juce::jlimit(0.0f, 2.0f, volume)); }
    float getVolume() const { return volume.load(); }

    void setPan(float pan) { this->pan.store(juce::jlimit(-1.0f, 1.0f, pan)); }
    float getPan() const { return pan.load(); }

    void setMuted(bool muted) { this->muted.store(muted); }
    bool isMuted() const { return muted.load(); }

    void setSoloed(bool soloed) { this->soloed.store(soloed); }
    bool isSoloed() const { return soloed.load(); }

    void setArmed(bool armed) { this->armed.store(armed); }
    bool isArmed() const { return armed.load(); }

    //==========================================================================
    // Metering
    float getMeterLevel() const { return meterLevel.load(); }

    //==========================================================================
    // Synth/Instrument
    SynthBase* getSynth() { return synth.get(); }
    const SynthBase* getSynth() const { return synth.get(); }
    SynthType getSynthType() const { return synthType; }
    void setSynthType(SynthType type);

    // Synth MIDI control (with optional sample offset for accurate timing)
    void synthNoteOn(int midiNote, float velocity, int sampleOffset = 0);
    void synthNoteOff(int midiNote, int sampleOffset = 0);
    void synthAllNotesOff();

    //==========================================================================
    // Plugin Instrument (alternative to built-in synth)
    bool hasPluginInstrument() const { return pluginInstrument != nullptr; }
    juce::AudioPluginInstance* getPluginInstrument() { return pluginInstrument.get(); }
    const juce::AudioPluginInstance* getPluginInstrument() const { return pluginInstrument.get(); }
    void setPluginInstrument(std::unique_ptr<juce::AudioPluginInstance> plugin);
    void clearPluginInstrument();
    const juce::PluginDescription* getPluginInstrumentDescription() const;

    // Plugin effect slots (insert effects)
    static constexpr int MAX_PLUGIN_EFFECTS = 8;
    juce::AudioPluginInstance* getPluginEffect(int slot);
    const juce::AudioPluginInstance* getPluginEffect(int slot) const;
    void setPluginEffect(int slot, std::unique_ptr<juce::AudioPluginInstance> plugin);
    void clearPluginEffect(int slot);
    int getNumPluginEffects() const;
    const juce::PluginDescription* getPluginEffectDescription(int slot) const;

    //==========================================================================
    // Automation
    void setAutomationMode(AutomationMode mode) { automationMode = mode; }
    AutomationMode getAutomationMode() const { return automationMode; }

    // Lane management
    AutomationLane* getAutomationLane(const juce::String& parameterId);
    const AutomationLane* getAutomationLane(const juce::String& parameterId) const;
    AutomationLane* getOrCreateAutomationLane(const juce::String& parameterId);
    void removeAutomationLane(const juce::String& parameterId);
    const std::vector<std::unique_ptr<AutomationLane>>& getAutomationLanes() const { return automationLanes; }

    // Get list of automatable parameters for this track
    std::vector<juce::String> getAutomatableParameters() const;

    //==========================================================================
    // Clip Management
    MidiClip* addClip(double startBar = 0.0, double durationBars = 4.0);
    MidiClip* addClip(std::unique_ptr<MidiClip> clip);
    void removeClip(const juce::Uuid& clipId);
    MidiClip* getClip(const juce::Uuid& clipId);
    const MidiClip* getClip(const juce::Uuid& clipId) const;
    MidiClip* getClipAt(double barPosition);

    const std::vector<std::unique_ptr<MidiClip>>& getClips() const { return clips; }
    size_t getNumClips() const { return clips.size(); }

    // Get clips overlapping a bar range (for playback scheduling)
    void getClipsInRange(double startBar, double endBar,
                         std::vector<MidiClip*>& result);

    // Sort clips by start position
    void sortClips();

    //==========================================================================
    // Recording (with overflow support)

    /**
     * Record a note at a specific bar position.
     * If no clip exists at that position, creates one.
     * If the note extends past the end of an existing clip:
     * - Extends the clip if extensible (e.g., recording in progress)
     * - Or creates a new "overflow" clip
     *
     * @param barPosition Position in bars (project time)
     * @param midiNote MIDI note number (0-127)
     * @param durationBars Note duration in bars
     * @param velocity Note velocity (0.0-1.0)
     * @param extendClip If true, extends existing clip rather than creating new one
     * @return Pointer to the note's clip
     */
    MidiClip* recordNoteAtPosition(double barPosition, int midiNote,
                                    double durationBars, float velocity = 0.8f,
                                    bool extendClip = true);

    /**
     * Get or create a clip suitable for recording at the given position.
     * Used by recordNoteAtPosition internally.
     * @param barPosition Position in bars
     * @param minDuration Minimum clip duration if creating new
     * @param extendExisting If true, extends existing clip to cover position
     */
    MidiClip* getOrCreateClipForRecording(double barPosition,
                                           double minDuration = 4.0,
                                           bool extendExisting = true);

    //==========================================================================
    // Audio Clip Management
    AudioClip* addAudioClip(double startBeat = 0.0);
    AudioClip* addAudioClip(std::unique_ptr<AudioClip> clip);
    void removeAudioClip(const juce::String& clipId);
    AudioClip* getAudioClip(const juce::String& clipId);
    const AudioClip* getAudioClip(const juce::String& clipId) const;
    AudioClip* getAudioClipAt(double beatPosition);

    const std::vector<std::unique_ptr<AudioClip>>& getAudioClips() const { return audioClips; }
    size_t getNumAudioClips() const { return audioClips.size(); }

    // Get audio clips overlapping a beat range (for playback)
    void getAudioClipsInRange(double startBeat, double endBeat, double bpm,
                               std::vector<AudioClip*>& result);

    // Sort audio clips by start position
    void sortAudioClips();

protected:
    // Track identity
    juce::Uuid id;
    juce::String name;
    juce::Colour colour{0xff3b82f6}; // Default blue

    // MIDI Clips
    std::vector<std::unique_ptr<MidiClip>> clips;
    juce::CriticalSection clipLock;  // Thread safety for clip modifications

    // Audio Clips
    std::vector<std::unique_ptr<AudioClip>> audioClips;
    juce::CriticalSection audioClipLock;

    // Automation
    std::vector<std::unique_ptr<AutomationLane>> automationLanes;
    AutomationMode automationMode = AutomationMode::Read;

    // Synth/Instrument
    std::unique_ptr<SynthBase> synth;
    SynthType synthType = SynthType::Analog;
    juce::MidiBuffer synthMidiBuffer;  // For collecting MIDI events
    juce::CriticalSection synthLock;

    // Plugin instrument (alternative to built-in synth)
    std::unique_ptr<juce::AudioPluginInstance> pluginInstrument;
    std::unique_ptr<juce::PluginDescription> pluginInstrumentDesc;
    bool usePluginInstrument = false;

    // Plugin effect slots
    std::array<std::unique_ptr<juce::AudioPluginInstance>, MAX_PLUGIN_EFFECTS> pluginEffects;
    std::array<std::unique_ptr<juce::PluginDescription>, MAX_PLUGIN_EFFECTS> pluginEffectDescs;

    // Mixing (atomic for thread-safe UI access)
    std::atomic<float> volume{1.0f};
    std::atomic<float> pan{0.0f};
    std::atomic<bool> muted{false};
    std::atomic<bool> soloed{false};
    std::atomic<bool> armed{false};

    // Metering
    std::atomic<float> meterLevel{0.0f};

    // Audio settings
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Apply volume and pan to buffer
    void applyGainAndPan(juce::AudioBuffer<float>& buffer);

    // Update meter level
    void updateMeter(const juce::AudioBuffer<float>& buffer);

    // Apply automation at given position
    void applyAutomation(double positionInBeats);

    // Process audio clips into buffer
    void processAudioClips(juce::AudioBuffer<float>& buffer, int numSamples,
                           double positionInBeats, double bpm);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
