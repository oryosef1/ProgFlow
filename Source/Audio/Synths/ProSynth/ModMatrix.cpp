#include "ModMatrix.h"

ModMatrix::ModMatrix()
{
    clearAllSlots();

    // Initialize all source values to 0
    for (int i = static_cast<int>(ModSource::None);
         i <= static_cast<int>(ModSource::Keytrack); ++i)
    {
        sourceValues[static_cast<ModSource>(i)] = 0.0f;
    }
}

const ModSlot& ModMatrix::getSlot(int index) const
{
    jassert(index >= 0 && index < NUM_SLOTS);
    return slots[index];
}

void ModMatrix::setSlot(int index, const ModSlot& slot)
{
    jassert(index >= 0 && index < NUM_SLOTS);
    slots[index] = slot;
    slots[index].amount = juce::jlimit(-1.0f, 1.0f, slot.amount);
}

void ModMatrix::clearSlot(int index)
{
    jassert(index >= 0 && index < NUM_SLOTS);
    slots[index] = ModSlot();
}

void ModMatrix::clearAllSlots()
{
    for (auto& slot : slots)
        slot = ModSlot();
}

void ModMatrix::setSourceValue(ModSource source, float value)
{
    sourceValues[source] = value;
}

float ModMatrix::getSourceValue(ModSource source) const
{
    auto it = sourceValues.find(source);
    if (it != sourceValues.end())
        return it->second;
    return 0.0f;
}

float ModMatrix::getModulationFor(ModDestination dest) const
{
    float totalModulation = 0.0f;

    for (const auto& slot : slots)
    {
        if (!slot.enabled || slot.destination != dest || slot.source == ModSource::None)
            continue;

        float sourceValue = getSourceValue(slot.source);
        totalModulation += sourceValue * slot.amount;
    }

    // Clamp to -1 to 1
    return juce::jlimit(-1.0f, 1.0f, totalModulation);
}

std::vector<int> ModMatrix::getActiveSlotsForDestination(ModDestination dest) const
{
    std::vector<int> result;

    for (int i = 0; i < NUM_SLOTS; ++i)
    {
        if (slots[i].enabled && slots[i].destination == dest)
            result.push_back(i);
    }

    return result;
}
