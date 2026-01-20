#pragma once

#include "EffectBase.h"

/**
 * CompressorEffect - Dynamic range compressor
 *
 * Parameters:
 * - threshold: dB level where compression starts (-60 to 0)
 * - ratio: Compression ratio (1:1 to 20:1)
 * - attack: Attack time in ms (0.1-100)
 * - release: Release time in ms (10-1000)
 * - makeupGain: Output gain in dB (0-24)
 */
class CompressorEffect : public EffectBase
{
public:
    CompressorEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    juce::String getName() const override { return "Compressor"; }
    juce::String getCategory() const override { return "Dynamics"; }

    std::vector<EffectPreset> getPresets() const override;

    // For metering
    float getGainReduction() const { return gainReduction; }

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    juce::dsp::Compressor<float> compressor;

    float gainReduction = 0.0f; // For metering

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorEffect)
};
