#pragma once

#include "EffectBase.h"

/**
 * FilterEffect - Multi-mode filter with LFO modulation
 *
 * Parameters:
 * - frequency: Cutoff frequency in Hz (20-20000)
 * - resonance: Q/Resonance (0.1-20)
 * - type: Filter type (0=lowpass, 1=highpass, 2=bandpass, 3=notch)
 * - lfoRate: LFO rate in Hz (0-10)
 * - lfoDepth: LFO modulation depth in Hz (0-5000)
 */
class FilterEffect : public EffectBase
{
public:
    FilterEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Filter"; }
    juce::String getCategory() const override { return "Filter"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    void updateFilterType();

    juce::dsp::StateVariableTPTFilter<float> filter;

    float cutoffFrequency = 1000.0f;
    float resonance = 1.0f;
    int filterType = 0;
    float lfoRate = 0.0f;
    float lfoDepth = 0.0f;
    float lfoPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilterEffect)
};
