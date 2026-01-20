#pragma once

#include "EffectBase.h"

/**
 * FlangerEffect - Classic flanger using short modulated delay with feedback
 *
 * Parameters:
 * - rate: LFO frequency in Hz (0.05-5)
 * - depth: Modulation depth (0-1)
 * - delay: Center delay time in ms (1-10)
 * - feedback: Feedback amount (0-0.95)
 */
class FlangerEffect : public EffectBase
{
public:
    FlangerEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Flanger"; }
    juce::String getCategory() const override { return "Modulation"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    // Delay lines for each channel
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineL { 2048 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLineR { 2048 };

    // LFO phase
    float lfoPhase = 0.0f;
    float lfoRate = 0.5f;
    float depth = 0.5f;
    float feedbackAmount = 0.5f;
    float centerDelayMs = 3.0f;

    // Feedback storage
    float feedbackL = 0.0f;
    float feedbackR = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FlangerEffect)
};
