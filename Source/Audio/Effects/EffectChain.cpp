#include "EffectChain.h"

EffectChain::EffectChain()
{
}

void EffectChain::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    for (auto& slot : slots)
    {
        if (slot.effect)
            slot.effect->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void EffectChain::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (globalBypass)
        return;

    for (auto& slot : slots)
    {
        if (slot.effect && !slot.bypassed)
        {
            slot.effect->processBlock(buffer);
        }
    }
}

void EffectChain::releaseResources()
{
    for (auto& slot : slots)
    {
        if (slot.effect)
            slot.effect->releaseResources();
    }
}

//==============================================================================
// Effect management

int EffectChain::addEffect(std::unique_ptr<EffectBase> effect)
{
    for (int i = 0; i < MAX_EFFECTS; ++i)
    {
        if (!slots[i].effect)
        {
            effect->prepareToPlay(sampleRate, samplesPerBlock);
            slots[i].effect = std::move(effect);
            slots[i].bypassed = false;
            return i;
        }
    }
    return -1; // No free slot
}

void EffectChain::insertEffect(int slot, std::unique_ptr<EffectBase> effect)
{
    if (slot < 0 || slot >= MAX_EFFECTS)
        return;

    // Shift effects down
    for (int i = MAX_EFFECTS - 1; i > slot; --i)
    {
        slots[i] = std::move(slots[i - 1]);
    }

    effect->prepareToPlay(sampleRate, samplesPerBlock);
    slots[slot].effect = std::move(effect);
    slots[slot].bypassed = false;
}

std::unique_ptr<EffectBase> EffectChain::removeEffect(int slot)
{
    if (slot < 0 || slot >= MAX_EFFECTS || !slots[slot].effect)
        return nullptr;

    auto effect = std::move(slots[slot].effect);
    slots[slot].bypassed = false;

    // Shift effects up
    for (int i = slot; i < MAX_EFFECTS - 1; ++i)
    {
        slots[i] = std::move(slots[i + 1]);
    }
    slots[MAX_EFFECTS - 1].effect = nullptr;
    slots[MAX_EFFECTS - 1].bypassed = false;

    return effect;
}

std::unique_ptr<EffectBase> EffectChain::replaceEffect(int slot, std::unique_ptr<EffectBase> effect)
{
    if (slot < 0 || slot >= MAX_EFFECTS)
        return effect;

    auto oldEffect = std::move(slots[slot].effect);
    effect->prepareToPlay(sampleRate, samplesPerBlock);
    slots[slot].effect = std::move(effect);

    return oldEffect;
}

void EffectChain::swapEffects(int slot1, int slot2)
{
    if (slot1 < 0 || slot1 >= MAX_EFFECTS || slot2 < 0 || slot2 >= MAX_EFFECTS)
        return;

    std::swap(slots[slot1], slots[slot2]);
}

void EffectChain::moveEffect(int fromSlot, int toSlot)
{
    if (fromSlot < 0 || fromSlot >= MAX_EFFECTS ||
        toSlot < 0 || toSlot >= MAX_EFFECTS ||
        fromSlot == toSlot)
        return;

    auto effect = std::move(slots[fromSlot].effect);
    bool bypassed = slots[fromSlot].bypassed;
    slots[fromSlot].bypassed = false;

    // Remove from old position
    if (fromSlot < toSlot)
    {
        for (int i = fromSlot; i < toSlot; ++i)
            slots[i] = std::move(slots[i + 1]);
    }
    else
    {
        for (int i = fromSlot; i > toSlot; --i)
            slots[i] = std::move(slots[i - 1]);
    }

    slots[toSlot].effect = std::move(effect);
    slots[toSlot].bypassed = bypassed;
}

void EffectChain::clearAll()
{
    for (auto& slot : slots)
    {
        slot.effect = nullptr;
        slot.bypassed = false;
    }
}

//==============================================================================
// Effect access

EffectBase* EffectChain::getEffect(int slot)
{
    if (slot < 0 || slot >= MAX_EFFECTS)
        return nullptr;
    return slots[slot].effect.get();
}

const EffectBase* EffectChain::getEffect(int slot) const
{
    if (slot < 0 || slot >= MAX_EFFECTS)
        return nullptr;
    return slots[slot].effect.get();
}

int EffectChain::getNumEffects() const
{
    int count = 0;
    for (const auto& slot : slots)
    {
        if (slot.effect)
            ++count;
    }
    return count;
}

int EffectChain::getNumActiveSlots() const
{
    // Find last occupied slot
    for (int i = MAX_EFFECTS - 1; i >= 0; --i)
    {
        if (slots[i].effect)
            return i + 1;
    }
    return 0;
}

//==============================================================================
// Per-slot bypass

void EffectChain::setSlotBypass(int slot, bool bypass)
{
    if (slot >= 0 && slot < MAX_EFFECTS)
        slots[slot].bypassed = bypass;
}

bool EffectChain::isSlotBypassed(int slot) const
{
    if (slot < 0 || slot >= MAX_EFFECTS)
        return true;
    return slots[slot].bypassed;
}
