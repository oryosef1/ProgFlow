#pragma once

#include "SynthBase.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

// Forward declaration for TinySoundFont handle
struct tsf;

/**
 * SoundFontPlayer - A SoundFont (.sf2) based synthesizer
 *
 * Uses TinySoundFont for SF2 parsing and rendering.
 * Provides access to 128 General MIDI instruments.
 *
 * Parameters:
 *   - instrument: 0-127 (GM program number)
 *   - bank: 0-128 (0 = melodic, 128 = percussion)
 *   - volume: 0-1 (output level)
 *   - pan: -1 to 1 (stereo position)
 *   - pitchBend: 0-1 (0.5 = center)
 *   - modWheel: 0-1 (modulation amount)
 *   - attackOverride: 0-1 (envelope attack override)
 *   - releaseOverride: 0-1 (envelope release override)
 */
class SoundFontPlayer : public SynthBase
{
public:
    SoundFontPlayer();
    ~SoundFontPlayer() override;

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
    // SoundFont specific methods

    /**
     * Load a SoundFont file
     * @param path Path to the .sf2 file
     * @return true if loaded successfully
     */
    bool loadSoundFont(const juce::String& path);

    /**
     * Load SoundFont from memory
     * @param data Pointer to SF2 data
     * @param size Size in bytes
     * @return true if loaded successfully
     */
    bool loadSoundFontFromMemory(const void* data, size_t size);

    /**
     * Check if a SoundFont is currently loaded
     */
    bool isSoundFontLoaded() const;

    /**
     * Get path to currently loaded SoundFont
     */
    juce::String getCurrentSoundFontPath() const;

    /**
     * Get the name of a GM instrument by program number
     * @param programNumber 0-127
     */
    static juce::String getInstrumentName(int programNumber);

    /**
     * Get all GM instrument names
     * @return Array of 128 instrument names
     */
    static juce::StringArray getAllInstrumentNames();

    /**
     * Get instrument category (Piano, Chromatic Percussion, etc.)
     * @param programNumber 0-127
     */
    static juce::String getInstrumentCategory(int programNumber);

protected:
    void onParameterChanged(const juce::String& name, float value) override;

private:
    // TinySoundFont instance
    tsf* soundFont = nullptr;
    juce::String currentSoundFontPath;

    // Rendering buffer (interleaved stereo)
    std::vector<float> renderBuffer;

    // Current settings
    int currentProgram = 0;
    int currentBank = 0;

    // Voice tracking for active notes
    struct VoiceInfo
    {
        int midiNote = -1;
        bool active = false;
    };
    static constexpr int MAX_VOICES = 64;
    std::array<VoiceInfo, MAX_VOICES> voices;
    int numActiveVoices = 0;

    // Load bundled SoundFont
    void loadBundledSoundFont();

    // Apply parameter changes to TSF
    void updateTSFSettings();

    // GM instrument names (128 entries)
    static const char* gmInstrumentNames[128];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SoundFontPlayer)
};
