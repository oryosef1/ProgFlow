#pragma once

#include "EffectBase.h"
#include <memory>
#include <vector>

/**
 * EffectChain - Manages a series of effects
 *
 * Signal flow:
 * input → effect[0] → effect[1] → ... → effect[n] → output
 *
 * Features:
 * - Up to 8 effect slots
 * - Per-slot bypass
 * - Reorder effects
 * - Add/remove effects dynamically
 */
class EffectChain
{
public:
    static constexpr int MAX_EFFECTS = 8;

    EffectChain();
    ~EffectChain() = default;

    //==========================================================================
    // Audio processing
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void processBlock(juce::AudioBuffer<float>& buffer);
    void releaseResources();

    //==========================================================================
    // Effect management

    // Add effect to next available slot (returns slot index, or -1 if full)
    int addEffect(std::unique_ptr<EffectBase> effect);

    // Insert effect at specific slot (shifts existing effects)
    void insertEffect(int slot, std::unique_ptr<EffectBase> effect);

    // Remove effect from slot
    std::unique_ptr<EffectBase> removeEffect(int slot);

    // Replace effect at slot
    std::unique_ptr<EffectBase> replaceEffect(int slot, std::unique_ptr<EffectBase> effect);

    // Swap two effects
    void swapEffects(int slot1, int slot2);

    // Move effect from one slot to another
    void moveEffect(int fromSlot, int toSlot);

    // Clear all effects
    void clearAll();

    //==========================================================================
    // Effect access
    EffectBase* getEffect(int slot);
    const EffectBase* getEffect(int slot) const;
    int getNumEffects() const;
    int getNumActiveSlots() const;

    //==========================================================================
    // Per-slot bypass
    void setSlotBypass(int slot, bool bypass);
    bool isSlotBypassed(int slot) const;

    //==========================================================================
    // Global bypass (bypasses entire chain)
    void setBypass(bool bypass) { globalBypass = bypass; }
    bool isBypassed() const { return globalBypass; }

private:
    struct EffectSlot
    {
        std::unique_ptr<EffectBase> effect;
        bool bypassed = false;
    };

    std::array<EffectSlot, MAX_EFFECTS> slots;
    bool globalBypass = false;

    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectChain)
};
