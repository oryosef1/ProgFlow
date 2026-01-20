#pragma once

#include "EffectBase.h"

/**
 * ChorusEffect - Classic chorus using modulated delay
 *
 * Parameters:
 * - rate: LFO rate in Hz (0.1-10)
 * - depth: Modulation depth (0-1)
 * - mix: Wet/dry mix (0-1)
 */
class ChorusEffect : public EffectBase
{
public:
    ChorusEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Chorus"; }
    juce::String getCategory() const override { return "Modulation"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    juce::dsp::Chorus<float> chorus;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusEffect)
};
