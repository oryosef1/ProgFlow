#include "EffectBase.h"

EffectBase::EffectBase()
{
    // Add universal wet/dry parameter
    addParameter("wet", "Wet/Dry", 1.0f, 0.0f, 1.0f, "%");
}

void EffectBase::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    samplesPerBlock = newSamplesPerBlock;

    // Prepare dry buffer for wet/dry mixing
    dryBuffer.setSize(2, newSamplesPerBlock);
}

void EffectBase::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return; // Bypassed - leave buffer unchanged

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Handle fully wet or fully dry cases efficiently
    if (wetAmount >= 0.999f)
    {
        // Fully wet - just process
        processEffect(buffer);
        return;
    }

    if (wetAmount <= 0.001f)
    {
        // Fully dry - leave unchanged
        return;
    }

    // Mix wet and dry signals
    // 1. Store dry signal
    dryBuffer.makeCopyOf(buffer, true);

    // 2. Process wet signal in place
    processEffect(buffer);

    // 3. Mix: output = wet * processed + (1-wet) * dry
    const float dryAmount = 1.0f - wetAmount;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* wet = buffer.getWritePointer(ch);
        const auto* dry = dryBuffer.getReadPointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            wet[i] = wet[i] * wetAmount + dry[i] * dryAmount;
        }
    }
}

void EffectBase::releaseResources()
{
    dryBuffer.setSize(0, 0);
}

void EffectBase::reset()
{
    dryBuffer.clear();
}

//==============================================================================
// Wet/dry and bypass

void EffectBase::setWetDry(float wet)
{
    wetAmount = juce::jlimit(0.0f, 1.0f, wet);
    parameters["wet"].value = wetAmount;
}

void EffectBase::setBypass(bool shouldBypass)
{
    bypassed = shouldBypass;
}

//==============================================================================
// Parameter management

void EffectBase::addParameter(const juce::String& id, const juce::String& name,
                             float defaultValue, float minValue, float maxValue,
                             const juce::String& unit, float step)
{
    EffectParameter param;
    param.name = name;
    param.value = defaultValue;
    param.defaultValue = defaultValue;
    param.minValue = minValue;
    param.maxValue = maxValue;
    param.unit = unit;
    param.step = step;

    parameters[id] = param;
}

void EffectBase::setParameter(const juce::String& name, float value)
{
    auto it = parameters.find(name);
    if (it == parameters.end()) return;

    auto& param = it->second;
    value = juce::jlimit(param.minValue, param.maxValue, value);
    if (param.step > 0.0f)
        value = std::round(value / param.step) * param.step;

    param.value = value;

    // Special handling for wet/dry
    if (name == "wet")
    {
        wetAmount = value;
    }

    onParameterChanged(name, value);
}

float EffectBase::getParameter(const juce::String& name) const
{
    auto it = parameters.find(name);
    if (it == parameters.end()) return 0.0f;
    return it->second.value;
}

const EffectParameter* EffectBase::getParameterInfo(const juce::String& name) const
{
    auto it = parameters.find(name);
    if (it == parameters.end()) return nullptr;
    return &it->second;
}

std::vector<juce::String> EffectBase::getParameterNames() const
{
    std::vector<juce::String> names;
    names.reserve(parameters.size());
    for (const auto& pair : parameters)
        names.push_back(pair.first);
    return names;
}

//==============================================================================
// Presets

void EffectBase::loadPreset(const EffectPreset& preset)
{
    for (const auto& pair : preset.values)
    {
        setParameter(pair.first, pair.second);
    }
}

void EffectBase::loadPreset(int index)
{
    auto presets = getPresets();
    if (index >= 0 && index < static_cast<int>(presets.size()))
    {
        loadPreset(presets[index]);
    }
}
