#pragma once

#include "EffectBase.h"

/**
 * TremoloEffect - Amplitude modulation effect
 *
 * Parameters:
 * - rate: LFO frequency in Hz (0.5-20)
 * - depth: Modulation depth (0-1)
 * - wave: Waveform type (0=sine, 1=square, 2=triangle, 3=sawtooth)
 * - spread: Stereo phase spread in degrees (0-180)
 */
class TremoloEffect : public EffectBase
{
public:
    TremoloEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Tremolo"; }
    juce::String getCategory() const override { return "Modulation"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    float getLfoSample(float phase, int waveType) const;

    float lfoPhase = 0.0f;
    float rate = 4.0f;
    float depth = 0.5f;
    int waveType = 0; // 0=sine, 1=square, 2=triangle, 3=sawtooth
    float spreadDegrees = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TremoloEffect)
};
