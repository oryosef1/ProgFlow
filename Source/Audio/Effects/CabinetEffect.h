#pragma once

#include "EffectBase.h"

/**
 * CabinetEffect - Speaker cabinet simulation using EQ
 *
 * Parameters:
 * - cabinet: Cabinet model (0=1x12, 1=2x12, 2=4x12 Closed, 3=4x12 Open, 4=1x15 Bass, 5=8x10 Bass)
 * - lowCut: Low cut frequency in Hz (30-200)
 * - highCut: High cut frequency in Hz (2000-12000)
 * - resonance: Speaker resonance boost in dB (0-12)
 * - size: Perceived cabinet size (0-100%)
 */
class CabinetEffect : public EffectBase
{
public:
    CabinetEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    juce::String getName() const override { return "Cabinet"; }
    juce::String getCategory() const override { return "Amp"; }

    std::vector<EffectPreset> getPresets() const override;

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    void applyCabinetModel(int model);
    void updateFilters();

    // Filter chain
    juce::dsp::StateVariableTPTFilter<float> lowCutFilter;
    juce::dsp::StateVariableTPTFilter<float> highCutFilter;

    // EQ for cabinet character
    juce::dsp::IIR::Filter<float> lowMidL, lowMidR;
    juce::dsp::IIR::Filter<float> highMidL, highMidR;
    juce::dsp::IIR::Filter<float> resonanceL, resonanceR;

    // Parameters
    int cabinetModel = 2;
    float lowCutFreq = 70.0f;
    float highCutFreq = 5500.0f;
    float resonanceDb = 5.0f;
    float sizePercent = 50.0f;

    // EQ frequencies for cabinet character
    float lowMidFreq = 350.0f;
    float lowMidGain = 4.0f;
    float highMidFreq = 2200.0f;
    float highMidGain = -3.0f;
    float resonanceFreq = 90.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CabinetEffect)
};
