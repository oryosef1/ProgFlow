#pragma once

#include "SynthBase.h"
#include "SynthVoice.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Sample zone - maps a sample to a MIDI note range
 */
struct SampleZone
{
    juce::String id;
    juce::String name;
    juce::AudioBuffer<float> sampleData;
    double sampleRate = 44100.0;

    int rootNote = 60;  // C4
    int lowNote = 0;    // Lowest MIDI note for this zone
    int highNote = 127; // Highest MIDI note for this zone

    float volumeDb = 0.0f;

    bool loopEnabled = false;
    int loopStart = 0;
    int loopEnd = -1; // -1 = end of sample

    SampleZone() = default;
    SampleZone(const juce::String& zoneId, const juce::String& zoneName, int root)
        : id(zoneId), name(zoneName), rootNote(root), lowNote(root), highNote(root)
    {}
};

/**
 * SamplerVoice - A single voice for sample playback
 *
 * Signal chain per voice:
 * Sample Buffer → Pitch Shifter → Filter → AmpEnv → Output
 */
class SamplerVoice : public SynthVoice
{
public:
    SamplerVoice();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void renderNextBlock(juce::AudioBuffer<float>& buffer,
                        int startSample, int numSamples) override;

    //==========================================================================
    // Sample assignment
    void setSample(const SampleZone* zone);

    //==========================================================================
    // Playback settings
    void setLoopMode(bool enabled);
    void setStartPosition(float normalizedPosition); // 0.0 to 1.0
    void setTranspose(int semitones);
    void setFineTune(float cents);

    //==========================================================================
    // Filter settings
    void setFilterCutoff(float frequency);
    void setFilterResonance(float resonance);
    void setFilterEnvAmount(float amount);
    void setFilterEnvelope(float attack, float decay, float sustain, float release);

protected:
    void onNoteStart() override;
    void onNoteStop() override;

private:
    // Sample reference
    const SampleZone* currentZone = nullptr;

    // Playback state
    double samplePosition = 0.0;
    double playbackRate = 1.0;
    float startPosition = 0.0f; // 0.0 to 1.0
    bool looping = false;

    // Transpose settings
    int transpose = 0;
    float fineTune = 0.0f; // In cents

    // Filter - using StateVariableTPT
    juce::dsp::StateVariableTPTFilter<float> filter;
    float filterCutoff = 20000.0f; // Default open filter
    float filterResonance = 0.1f;
    float filterEnvAmount = 0.0f;

    // Filter envelope
    juce::ADSR filterEnvelope;
    juce::ADSR::Parameters filterEnvParams{0.01f, 0.1f, 1.0f, 0.3f};

    // Helper to calculate playback rate from target note
    double calculatePlaybackRate(int targetNote);

    // Helper to read interpolated sample
    float getInterpolatedSample(double position);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerVoice)
};

//==============================================================================

/**
 * Sampler - Multi-zone sample playback instrument
 *
 * Features:
 * - Load audio files (WAV, AIFF, FLAC, MP3, OGG)
 * - Multi-zone mapping (assign samples to note ranges)
 * - Pitch shifting with interpolation
 * - Loop modes (one-shot, forward loop)
 * - ADSR envelope
 * - Filter with envelope
 * - Sample start position
 * - Transpose and fine tune
 */
class Sampler : public SynthBase
{
public:
    Sampler();
    ~Sampler() override;

    //==========================================================================
    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer,
                     juce::MidiBuffer& midiMessages) override;
    void releaseResources() override;

    //==========================================================================
    // Note handling
    void noteOn(int midiNote, float velocity, int sampleOffset = 0) override;
    void noteOff(int midiNote, int sampleOffset = 0) override;
    void allNotesOff() override;
    void killAllNotes() override;

    //==========================================================================
    // Sample management
    bool loadSample(const juce::File& file, int rootNote, int lowNote = -1, int highNote = -1);
    bool loadSample(const juce::String& zoneId, const juce::String& name,
                   const juce::AudioBuffer<float>& samples, double sr,
                   int rootNote, int lowNote = -1, int highNote = -1);
    void removeSample(const juce::String& zoneId);
    void clearAllSamples();

    std::vector<SampleZone*> getZones();
    const SampleZone* findZoneForNote(int midiNote) const;

    //==========================================================================
    // Audio format manager
    juce::AudioFormatManager& getFormatManager() { return formatManager; }

    //==========================================================================
    // Presets
    std::vector<SynthPreset> getPresets() const override;

    static constexpr int MAX_VOICES = 16;

protected:
    void onParameterChanged(const juce::String& name, float value) override;
    void onParameterEnumChanged(const juce::String& name, int index) override;

private:
    // Audio format manager for loading files
    juce::AudioFormatManager formatManager;

    // Voice pool
    std::array<std::unique_ptr<SamplerVoice>, MAX_VOICES> voices;

    // Sample zones
    std::vector<std::unique_ptr<SampleZone>> zones;

    // Initialize all parameters
    void initializeParameters();

    // Update all voice parameters
    void updateVoiceParameters();

    // Voice allocation
    SamplerVoice* findFreeVoice();
    SamplerVoice* findVoiceToSteal();
    SamplerVoice* findVoicePlayingNote(int midiNote);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sampler)
};
