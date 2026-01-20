#include "PhaserEffect.h"

PhaserEffect::PhaserEffect()
{
    addParameter("rate", "Rate", 0.5f, 0.1f, 10.0f, "Hz");
    addParameter("depth", "Depth", 0.5f, 0.0f, 1.0f);
    addParameter("centreFrequency", "Center Freq", 350.0f, 100.0f, 2000.0f, "Hz");
    addParameter("feedback", "Feedback", 0.5f, -1.0f, 1.0f);
}

void PhaserEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    phaser.prepare(spec);
    phaser.setRate(getParameter("rate"));
    phaser.setDepth(getParameter("depth"));
    phaser.setCentreFrequency(getParameter("centreFrequency"));
    phaser.setFeedback(getParameter("feedback"));
    phaser.setMix(1.0f); // We handle wet/dry in base class
}

void PhaserEffect::releaseResources()
{
    EffectBase::releaseResources();
    phaser.reset();
}

void PhaserEffect::reset()
{
    EffectBase::reset();
    phaser.reset();
}

void PhaserEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    phaser.process(context);
}

void PhaserEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "rate")
        phaser.setRate(value);
    else if (name == "depth")
        phaser.setDepth(value);
    else if (name == "centreFrequency")
        phaser.setCentreFrequency(value);
    else if (name == "feedback")
        phaser.setFeedback(value);
}

std::vector<EffectPreset> PhaserEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Subtle";
        p.values["rate"] = 0.3f;
        p.values["depth"] = 0.3f;
        p.values["centreFrequency"] = 400.0f;
        p.values["feedback"] = 0.25f;
        p.values["wet"] = 0.3f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Classic";
        p.values["rate"] = 0.5f;
        p.values["depth"] = 0.5f;
        p.values["centreFrequency"] = 350.0f;
        p.values["feedback"] = 0.5f;
        p.values["wet"] = 0.5f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Deep Sweep";
        p.values["rate"] = 0.2f;
        p.values["depth"] = 0.8f;
        p.values["centreFrequency"] = 200.0f;
        p.values["feedback"] = 0.7f;
        p.values["wet"] = 0.6f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Fast";
        p.values["rate"] = 3.0f;
        p.values["depth"] = 0.4f;
        p.values["centreFrequency"] = 500.0f;
        p.values["feedback"] = 0.4f;
        p.values["wet"] = 0.4f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Jet";
        p.values["rate"] = 0.1f;
        p.values["depth"] = 1.0f;
        p.values["centreFrequency"] = 150.0f;
        p.values["feedback"] = 0.9f;
        p.values["wet"] = 0.7f;
        presets.push_back(p);
    }

    return presets;
}
