#include "DelayEffect.h"

DelayEffect::DelayEffect()
    : delayLineL(static_cast<int>(MAX_DELAY_SECONDS * 48000)),
      delayLineR(static_cast<int>(MAX_DELAY_SECONDS * 48000))
{
    addParameter("delayTime", "Delay Time", 300.0f, 1.0f, 2000.0f, "ms");
    addParameter("feedback", "Feedback", 0.5f, 0.0f, 0.95f);
    addParameter("pingPong", "Ping Pong", 0.0f, 0.0f, 1.0f);

    feedback = 0.5f;
}

void DelayEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    delayLineL.prepare(spec);
    delayLineR.prepare(spec);

    // Set max delay
    int maxDelaySamples = static_cast<int>(MAX_DELAY_SECONDS * sampleRate);
    delayLineL.setMaximumDelayInSamples(maxDelaySamples);
    delayLineR.setMaximumDelayInSamples(maxDelaySamples);

    // Calculate initial delay in samples
    float delayMs = getParameter("delayTime");
    currentDelayL = (delayMs / 1000.0f) * static_cast<float>(sampleRate);
    currentDelayR = currentDelayL;
}

void DelayEffect::releaseResources()
{
    EffectBase::releaseResources();
    delayLineL.reset();
    delayLineR.reset();
}

void DelayEffect::reset()
{
    EffectBase::reset();
    delayLineL.reset();
    delayLineR.reset();
}

void DelayEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    float* leftChannel = buffer.getWritePointer(0);
    float* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : leftChannel;

    // Calculate target delay in samples
    float delayMs = getParameter("delayTime");
    float targetDelay = (delayMs / 1000.0f) * static_cast<float>(sampleRate);

    // Smooth delay time changes
    const float smoothing = 0.999f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Smooth delay time
        currentDelayL = currentDelayL * smoothing + targetDelay * (1.0f - smoothing);
        currentDelayR = currentDelayR * smoothing + targetDelay * (1.0f - smoothing);

        delayLineL.setDelay(currentDelayL);
        delayLineR.setDelay(currentDelayR);

        float inputL = leftChannel[i];
        float inputR = rightChannel[i];

        // Read from delay lines
        float delayedL = delayLineL.popSample(0);
        float delayedR = delayLineR.popSample(0);

        // Write to delay lines with feedback
        if (pingPong)
        {
            // Ping-pong: feed L to R and R to L
            delayLineL.pushSample(0, inputL + delayedR * feedback);
            delayLineR.pushSample(0, inputR + delayedL * feedback);
        }
        else
        {
            // Normal stereo delay
            delayLineL.pushSample(0, inputL + delayedL * feedback);
            delayLineR.pushSample(0, inputR + delayedR * feedback);
        }

        // Output
        leftChannel[i] = delayedL;
        rightChannel[i] = delayedR;
    }
}

void DelayEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "feedback")
        feedback = value;
    else if (name == "pingPong")
        pingPong = value > 0.5f;
}

std::vector<EffectPreset> DelayEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Slapback";
        p.values["delayTime"] = 80.0f;
        p.values["feedback"] = 0.2f;
        p.values["wet"] = 0.4f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Quarter Note";
        p.values["delayTime"] = 500.0f;
        p.values["feedback"] = 0.4f;
        p.values["wet"] = 0.35f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Ping Pong";
        p.values["delayTime"] = 375.0f;
        p.values["feedback"] = 0.5f;
        p.values["pingPong"] = 1.0f;
        p.values["wet"] = 0.4f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Ambient";
        p.values["delayTime"] = 750.0f;
        p.values["feedback"] = 0.6f;
        p.values["wet"] = 0.3f;
        presets.push_back(p);
    }

    return presets;
}
