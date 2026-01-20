#pragma once

#include "EffectBase.h"

/**
 * EQEffect - 3-band parametric EQ
 *
 * Bands:
 * - Low shelf (80-500 Hz)
 * - Mid peak (200-8000 Hz, Q adjustable)
 * - High shelf (2000-16000 Hz)
 *
 * Each band has gain in dB (-12 to +12)
 */
class EQEffect : public EffectBase
{
public:
    EQEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    juce::String getName() const override { return "EQ"; }
    juce::String getCategory() const override { return "Filter"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    // Three filter bands per channel
    juce::dsp::IIR::Filter<float> lowShelfL, lowShelfR;
    juce::dsp::IIR::Filter<float> midPeakL, midPeakR;
    juce::dsp::IIR::Filter<float> highShelfL, highShelfR;

    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQEffect)
};
