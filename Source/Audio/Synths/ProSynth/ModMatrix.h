#pragma once

#include <juce_core/juce_core.h>
#include <map>
#include <vector>

/**
 * Modulation sources
 */
enum class ModSource
{
    None = 0,
    Env1, Env2, Env3, Env4,
    LFO1, LFO2, LFO3, LFO4,
    Velocity, Aftertouch, ModWheel, PitchBend,
    Note, Random, Keytrack
};

/**
 * Modulation destinations
 */
enum class ModDestination
{
    None = 0,
    // Oscillator 1
    Osc1_Level, Osc1_Pitch, Osc1_Pan, Osc1_WTPosition,
    // Oscillator 2
    Osc2_Level, Osc2_Pitch, Osc2_Pan, Osc2_WTPosition,
    // Oscillator 3
    Osc3_Level, Osc3_Pitch, Osc3_Pan, Osc3_WTPosition,
    // Filter 1
    Filter1_Cutoff, Filter1_Resonance, Filter1_Drive,
    // Filter 2
    Filter2_Cutoff, Filter2_Resonance,
    // LFOs
    LFO1_Rate, LFO2_Rate, LFO3_Rate, LFO4_Rate,
    // Master
    Master_Volume, Master_Pan
};

/**
 * A single modulation slot
 */
struct ModSlot
{
    ModSource source = ModSource::None;
    ModDestination destination = ModDestination::None;
    float amount = 0.0f; // -1 to 1
    bool enabled = false;
};

/**
 * ModMatrix - Modulation routing matrix
 *
 * Features:
 * - 16 modulation slots
 * - Multiple sources to multiple destinations
 * - Bipolar modulation amount
 * - Enable/disable per slot
 */
class ModMatrix
{
public:
    static constexpr int NUM_SLOTS = 16;

    ModMatrix();

    //==========================================================================
    // Slot access
    const ModSlot& getSlot(int index) const;
    void setSlot(int index, const ModSlot& slot);
    void clearSlot(int index);
    void clearAllSlots();

    //==========================================================================
    // Source values (set by synth engine)
    void setSourceValue(ModSource source, float value);
    float getSourceValue(ModSource source) const;

    //==========================================================================
    // Calculate modulation for a destination
    float getModulationFor(ModDestination dest) const;

    //==========================================================================
    // Utility
    std::vector<int> getActiveSlotsForDestination(ModDestination dest) const;

private:
    std::array<ModSlot, NUM_SLOTS> slots;
    std::map<ModSource, float> sourceValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrix)
};
