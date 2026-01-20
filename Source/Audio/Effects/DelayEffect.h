#pragma once

#include "EffectBase.h"

/**
 * DelayEffect - Stereo delay with feedback and filtering
 *
 * Parameters:
 * - delayTime: Delay time in ms (1-2000)
 * - feedback: Feedback amount (0-0.95)
 * - wet: Wet/dry mix (0-1)
 */
class DelayEffect : public EffectBase
{
public:
    DelayEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Delay"; }
    juce::String getCategory() const override { return "Time"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    static constexpr float MAX_DELAY_SECONDS = 2.0f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineR;

    float currentDelayL = 0.0f;
    float currentDelayR = 0.0f;
    float feedback = 0.5f;
    bool pingPong = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayEffect)
};
