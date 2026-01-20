#pragma once

#include "EffectBase.h"

/**
 * BitcrusherEffect - Lo-fi distortion by reducing bit depth
 *
 * Parameters:
 * - bits: Bit depth (1-16)
 * - sampleRateReduction: Sample rate reduction factor (1-32)
 */
class BitcrusherEffect : public EffectBase
{
public:
    BitcrusherEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Bitcrusher"; }
    juce::String getCategory() const override { return "Distortion"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    float bits = 8.0f;
    int sampleRateReduction = 1;

    // Sample-and-hold state for downsampling
    float holdL = 0.0f;
    float holdR = 0.0f;
    int sampleCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BitcrusherEffect)
};
