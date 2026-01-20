#include "ReverbEffect.h"

ReverbEffect::ReverbEffect()
{
    addParameter("roomSize", "Room Size", 0.5f, 0.0f, 1.0f);
    addParameter("damping", "Damping", 0.5f, 0.0f, 1.0f);
    addParameter("width", "Width", 1.0f, 0.0f, 1.0f);
    addParameter("predelay", "Pre-delay", 0.0f, 0.0f, 100.0f, "ms");

    updateReverbParams();
}

void ReverbEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    reverb.prepare(spec);
    reverb.setParameters(reverbParams);
}

void ReverbEffect::releaseResources()
{
    EffectBase::releaseResources();
    reverb.reset();
}

void ReverbEffect::reset()
{
    EffectBase::reset();
    reverb.reset();
}

void ReverbEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}

void ReverbEffect::onParameterChanged(const juce::String& name, float value)
{
    updateReverbParams();
}

void ReverbEffect::updateReverbParams()
{
    reverbParams.roomSize = getParameter("roomSize");
    reverbParams.damping = getParameter("damping");
    reverbParams.width = getParameter("width");
    reverbParams.wetLevel = 1.0f; // We handle wet/dry in base class
    reverbParams.dryLevel = 0.0f;
    reverbParams.freezeMode = 0.0f;

    reverb.setParameters(reverbParams);
}

std::vector<EffectPreset> ReverbEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Small Room";
        p.values["roomSize"] = 0.3f;
        p.values["damping"] = 0.6f;
        p.values["width"] = 0.8f;
        p.values["wet"] = 0.3f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Large Hall";
        p.values["roomSize"] = 0.8f;
        p.values["damping"] = 0.3f;
        p.values["width"] = 1.0f;
        p.values["wet"] = 0.4f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Plate";
        p.values["roomSize"] = 0.5f;
        p.values["damping"] = 0.8f;
        p.values["width"] = 1.0f;
        p.values["wet"] = 0.35f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Cathedral";
        p.values["roomSize"] = 0.95f;
        p.values["damping"] = 0.2f;
        p.values["width"] = 1.0f;
        p.values["wet"] = 0.5f;
        presets.push_back(p);
    }

    return presets;
}
