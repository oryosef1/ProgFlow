#pragma once

#include "SynthBase.h"
#include <memory>

/**
 * SynthType - Enumeration of available synth types
 */
enum class SynthType
{
    Analog = 0,
    FM,
    Pro,
    Sampler,
    SoundFont,
    Drums,

    COUNT  // Number of synth types (6 synths)
};

/**
 * SynthFactory - Creates synth instances by type
 *
 * Usage:
 *   auto synth = SynthFactory::createSynth(SynthType::Analog);
 */
class SynthFactory
{
public:
    /**
     * Create a new synth instance of the given type
     */
    static std::unique_ptr<SynthBase> createSynth(SynthType type);

    /**
     * Get the display name for a synth type
     */
    static juce::String getSynthName(SynthType type);

    /**
     * Get synth type from index (for ComboBox)
     */
    static SynthType getSynthType(int index);

    /**
     * Get synth type from name string (for deserialization)
     */
    static SynthType getSynthType(const juce::String& name);

    /**
     * Get all synth names for populating a ComboBox
     */
    static juce::StringArray getAllSynthNames();

    /**
     * Get number of synth types available
     */
    static int getNumSynthTypes() { return static_cast<int>(SynthType::COUNT); }

private:
    SynthFactory() = delete;  // Static-only class
};
