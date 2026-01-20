#pragma once

#include "EffectBase.h"

/**
 * LimiterEffect - Hard limiter to prevent clipping
 *
 * Parameters:
 * - threshold: Ceiling in dB (-30 to 0)
 * - release: Release time in seconds (0.01-1)
 * - outputGain: Output gain in dB (-12 to +12)
 */
class LimiterEffect : public EffectBase
{
public:
    LimiterEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Limiter"; }
    juce::String getCategory() const override { return "Dynamics"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Gain<float> outputGain;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterEffect)
};
