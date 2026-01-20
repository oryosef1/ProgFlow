#include "CompressorEffect.h"

CompressorEffect::CompressorEffect()
{
    addParameter("threshold", "Threshold", -20.0f, -60.0f, 0.0f, "dB");
    addParameter("ratio", "Ratio", 4.0f, 1.0f, 20.0f, ":1");
    addParameter("attack", "Attack", 10.0f, 0.1f, 100.0f, "ms");
    addParameter("release", "Release", 100.0f, 10.0f, 1000.0f, "ms");
    addParameter("makeupGain", "Makeup", 0.0f, 0.0f, 24.0f, "dB");
}

void CompressorEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    compressor.prepare(spec);
    compressor.setThreshold(getParameter("threshold"));
    compressor.setRatio(getParameter("ratio"));
    compressor.setAttack(getParameter("attack"));
    compressor.setRelease(getParameter("release"));
}

void CompressorEffect::reset()
{
    EffectBase::reset();
    compressor.reset();
    gainReduction = 0.0f;
}

void CompressorEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    compressor.process(context);

    // Apply makeup gain
    float makeupDb = getParameter("makeupGain");
    if (makeupDb != 0.0f)
    {
        float makeupLinear = juce::Decibels::decibelsToGain(makeupDb);
        buffer.applyGain(makeupLinear);
    }
}

void CompressorEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "threshold")
        compressor.setThreshold(value);
    else if (name == "ratio")
        compressor.setRatio(value);
    else if (name == "attack")
        compressor.setAttack(value);
    else if (name == "release")
        compressor.setRelease(value);
}

std::vector<EffectPreset> CompressorEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Gentle";
        p.values["threshold"] = -20.0f;
        p.values["ratio"] = 2.0f;
        p.values["attack"] = 20.0f;
        p.values["release"] = 200.0f;
        p.values["makeupGain"] = 3.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Vocal";
        p.values["threshold"] = -18.0f;
        p.values["ratio"] = 4.0f;
        p.values["attack"] = 5.0f;
        p.values["release"] = 100.0f;
        p.values["makeupGain"] = 6.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Drums";
        p.values["threshold"] = -15.0f;
        p.values["ratio"] = 6.0f;
        p.values["attack"] = 1.0f;
        p.values["release"] = 50.0f;
        p.values["makeupGain"] = 4.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Limiter";
        p.values["threshold"] = -6.0f;
        p.values["ratio"] = 20.0f;
        p.values["attack"] = 0.1f;
        p.values["release"] = 50.0f;
        p.values["makeupGain"] = 0.0f;
        presets.push_back(p);
    }

    return presets;
}
