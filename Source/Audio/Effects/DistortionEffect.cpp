#include "DistortionEffect.h"

DistortionEffect::DistortionEffect()
{
    addParameter("drive", "Drive", 0.5f, 0.0f, 1.0f);
    addParameter("type", "Type", 0.0f, 0.0f, 2.0f, "", 1.0f);
    addParameter("tone", "Tone", 0.7f, 0.0f, 1.0f);
    addParameter("output", "Output", 0.5f, 0.0f, 1.0f);
}

void DistortionEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    // Initialize tone filter
    toneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate, toneFreq);
    toneFilter.reset();
}

void DistortionEffect::reset()
{
    EffectBase::reset();
    toneFilter.reset();
}

float DistortionEffect::processSample(float input)
{
    // Apply drive (gain before distortion)
    float gained = input * (1.0f + drive * 10.0f);

    float output;

    switch (type)
    {
        case DistortionType::Soft:
        {
            // Soft clip using tanh
            output = std::tanh(gained);
            break;
        }

        case DistortionType::Hard:
        {
            // Hard clip
            output = juce::jlimit(-1.0f, 1.0f, gained);
            break;
        }

        case DistortionType::Fuzz:
        {
            // Asymmetric fuzz
            if (gained >= 0.0f)
                output = 1.0f - std::exp(-gained);
            else
                output = -1.0f + std::exp(gained);
            break;
        }

        default:
            output = gained;
    }

    return output;
}

void DistortionEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float outputGain = getParameter("output");

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float distorted = processSample(data[i]);

            // Apply tone filter
            float filtered = toneFilter.processSample(distorted);

            // Apply output gain
            data[i] = filtered * outputGain;
        }
    }
}

void DistortionEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "drive")
        drive = value;
    else if (name == "type")
        type = static_cast<DistortionType>(static_cast<int>(value));
    else if (name == "tone")
    {
        // Map 0-1 to 500-15000 Hz
        toneFreq = 500.0f + value * 14500.0f;
        toneFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            sampleRate, toneFreq);
    }
}

std::vector<EffectPreset> DistortionEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Warm Overdrive";
        p.values["drive"] = 0.3f;
        p.values["type"] = 0.0f;
        p.values["tone"] = 0.6f;
        p.values["output"] = 0.6f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Crunch";
        p.values["drive"] = 0.5f;
        p.values["type"] = 1.0f;
        p.values["tone"] = 0.5f;
        p.values["output"] = 0.5f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Fuzz Face";
        p.values["drive"] = 0.7f;
        p.values["type"] = 2.0f;
        p.values["tone"] = 0.4f;
        p.values["output"] = 0.4f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Extreme";
        p.values["drive"] = 0.9f;
        p.values["type"] = 1.0f;
        p.values["tone"] = 0.7f;
        p.values["output"] = 0.3f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    return presets;
}
