#pragma once

#include "EffectBase.h"

/**
 * ReverbEffect - Algorithmic reverb using JUCE's built-in reverb
 *
 * Parameters:
 * - roomSize: Room size (0-1)
 * - damping: High frequency damping (0-1)
 * - width: Stereo width (0-1)
 * - wet: Wet/dry mix (0-1)
 */
class ReverbEffect : public EffectBase
{
public:
    ReverbEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Reverb"; }
    juce::String getCategory() const override { return "Space"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    juce::dsp::Reverb reverb;
    juce::dsp::Reverb::Parameters reverbParams;

    void updateReverbParams();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbEffect)
};
