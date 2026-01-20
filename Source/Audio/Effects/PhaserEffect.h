#pragma once

#include "EffectBase.h"

/**
 * PhaserEffect - Classic phaser with all-pass filters modulated by LFO
 *
 * Parameters:
 * - rate: LFO frequency in Hz (0.1-10)
 * - depth: Modulation depth (0-1)
 * - centreFrequency: Center frequency for the sweep (100-2000 Hz)
 * - feedback: Resonance feedback (-1 to 1)
 */
class PhaserEffect : public EffectBase
{
public:
    PhaserEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Phaser"; }
    juce::String getCategory() const override { return "Modulation"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    juce::dsp::Phaser<float> phaser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserEffect)
};
