#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <map>
#include <set>

/**
 * SynthParameter - A single synth parameter with metadata
 */
struct SynthParameter
{
    juce::String name;
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float step = 0.0f; // 0 = continuous

    // For enum-style parameters
    juce::StringArray options;
    int enumIndex = 0;

    bool isEnum() const { return options.size() > 0; }

    // Helper to get normalized 0-1 value
    float getNormalized() const
    {
        if (maxValue == minValue) return 0.0f;
        return (value - minValue) / (maxValue - minValue);
    }

    void setFromNormalized(float normalized)
    {
        value = minValue + normalized * (maxValue - minValue);
        if (step > 0.0f)
            value = std::round(value / step) * step;
    }
};

/**
 * Preset - A named collection of parameter values
 */
struct SynthPreset
{
    juce::String name;
    juce::String category;
    std::map<juce::String, float> values;
    std::map<juce::String, int> enumValues; // For enum parameters
};

/**
 * SynthBase - Abstract base class for all synthesizers
 *
 * Provides:
 * - Parameter management with thread-safe access
 * - MIDI note handling (noteOn, noteOff, allNotesOff)
 * - Preset management
 * - Audio processing interface
 *
 * Subclasses implement the actual DSP in processBlock()
 */
class SynthBase
{
public:
    SynthBase();
    virtual ~SynthBase() = default;

    //==========================================================================
    // Audio processing
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);
    virtual void processBlock(juce::AudioBuffer<float>& buffer,
                             juce::MidiBuffer& midiMessages) = 0;
    virtual void releaseResources();

    //==========================================================================
    // MIDI handling
    virtual void noteOn(int midiNote, float velocity, int sampleOffset = 0) = 0;
    virtual void noteOff(int midiNote, int sampleOffset = 0) = 0;
    virtual void allNotesOff();
    virtual void killAllNotes(); // Hard stop, no release

    // Convert between MIDI note and frequency
    static float midiToFrequency(int midiNote);
    static int frequencyToMidi(float frequency);
    static juce::String midiToNoteName(int midiNote);

    //==========================================================================
    // Parameter management
    void setParameter(const juce::String& name, float value);
    void setParameterEnum(const juce::String& name, int index);
    void setParameterEnum(const juce::String& name, const juce::String& optionName);
    float getParameter(const juce::String& name) const;
    int getParameterEnum(const juce::String& name) const;
    const SynthParameter* getParameterInfo(const juce::String& name) const;
    std::vector<juce::String> getParameterNames() const;

    //==========================================================================
    // Presets
    virtual std::vector<SynthPreset> getPresets() const { return {}; }
    void loadPreset(const SynthPreset& preset);
    void loadPreset(int index);
    void loadPreset(const juce::String& presetName);
    SynthPreset getCurrentAsPreset(const juce::String& name) const;
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    juce::String getCurrentPresetName() const;

    //==========================================================================
    // Serialization support
    std::map<juce::String, float> getParameters() const;

    //==========================================================================
    // State
    const std::set<int>& getActiveNotes() const { return activeNotes; }
    bool hasActiveNotes() const { return !activeNotes.empty(); }

    double getSampleRate() const { return sampleRate; }
    int getBlockSize() const { return samplesPerBlock; }

    //==========================================================================
    // Tempo sync (for LFOs, arpeggiators, etc.)
    virtual void setBpm(double newBpm) { currentBpm = newBpm; }
    double getBpm() const { return currentBpm; }

protected:
    // Parameter storage - use CriticalSection for thread safety
    std::map<juce::String, SynthParameter> parameters;
    mutable juce::CriticalSection parameterLock;

    // Active notes being played
    std::set<int> activeNotes;

    // Audio settings
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Tempo sync
    double currentBpm = 120.0;

    // Current preset
    int currentPresetIndex = -1;

    // Helper for subclasses to register parameters
    void addParameter(const juce::String& id, const juce::String& name,
                     float defaultValue, float minValue, float maxValue,
                     float step = 0.0f);
    void addEnumParameter(const juce::String& id, const juce::String& name,
                         const juce::StringArray& options, int defaultIndex = 0);

    // Called when a parameter changes - override to update DSP
    virtual void onParameterChanged(const juce::String& name, float value) {}
    virtual void onParameterEnumChanged(const juce::String& name, int index) {}

    // Process MIDI messages within a block
    void processMidiMessages(juce::MidiBuffer& midiMessages);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthBase)
};
