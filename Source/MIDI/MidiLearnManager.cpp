#include "MidiLearnManager.h"

MidiLearnManager::MidiLearnManager()
{
}

MidiLearnManager::~MidiLearnManager()
{
    if (midiInput)
        midiInput->stop();
}

MidiLearnManager& MidiLearnManager::getInstance()
{
    static MidiLearnManager instance;
    return instance;
}

//==============================================================================
// MIDI Learn Mode

void MidiLearnManager::startLearning(const juce::String& parameterId,
                                     std::function<void(int channel, int cc)> callback)
{
    learningActive = true;
    learningParameterId = parameterId;
    learningCallback = std::move(callback);
}

void MidiLearnManager::cancelLearning()
{
    if (learningActive && learningCallback)
        learningCallback(-1, -1);  // Signal cancellation

    learningActive = false;
    learningParameterId.clear();
    learningCallback = nullptr;
}

//==============================================================================
// Mapping Management

void MidiLearnManager::setMapping(const juce::String& parameterId, const MidiMapping& mapping)
{
    mappings[parameterId] = mapping;
    rebuildReverseLookup();
}

void MidiLearnManager::removeMapping(const juce::String& parameterId)
{
    mappings.erase(parameterId);
    rebuildReverseLookup();
}

const MidiLearnManager::MidiMapping* MidiLearnManager::getMapping(const juce::String& parameterId) const
{
    auto it = mappings.find(parameterId);
    return (it != mappings.end()) ? &it->second : nullptr;
}

bool MidiLearnManager::hasMapping(const juce::String& parameterId) const
{
    return mappings.find(parameterId) != mappings.end();
}

void MidiLearnManager::clearAllMappings()
{
    mappings.clear();
    ccToParameterLookup.clear();
}

//==============================================================================
// MIDI Input Handling

void MidiLearnManager::setMidiInputDevice(const juce::String& deviceName)
{
    // Stop current input if any
    if (midiInput)
    {
        midiInput->stop();
        midiInput.reset();
    }

    currentMidiInputDevice = deviceName;

    if (deviceName.isEmpty())
        return;

    // Find and open the device
    for (auto& device : juce::MidiInput::getAvailableDevices())
    {
        if (device.name == deviceName)
        {
            midiInput = juce::MidiInput::openDevice(device.identifier, this);
            if (midiInput)
                midiInput->start();
            return;
        }
    }
}

//==============================================================================
// Parameter Callbacks

void MidiLearnManager::registerParameterCallback(const juce::String& parameterId,
                                                 std::function<void(float value)> callback)
{
    parameterCallbacks[parameterId] = std::move(callback);
}

void MidiLearnManager::unregisterParameterCallback(const juce::String& parameterId)
{
    parameterCallbacks.erase(parameterId);
}

//==============================================================================
// Persistence

juce::var MidiLearnManager::toJson() const
{
    juce::DynamicObject::Ptr root = new juce::DynamicObject();

    for (const auto& [parameterId, mapping] : mappings)
    {
        juce::DynamicObject::Ptr mappingObj = new juce::DynamicObject();
        mappingObj->setProperty("channel", mapping.channel);
        mappingObj->setProperty("cc", mapping.ccNumber);
        mappingObj->setProperty("min", mapping.minValue);
        mappingObj->setProperty("max", mapping.maxValue);
        mappingObj->setProperty("enabled", mapping.enabled);

        root->setProperty(parameterId, juce::var(mappingObj.get()));
    }

    return juce::var(root.get());
}

void MidiLearnManager::fromJson(const juce::var& json)
{
    mappings.clear();

    if (auto* obj = json.getDynamicObject())
    {
        for (const auto& prop : obj->getProperties())
        {
            if (auto* mappingObj = prop.value.getDynamicObject())
            {
                MidiMapping mapping;
                mapping.channel = mappingObj->getProperty("channel");
                mapping.ccNumber = mappingObj->getProperty("cc");
                mapping.minValue = static_cast<float>(static_cast<double>(mappingObj->getProperty("min")));
                mapping.maxValue = static_cast<float>(static_cast<double>(mappingObj->getProperty("max")));
                mapping.enabled = mappingObj->getProperty("enabled");

                mappings[prop.name.toString()] = mapping;
            }
        }
    }

    rebuildReverseLookup();
}

//==============================================================================
// MidiInputCallback

void MidiLearnManager::handleIncomingMidiMessage(juce::MidiInput* /*source*/,
                                                 const juce::MidiMessage& message)
{
    if (!message.isController())
        return;

    int channel = message.getChannel();
    int cc = message.getControllerNumber();
    int value = message.getControllerValue();

    // If learning, capture this CC and complete learning
    if (learningActive)
    {
        MidiMapping newMapping;
        newMapping.channel = -1;  // Accept any channel
        newMapping.ccNumber = cc;
        newMapping.minValue = 0.0f;
        newMapping.maxValue = 1.0f;
        newMapping.enabled = true;

        setMapping(learningParameterId, newMapping);

        if (learningCallback)
            learningCallback(channel, cc);

        learningActive = false;
        learningParameterId.clear();
        learningCallback = nullptr;
        return;
    }

    // Look up if this CC is mapped to a parameter
    // First try channel-specific lookup
    int lookupKey = channel * 128 + cc;
    auto it = ccToParameterLookup.find(lookupKey);

    // If not found, try "any channel" lookup (-1 channel)
    if (it == ccToParameterLookup.end())
    {
        lookupKey = -1 * 128 + cc;  // -128 + cc for "any channel"
        it = ccToParameterLookup.find(lookupKey);
    }

    if (it != ccToParameterLookup.end())
    {
        const juce::String& parameterId = it->second;

        if (auto* mapping = getMapping(parameterId))
        {
            if (mapping->enabled)
            {
                float normalizedValue = ccToNormalizedValue(value, *mapping);

                // Call the parameter callback on the message thread
                auto callbackIt = parameterCallbacks.find(parameterId);
                if (callbackIt != parameterCallbacks.end())
                {
                    // Use MessageManager to call back on the main thread
                    juce::MessageManager::callAsync([callback = callbackIt->second, normalizedValue]()
                    {
                        callback(normalizedValue);
                    });
                }
            }
        }
    }
}

//==============================================================================
// Private helpers

void MidiLearnManager::rebuildReverseLookup()
{
    ccToParameterLookup.clear();

    for (const auto& [parameterId, mapping] : mappings)
    {
        if (!mapping.enabled)
            continue;

        int lookupKey = mapping.channel * 128 + mapping.ccNumber;
        ccToParameterLookup[lookupKey] = parameterId;
    }
}

float MidiLearnManager::ccToNormalizedValue(int ccValue, const MidiMapping& mapping) const
{
    // CC values are 0-127
    float normalized = ccValue / 127.0f;

    // Map to the configured range
    return mapping.minValue + normalized * (mapping.maxValue - mapping.minValue);
}
