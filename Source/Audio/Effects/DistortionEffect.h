#pragma once

#include "EffectBase.h"

/**
 * DistortionEffect - Multiple distortion types
 *
 * Types:
 * - Soft clip (smooth saturation)
 * - Hard clip (aggressive)
 * - Fuzz (asymmetric)
 *
 * Parameters:
 * - drive: Amount of distortion (0-1)
 * - type: Distortion type (0=soft, 1=hard, 2=fuzz)
 * - tone: High frequency rolloff (0-1)
 */
class DistortionEffect : public EffectBase
{
public:
    DistortionEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    juce::String getName() const override { return "Distortion"; }
    juce::String getCategory() const override { return "Dynamics"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    enum class DistortionType { Soft, Hard, Fuzz };

    float drive = 0.5f;
    DistortionType type = DistortionType::Soft;

    // Tone filter (simple one-pole lowpass)
    juce::dsp::IIR::Filter<float> toneFilter;
    float toneFreq = 8000.0f;

    float processSample(float input);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DistortionEffect)
};
