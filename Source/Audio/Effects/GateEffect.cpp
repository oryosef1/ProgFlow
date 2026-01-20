#include "GateEffect.h"

GateEffect::GateEffect()
{
    addParameter("threshold", "Threshold", -40.0f, -80.0f, 0.0f, "dB", 0.5f);
    addParameter("attack", "Attack", 1.0f, 0.1f, 100.0f, "ms");
    addParameter("release", "Release", 100.0f, 1.0f, 1000.0f, "ms");
    addParameter("ratio", "Ratio", 10.0f, 1.0f, 100.0f);
}

void GateEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    gate.prepare(spec);
    gate.setThreshold(getParameter("threshold"));
    gate.setAttack(getParameter("attack"));
    gate.setRelease(getParameter("release"));
    gate.setRatio(getParameter("ratio"));
}

void GateEffect::releaseResources()
{
    EffectBase::releaseResources();
    gate.reset();
}

void GateEffect::reset()
{
    EffectBase::reset();
    gate.reset();
}

void GateEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    gate.process(context);
}

void GateEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "threshold")
        gate.setThreshold(value);
    else if (name == "attack")
        gate.setAttack(value);
    else if (name == "release")
        gate.setRelease(value);
    else if (name == "ratio")
        gate.setRatio(value);
}

std::vector<EffectPreset> GateEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Gentle";
        p.values["threshold"] = -50.0f;
        p.values["attack"] = 5.0f;
        p.values["release"] = 200.0f;
        p.values["ratio"] = 5.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Medium";
        p.values["threshold"] = -40.0f;
        p.values["attack"] = 2.0f;
        p.values["release"] = 100.0f;
        p.values["ratio"] = 10.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Tight";
        p.values["threshold"] = -30.0f;
        p.values["attack"] = 0.5f;
        p.values["release"] = 50.0f;
        p.values["ratio"] = 20.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Drums";
        p.values["threshold"] = -35.0f;
        p.values["attack"] = 0.2f;
        p.values["release"] = 80.0f;
        p.values["ratio"] = 50.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Vocal";
        p.values["threshold"] = -45.0f;
        p.values["attack"] = 5.0f;
        p.values["release"] = 150.0f;
        p.values["ratio"] = 8.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Extreme";
        p.values["threshold"] = -20.0f;
        p.values["attack"] = 0.1f;
        p.values["release"] = 20.0f;
        p.values["ratio"] = 100.0f;
        presets.push_back(p);
    }

    return presets;
}
