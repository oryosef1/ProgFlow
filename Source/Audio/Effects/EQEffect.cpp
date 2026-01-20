#include "EQEffect.h"

EQEffect::EQEffect()
{
    // Low band
    addParameter("lowFreq", "Low Freq", 100.0f, 80.0f, 500.0f, "Hz");
    addParameter("lowGain", "Low Gain", 0.0f, -12.0f, 12.0f, "dB");

    // Mid band
    addParameter("midFreq", "Mid Freq", 1000.0f, 200.0f, 8000.0f, "Hz");
    addParameter("midGain", "Mid Gain", 0.0f, -12.0f, 12.0f, "dB");
    addParameter("midQ", "Mid Q", 1.0f, 0.1f, 10.0f);

    // High band
    addParameter("highFreq", "High Freq", 8000.0f, 2000.0f, 16000.0f, "Hz");
    addParameter("highGain", "High Gain", 0.0f, -12.0f, 12.0f, "dB");
}

void EQEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    lowShelfL.reset();
    lowShelfR.reset();
    midPeakL.reset();
    midPeakR.reset();
    highShelfL.reset();
    highShelfR.reset();

    updateFilters();
}

void EQEffect::reset()
{
    EffectBase::reset();
    lowShelfL.reset();
    lowShelfR.reset();
    midPeakL.reset();
    midPeakR.reset();
    highShelfL.reset();
    highShelfR.reset();
}

void EQEffect::updateFilters()
{
    float lowFreq = getParameter("lowFreq");
    float lowGain = getParameter("lowGain");

    float midFreq = getParameter("midFreq");
    float midGain = getParameter("midGain");
    float midQ = getParameter("midQ");

    float highFreq = getParameter("highFreq");
    float highGain = getParameter("highGain");

    // Low shelf
    auto lowCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, lowFreq, 0.707f, juce::Decibels::decibelsToGain(lowGain));
    lowShelfL.coefficients = lowCoeffs;
    lowShelfR.coefficients = lowCoeffs;

    // Mid peak
    auto midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        sampleRate, midFreq, midQ, juce::Decibels::decibelsToGain(midGain));
    midPeakL.coefficients = midCoeffs;
    midPeakR.coefficients = midCoeffs;

    // High shelf
    auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, highFreq, 0.707f, juce::Decibels::decibelsToGain(highGain));
    highShelfL.coefficients = highCoeffs;
    highShelfR.coefficients = highCoeffs;
}

void EQEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    if (numChannels >= 1)
    {
        float* dataL = buffer.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            dataL[i] = lowShelfL.processSample(dataL[i]);
            dataL[i] = midPeakL.processSample(dataL[i]);
            dataL[i] = highShelfL.processSample(dataL[i]);
        }
    }

    if (numChannels >= 2)
    {
        float* dataR = buffer.getWritePointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            dataR[i] = lowShelfR.processSample(dataR[i]);
            dataR[i] = midPeakR.processSample(dataR[i]);
            dataR[i] = highShelfR.processSample(dataR[i]);
        }
    }
}

void EQEffect::onParameterChanged(const juce::String& name, float value)
{
    updateFilters();
}

std::vector<EffectPreset> EQEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Flat";
        p.values["lowGain"] = 0.0f;
        p.values["midGain"] = 0.0f;
        p.values["highGain"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Bass Boost";
        p.values["lowFreq"] = 100.0f;
        p.values["lowGain"] = 6.0f;
        p.values["midGain"] = 0.0f;
        p.values["highGain"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Presence";
        p.values["lowGain"] = 0.0f;
        p.values["midFreq"] = 3000.0f;
        p.values["midGain"] = 4.0f;
        p.values["midQ"] = 2.0f;
        p.values["highGain"] = 2.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Scoop";
        p.values["lowGain"] = 4.0f;
        p.values["midFreq"] = 800.0f;
        p.values["midGain"] = -4.0f;
        p.values["midQ"] = 1.5f;
        p.values["highGain"] = 4.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Telephone";
        p.values["lowFreq"] = 300.0f;
        p.values["lowGain"] = -12.0f;
        p.values["midFreq"] = 1500.0f;
        p.values["midGain"] = 6.0f;
        p.values["midQ"] = 2.0f;
        p.values["highFreq"] = 3000.0f;
        p.values["highGain"] = -12.0f;
        presets.push_back(p);
    }

    return presets;
}
