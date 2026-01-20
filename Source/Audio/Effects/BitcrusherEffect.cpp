#include "BitcrusherEffect.h"
#include <cmath>

BitcrusherEffect::BitcrusherEffect()
{
    addParameter("bits", "Bit Depth", 8.0f, 1.0f, 16.0f, "bits", 1.0f);
    addParameter("sampleRateReduction", "Downsample", 1.0f, 1.0f, 32.0f, "x", 1.0f);
}

void BitcrusherEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    bits = getParameter("bits");
    sampleRateReduction = static_cast<int>(getParameter("sampleRateReduction"));

    holdL = 0.0f;
    holdR = 0.0f;
    sampleCounter = 0;
}

void BitcrusherEffect::releaseResources()
{
    EffectBase::releaseResources();
}

void BitcrusherEffect::reset()
{
    EffectBase::reset();
    holdL = 0.0f;
    holdR = 0.0f;
    sampleCounter = 0;
}

void BitcrusherEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    // Calculate quantization levels
    float maxLevels = std::pow(2.0f, bits);
    float quantScale = maxLevels / 2.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Sample rate reduction (sample-and-hold)
        if (sampleCounter == 0)
        {
            // Bit depth reduction: quantize the signal
            holdL = std::round(leftChannel[i] * quantScale) / quantScale;

            if (rightChannel != nullptr)
                holdR = std::round(rightChannel[i] * quantScale) / quantScale;
        }

        // Output the held sample
        leftChannel[i] = holdL;

        if (rightChannel != nullptr)
            rightChannel[i] = holdR;

        // Advance sample counter
        sampleCounter++;
        if (sampleCounter >= sampleRateReduction)
            sampleCounter = 0;
    }
}

void BitcrusherEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "bits")
        bits = value;
    else if (name == "sampleRateReduction")
        sampleRateReduction = static_cast<int>(value);
}

std::vector<EffectPreset> BitcrusherEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Subtle Lo-Fi";
        p.values["bits"] = 12.0f;
        p.values["sampleRateReduction"] = 1.0f;
        p.values["wet"] = 0.5f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "8-Bit";
        p.values["bits"] = 8.0f;
        p.values["sampleRateReduction"] = 1.0f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "4-Bit";
        p.values["bits"] = 4.0f;
        p.values["sampleRateReduction"] = 2.0f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Extreme";
        p.values["bits"] = 2.0f;
        p.values["sampleRateReduction"] = 4.0f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Telephone";
        p.values["bits"] = 6.0f;
        p.values["sampleRateReduction"] = 8.0f;
        p.values["wet"] = 0.8f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Radio";
        p.values["bits"] = 10.0f;
        p.values["sampleRateReduction"] = 4.0f;
        p.values["wet"] = 0.6f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "NES";
        p.values["bits"] = 4.0f;
        p.values["sampleRateReduction"] = 16.0f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    return presets;
}
