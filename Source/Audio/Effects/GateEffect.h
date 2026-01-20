#pragma once

#include "EffectBase.h"

/**
 * GateEffect - Noise gate to cut audio below threshold
 *
 * Parameters:
 * - threshold: Gate threshold in dB (-80 to 0)
 * - attack: Attack time in ms (0.1-100)
 * - release: Release time in ms (1-1000)
 * - ratio: Gate ratio (1-100, higher = harder gate)
 */
class GateEffect : public EffectBase
{
public:
    GateEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Gate"; }
    juce::String getCategory() const override { return "Dynamics"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    juce::dsp::NoiseGate<float> gate;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GateEffect)
};
