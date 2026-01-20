#include "ChorusEffect.h"

ChorusEffect::ChorusEffect()
{
    addParameter("rate", "Rate", 1.0f, 0.1f, 10.0f, "Hz");
    addParameter("depth", "Depth", 0.5f, 0.0f, 1.0f);
    addParameter("centreDelay", "Delay", 7.0f, 1.0f, 30.0f, "ms");
    addParameter("feedback", "Feedback", 0.0f, -1.0f, 1.0f);
}

void ChorusEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    chorus.prepare(spec);
    chorus.setRate(getParameter("rate"));
    chorus.setDepth(getParameter("depth"));
    chorus.setCentreDelay(getParameter("centreDelay"));
    chorus.setFeedback(getParameter("feedback"));
    chorus.setMix(1.0f); // We handle wet/dry in base class
}

void ChorusEffect::releaseResources()
{
    EffectBase::releaseResources();
    chorus.reset();
}

void ChorusEffect::reset()
{
    EffectBase::reset();
    chorus.reset();
}

void ChorusEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    chorus.process(context);
}

void ChorusEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "rate")
        chorus.setRate(value);
    else if (name == "depth")
        chorus.setDepth(value);
    else if (name == "centreDelay")
        chorus.setCentreDelay(value);
    else if (name == "feedback")
        chorus.setFeedback(value);
}

std::vector<EffectPreset> ChorusEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Subtle";
        p.values["rate"] = 0.5f;
        p.values["depth"] = 0.3f;
        p.values["centreDelay"] = 7.0f;
        p.values["wet"] = 0.4f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Classic";
        p.values["rate"] = 1.0f;
        p.values["depth"] = 0.5f;
        p.values["centreDelay"] = 10.0f;
        p.values["wet"] = 0.5f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Wide";
        p.values["rate"] = 0.3f;
        p.values["depth"] = 0.8f;
        p.values["centreDelay"] = 15.0f;
        p.values["wet"] = 0.6f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Vibrato";
        p.values["rate"] = 5.0f;
        p.values["depth"] = 0.6f;
        p.values["centreDelay"] = 3.0f;
        p.values["wet"] = 1.0f;
        presets.push_back(p);
    }

    return presets;
}
