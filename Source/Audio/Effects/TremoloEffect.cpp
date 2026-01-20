#include "TremoloEffect.h"
#include <cmath>

TremoloEffect::TremoloEffect()
{
    addParameter("rate", "Rate", 4.0f, 0.5f, 20.0f, "Hz");
    addParameter("depth", "Depth", 0.5f, 0.0f, 1.0f);
    addParameter("wave", "Wave", 0.0f, 0.0f, 3.0f, "", 1.0f);
    addParameter("spread", "Spread", 0.0f, 0.0f, 180.0f, "°");
}

void TremoloEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    rate = getParameter("rate");
    depth = getParameter("depth");
    waveType = static_cast<int>(getParameter("wave"));
    spreadDegrees = getParameter("spread");
}

void TremoloEffect::releaseResources()
{
    EffectBase::releaseResources();
}

void TremoloEffect::reset()
{
    EffectBase::reset();
    lfoPhase = 0.0f;
}

float TremoloEffect::getLfoSample(float phase, int wave) const
{
    // Normalize phase to 0-1
    float p = phase - std::floor(phase);

    switch (wave)
    {
        case 0: // Sine
            return std::sin(2.0f * juce::MathConstants<float>::pi * p);

        case 1: // Square
            return p < 0.5f ? 1.0f : -1.0f;

        case 2: // Triangle
            return 4.0f * std::abs(p - 0.5f) - 1.0f;

        case 3: // Sawtooth
            return 2.0f * p - 1.0f;

        default:
            return std::sin(2.0f * juce::MathConstants<float>::pi * p);
    }
}

void TremoloEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    const float lfoIncrement = rate / static_cast<float>(sampleRate);
    const float spreadPhase = spreadDegrees / 360.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Calculate LFO values for left and right channels
        float lfoL = getLfoSample(lfoPhase, waveType);
        float lfoR = getLfoSample(lfoPhase + spreadPhase, waveType);

        // Convert LFO (-1 to 1) to amplitude (1-depth to 1)
        float ampL = 1.0f - depth * (0.5f - 0.5f * lfoL);
        float ampR = 1.0f - depth * (0.5f - 0.5f * lfoR);

        // Apply tremolo
        leftChannel[i] *= ampL;

        if (rightChannel != nullptr)
            rightChannel[i] *= ampR;

        // Advance LFO phase
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }
}

void TremoloEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "rate")
        rate = value;
    else if (name == "depth")
        depth = value;
    else if (name == "wave")
        waveType = static_cast<int>(value);
    else if (name == "spread")
        spreadDegrees = value;
}

std::vector<EffectPreset> TremoloEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Subtle";
        p.values["rate"] = 3.0f;
        p.values["depth"] = 0.3f;
        p.values["wave"] = 0.0f;
        p.values["spread"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Classic";
        p.values["rate"] = 5.0f;
        p.values["depth"] = 0.5f;
        p.values["wave"] = 0.0f;
        p.values["spread"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Fast";
        p.values["rate"] = 10.0f;
        p.values["depth"] = 0.6f;
        p.values["wave"] = 2.0f;
        p.values["spread"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Slow Pulse";
        p.values["rate"] = 2.0f;
        p.values["depth"] = 0.7f;
        p.values["wave"] = 0.0f;
        p.values["spread"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Helicopter";
        p.values["rate"] = 15.0f;
        p.values["depth"] = 0.9f;
        p.values["wave"] = 1.0f;
        p.values["spread"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Stereo Wide";
        p.values["rate"] = 4.0f;
        p.values["depth"] = 0.5f;
        p.values["wave"] = 0.0f;
        p.values["spread"] = 90.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Auto-Pan";
        p.values["rate"] = 2.0f;
        p.values["depth"] = 0.8f;
        p.values["wave"] = 0.0f;
        p.values["spread"] = 180.0f;
        presets.push_back(p);
    }

    return presets;
}
