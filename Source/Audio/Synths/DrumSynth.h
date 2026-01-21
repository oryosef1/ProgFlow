#pragma once

#include "SynthBase.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <array>

/**
 * DrumPad - A single drum sound synthesizer
 *
 * Each pad can generate different drum sounds based on synthesis.
 */
struct DrumPad
{
    juce::String name;
    int midiNote = 36;        // MIDI note that triggers this pad
    int chokeGroup = -1;      // -1 = no choke, same group = mutual choke

    // Synthesis parameters
    float pitch = 1.0f;       // Pitch multiplier (0.5 - 2.0)
    float decay = 0.5f;       // Decay time (0.0 - 1.0)
    float tone = 0.5f;        // Tone/brightness (0.0 - 1.0)
    float level = 0.8f;       // Output level (0.0 - 1.0)
    float pan = 0.0f;         // Stereo pan (-1.0 to 1.0)

    // Sound type
    enum class SoundType
    {
        Kick,
        Snare,
        ClosedHiHat,
        OpenHiHat,
        Tom,
        Clap,
        Rim,
        Cymbal,
        Cowbell,
        Clave,
        Shaker,
        Conga  // For Perc 2 - Latin percussion character
    };
    SoundType soundType = SoundType::Kick;

    // Playback state
    bool playing = false;
    float phase = 0.0f;
    float envelope = 0.0f;
    float velocity = 1.0f;    // Stored velocity from noteOn
    float noisePhase = 0.0f;
    float clickEnv = 0.0f;
};

/**
 * DrumSynth - A synthesis-based drum machine
 *
 * Features:
 * - 16 drum pads with different sounds
 * - Multiple kits (808, 909, Acoustic, Lo-Fi, Trap)
 * - Per-pad parameters (pitch, decay, tone, level, pan)
 * - Hi-hat choke groups
 * - All synthesis-based, no samples required
 */
class DrumSynth : public SynthBase
{
public:
    DrumSynth();
    ~DrumSynth() override = default;

    //==========================================================================
    // SynthBase overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override;
    void releaseResources() override;

    void noteOn(int midiNote, float velocity, int sampleOffset = 0) override;
    void noteOff(int midiNote, int sampleOffset = 0) override;
    void allNotesOff() override;

    std::vector<SynthPreset> getPresets() const override;

    //==========================================================================
    // Drum-specific methods

    /**
     * Get number of drum pads (always 16)
     */
    int getNumPads() const { return NUM_PADS; }

    /**
     * Get the name of a pad
     */
    juce::String getPadName(int padIndex) const;

    /**
     * Set a parameter for a specific pad
     * @param padIndex 0-15
     * @param paramName "pitch", "decay", "tone", "level", "pan"
     * @param value Parameter value
     */
    void setPadParameter(int padIndex, const juce::String& paramName, float value);

    /**
     * Get a parameter for a specific pad
     */
    float getPadParameter(int padIndex, const juce::String& paramName) const;

    /**
     * Check if a specific MIDI note is currently active
     */
    bool isNoteActive(int midiNote) const;

    /**
     * Get the MIDI note assigned to a specific pad
     */
    int getPadMidiNote(int padIndex) const;

    //==========================================================================
    // Kit management

    /**
     * Load a drum kit by name
     */
    void loadKit(const juce::String& kitName);

    /**
     * Get current kit name
     */
    juce::String getCurrentKit() const { return currentKit; }

    /**
     * Get list of available kits
     */
    juce::StringArray getAvailableKits() const;

    //==========================================================================
    // MIDI note mapping (GM drum map)
    static constexpr int KICK_NOTE = 36;
    static constexpr int SNARE_NOTE = 38;
    static constexpr int CLOSED_HH_NOTE = 42;
    static constexpr int OPEN_HH_NOTE = 46;
    static constexpr int TOM_LOW_NOTE = 41;
    static constexpr int TOM_MID_NOTE = 47;
    static constexpr int TOM_HIGH_NOTE = 50;
    static constexpr int CLAP_NOTE = 39;
    static constexpr int RIM_NOTE = 37;
    static constexpr int CRASH_NOTE = 49;
    static constexpr int RIDE_NOTE = 51;
    static constexpr int COWBELL_NOTE = 56;
    static constexpr int CLAVE_NOTE = 75;
    static constexpr int SHAKER_NOTE = 70;

protected:
    void onParameterChanged(const juce::String& name, float value) override;
    void onParameterEnumChanged(const juce::String& name, int index) override;

private:
    static constexpr int NUM_PADS = 16;

    // Drum pads
    std::array<DrumPad, NUM_PADS> pads;

    // Current kit
    juce::String currentKit = "808";

    // Audio processing
    double sampleRateLocal = 44100.0;

    // Synthesis helpers
    float synthesizeKick(DrumPad& pad, float velocity);
    float synthesizeSnare(DrumPad& pad, float velocity);
    float synthesizeHiHat(DrumPad& pad, float velocity, bool open);
    float synthesizeTom(DrumPad& pad, float velocity);
    float synthesizeClap(DrumPad& pad, float velocity);
    float synthesizeRim(DrumPad& pad, float velocity);
    float synthesizeCymbal(DrumPad& pad, float velocity);
    float synthesizeCowbell(DrumPad& pad, float velocity);
    float synthesizeClave(DrumPad& pad, float velocity);
    float synthesizeShaker(DrumPad& pad, float velocity);
    float synthesizeConga(DrumPad& pad, float velocity);

    // Random noise generator
    juce::Random noiseRandom;

    // Find pad by MIDI note
    int findPadByNote(int midiNote) const;

    // Choke pads in the same group
    void chokePadsInGroup(int chokeGroup, int exceptPad);

    // Initialize pads with default layout
    void initializePads();

    // Kit configurations
    void configure808Kit();
    void configure909Kit();
    void configureAcousticKit();
    void configureLoFiKit();
    void configureTrapKit();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSynth)
};
