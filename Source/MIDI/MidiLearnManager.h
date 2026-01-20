#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>
#include <map>
#include <functional>

/**
 * MidiLearnManager - Handles MIDI CC mapping to parameters
 *
 * Features:
 * - MIDI learn mode: Listen for incoming CC and assign to a target parameter
 * - Store mappings: {parameterId -> {channel, cc, min, max}}
 * - Apply CC values to mapped parameters
 * - Save/load mappings to preferences
 */
class MidiLearnManager : public juce::MidiInputCallback
{
public:
    MidiLearnManager();
    ~MidiLearnManager() override;

    // Singleton access
    static MidiLearnManager& getInstance();

    //==========================================================================
    // MIDI Learn Mode

    /**
     * Start learning mode for a specific parameter
     * @param parameterId Unique identifier for the parameter (e.g., "track.0.volume")
     * @param callback Called when CC is learned with (channel, cc) or canceled (channel=-1)
     */
    void startLearning(const juce::String& parameterId,
                       std::function<void(int channel, int cc)> callback);

    /**
     * Cancel learning mode
     */
    void cancelLearning();

    /**
     * Check if currently in learning mode
     */
    bool isLearning() const { return learningActive; }

    /**
     * Get the parameter currently being learned
     */
    juce::String getLearningParameterId() const { return learningParameterId; }

    //==========================================================================
    // Mapping Management

    struct MidiMapping
    {
        int channel = -1;       // -1 means any channel
        int ccNumber = 0;
        float minValue = 0.0f;  // Mapped value at CC 0
        float maxValue = 1.0f;  // Mapped value at CC 127
        bool enabled = true;
    };

    /**
     * Add or update a mapping for a parameter
     */
    void setMapping(const juce::String& parameterId, const MidiMapping& mapping);

    /**
     * Remove a mapping
     */
    void removeMapping(const juce::String& parameterId);

    /**
     * Get mapping for a parameter (returns nullptr if not mapped)
     */
    const MidiMapping* getMapping(const juce::String& parameterId) const;

    /**
     * Check if a parameter has a mapping
     */
    bool hasMapping(const juce::String& parameterId) const;

    /**
     * Get all mappings
     */
    const std::map<juce::String, MidiMapping>& getAllMappings() const { return mappings; }

    /**
     * Clear all mappings
     */
    void clearAllMappings();

    //==========================================================================
    // MIDI Input Handling

    /**
     * Set the MIDI input device to listen to
     * @param deviceName Device name, empty string disables MIDI input
     */
    void setMidiInputDevice(const juce::String& deviceName);

    /**
     * Get current MIDI input device name
     */
    juce::String getMidiInputDevice() const { return currentMidiInputDevice; }

    //==========================================================================
    // Parameter Value Callbacks

    /**
     * Register a callback for when a mapped parameter should change
     * @param parameterId Parameter ID to listen for
     * @param callback Called with the new value (0.0-1.0 range)
     */
    void registerParameterCallback(const juce::String& parameterId,
                                   std::function<void(float value)> callback);

    /**
     * Unregister a parameter callback
     */
    void unregisterParameterCallback(const juce::String& parameterId);

    //==========================================================================
    // Persistence

    /**
     * Save mappings to JSON
     */
    juce::var toJson() const;

    /**
     * Load mappings from JSON
     */
    void fromJson(const juce::var& json);

    //==========================================================================
    // MidiInputCallback

    void handleIncomingMidiMessage(juce::MidiInput* source,
                                   const juce::MidiMessage& message) override;

private:
    // Learning state
    bool learningActive = false;
    juce::String learningParameterId;
    std::function<void(int channel, int cc)> learningCallback;

    // Mappings
    std::map<juce::String, MidiMapping> mappings;

    // Parameter callbacks
    std::map<juce::String, std::function<void(float value)>> parameterCallbacks;

    // MIDI input
    juce::String currentMidiInputDevice;
    std::unique_ptr<juce::MidiInput> midiInput;

    // Reverse lookup: (channel*128 + ccNumber) -> parameterId
    std::map<int, juce::String> ccToParameterLookup;

    void rebuildReverseLookup();
    float ccToNormalizedValue(int ccValue, const MidiMapping& mapping) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiLearnManager)
};
