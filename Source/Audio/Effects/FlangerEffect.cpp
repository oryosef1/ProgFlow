#include "FlangerEffect.h"
#include <cmath>

FlangerEffect::FlangerEffect()
{
    addParameter("rate", "Rate", 0.5f, 0.05f, 5.0f, "Hz");
    addParameter("depth", "Depth", 0.5f, 0.0f, 1.0f);
    addParameter("delay", "Delay", 3.0f, 1.0f, 10.0f, "ms");
    addParameter("feedback", "Feedback", 0.5f, 0.0f, 0.95f);
}

void FlangerEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    delayLineL.prepare(spec);
    delayLineR.prepare(spec);

    lfoRate = getParameter("rate");
    depth = getParameter("depth");
    centerDelayMs = getParameter("delay");
    feedbackAmount = getParameter("feedback");
}

void FlangerEffect::releaseResources()
{
    EffectBase::releaseResources();
    delayLineL.reset();
    delayLineR.reset();
}

void FlangerEffect::reset()
{
    EffectBase::reset();
    delayLineL.reset();
    delayLineR.reset();
    lfoPhase = 0.0f;
    feedbackL = 0.0f;
    feedbackR = 0.0f;
}

void FlangerEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    const float maxDepthMs = 4.0f; // Maximum modulation depth in ms
    const float lfoIncrement = lfoRate / static_cast<float>(sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        // Calculate LFO value (0 to 1)
        float lfoValue = 0.5f + 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase);

        // Calculate modulated delay time in samples
        float modulatedDelayMs = centerDelayMs + (lfoValue - 0.5f) * depth * maxDepthMs * 2.0f;
        float delaySamples = modulatedDelayMs * 0.001f * static_cast<float>(sampleRate);
        delaySamples = juce::jlimit(1.0f, 2000.0f, delaySamples);

        // Process left channel
        float inputL = leftChannel[i] + feedbackL * feedbackAmount;
        delayLineL.pushSample(0, inputL);
        float delayedL = delayLineL.popSample(0, delaySamples);
        feedbackL = delayedL;
        leftChannel[i] = delayedL;

        // Process right channel (with slightly offset LFO phase for stereo width)
        if (rightChannel != nullptr)
        {
            float lfoValueR = 0.5f + 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * (lfoPhase + 0.25f));
            float modulatedDelayMsR = centerDelayMs + (lfoValueR - 0.5f) * depth * maxDepthMs * 2.0f;
            float delaySamplesR = modulatedDelayMsR * 0.001f * static_cast<float>(sampleRate);
            delaySamplesR = juce::jlimit(1.0f, 2000.0f, delaySamplesR);

            float inputR = rightChannel[i] + feedbackR * feedbackAmount;
            delayLineR.pushSample(0, inputR);
            float delayedR = delayLineR.popSample(0, delaySamplesR);
            feedbackR = delayedR;
            rightChannel[i] = delayedR;
        }

        // Advance LFO phase
        lfoPhase += lfoIncrement;
        if (lfoPhase >= 1.0f)
            lfoPhase -= 1.0f;
    }
}

void FlangerEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "rate")
        lfoRate = value;
    else if (name == "depth")
        depth = value;
    else if (name == "delay")
        centerDelayMs = value;
    else if (name == "feedback")
        feedbackAmount = value;
}

std::vector<EffectPreset> FlangerEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Subtle";
        p.values["rate"] = 0.3f;
        p.values["depth"] = 0.3f;
        p.values["delay"] = 3.0f;
        p.values["feedback"] = 0.3f;
        p.values["wet"] = 0.3f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Classic";
        p.values["rate"] = 0.5f;
        p.values["depth"] = 0.5f;
        p.values["delay"] = 4.0f;
        p.values["feedback"] = 0.5f;
        p.values["wet"] = 0.5f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Jet";
        p.values["rate"] = 0.2f;
        p.values["depth"] = 0.8f;
        p.values["delay"] = 5.0f;
        p.values["feedback"] = 0.8f;
        p.values["wet"] = 0.7f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Metallic";
        p.values["rate"] = 1.5f;
        p.values["depth"] = 0.6f;
        p.values["delay"] = 2.0f;
        p.values["feedback"] = 0.7f;
        p.values["wet"] = 0.5f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Slow Sweep";
        p.values["rate"] = 0.1f;
        p.values["depth"] = 0.7f;
        p.values["delay"] = 4.0f;
        p.values["feedback"] = 0.6f;
        p.values["wet"] = 0.4f;
        presets.push_back(p);
    }

    return presets;
}
