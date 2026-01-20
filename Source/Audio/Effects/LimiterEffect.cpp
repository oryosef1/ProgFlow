#include "LimiterEffect.h"

LimiterEffect::LimiterEffect()
{
    addParameter("threshold", "Ceiling", -3.0f, -30.0f, 0.0f, "dB", 0.5f);
    addParameter("release", "Release", 0.1f, 0.01f, 1.0f, "s");
    addParameter("outputGain", "Output", 0.0f, -12.0f, 12.0f, "dB", 0.5f);
}

void LimiterEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    limiter.prepare(spec);
    limiter.setThreshold(getParameter("threshold"));
    limiter.setRelease(getParameter("release") * 1000.0f); // Convert to ms

    outputGain.prepare(spec);
    outputGain.setGainDecibels(getParameter("outputGain"));
}

void LimiterEffect::releaseResources()
{
    EffectBase::releaseResources();
    limiter.reset();
    outputGain.reset();
}

void LimiterEffect::reset()
{
    EffectBase::reset();
    limiter.reset();
    outputGain.reset();
}

void LimiterEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    limiter.process(context);
    outputGain.process(context);
}

void LimiterEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "threshold")
        limiter.setThreshold(value);
    else if (name == "release")
        limiter.setRelease(value * 1000.0f); // Convert to ms
    else if (name == "outputGain")
        outputGain.setGainDecibels(value);
}

std::vector<EffectPreset> LimiterEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Soft Ceiling";
        p.values["threshold"] = -6.0f;
        p.values["release"] = 0.1f;
        p.values["outputGain"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Medium";
        p.values["threshold"] = -3.0f;
        p.values["release"] = 0.1f;
        p.values["outputGain"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Hard Limit";
        p.values["threshold"] = -1.0f;
        p.values["release"] = 0.05f;
        p.values["outputGain"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Mastering";
        p.values["threshold"] = -0.3f;
        p.values["release"] = 0.15f;
        p.values["outputGain"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Loudness";
        p.values["threshold"] = -1.0f;
        p.values["release"] = 0.1f;
        p.values["outputGain"] = 3.0f;
        presets.push_back(p);
    }

    return presets;
}
