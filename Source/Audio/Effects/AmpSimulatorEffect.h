#pragma once

#include "EffectBase.h"

/**
 * AmpSimulatorEffect - Guitar/Bass amplifier simulation
 * Models tone-shaping and distortion characteristics of tube amplifiers
 *
 * Parameters:
 * - drive: Preamp gain (0-10)
 * - bass: Bass EQ (0-10)
 * - mid: Mid EQ (0-10)
 * - treble: Treble EQ (0-10)
 * - presence: Presence boost (0-10)
 * - master: Master volume (0-10)
 * - model: Amp model (0=Clean, 1=Crunch, 2=Lead, 3=HighGain)
 */
class AmpSimulatorEffect : public EffectBase
{
public:
    AmpSimulatorEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Amp Sim"; }
    juce::String getCategory() const override { return "Amp"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    void applyAmpModel(int model);
    float waveshape(float input, float amount);

    // Input gain (preamp/drive)
    juce::dsp::Gain<float> inputGain;

    // Low cut filter
    juce::dsp::StateVariableTPTFilter<float> lowCut;

    // Tone stack (3-band EQ)
    juce::dsp::IIR::Filter<float> toneStackLowL, toneStackLowR;
    juce::dsp::IIR::Filter<float> toneStackMidL, toneStackMidR;
    juce::dsp::IIR::Filter<float> toneStackHighL, toneStackHighR;

    // Presence
    juce::dsp::IIR::Filter<float> presenceL, presenceR;

    // Output gain
    juce::dsp::Gain<float> outputGain;

    // Parameters
    float drive = 5.0f;
    float bass = 5.0f;
    float mid = 5.0f;
    float treble = 5.0f;
    float presenceAmount = 5.0f;
    float master = 5.0f;
    int ampModel = 1;

    // Waveshaper amount
    float distortionAmount = 0.3f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpSimulatorEffect)
};
