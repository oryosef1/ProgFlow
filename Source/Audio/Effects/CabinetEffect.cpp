#include "CabinetEffect.h"
#include <cmath>

CabinetEffect::CabinetEffect()
{
    addParameter("cabinet", "Cabinet", 2.0f, 0.0f, 5.0f, "", 1.0f);
    addParameter("lowCut", "Low Cut", 70.0f, 30.0f, 200.0f, "Hz");
    addParameter("highCut", "High Cut", 5500.0f, 2000.0f, 12000.0f, "Hz");
    addParameter("resonance", "Resonance", 5.0f, 0.0f, 12.0f, "dB");
    addParameter("size", "Size", 50.0f, 0.0f, 100.0f, "%");
}

void CabinetEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    lowCutFilter.prepare(spec);
    highCutFilter.prepare(spec);

    lowCutFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    highCutFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

    // Get initial parameters
    cabinetModel = static_cast<int>(getParameter("cabinet"));
    lowCutFreq = getParameter("lowCut");
    highCutFreq = getParameter("highCut");
    resonanceDb = getParameter("resonance");
    sizePercent = getParameter("size");

    applyCabinetModel(cabinetModel);
    updateFilters();
}

void CabinetEffect::releaseResources()
{
    EffectBase::releaseResources();
    lowCutFilter.reset();
    highCutFilter.reset();
}

void CabinetEffect::reset()
{
    EffectBase::reset();
    lowCutFilter.reset();
    highCutFilter.reset();
    lowMidL.reset();
    lowMidR.reset();
    highMidL.reset();
    highMidR.reset();
    resonanceL.reset();
    resonanceR.reset();
}

void CabinetEffect::updateFilters()
{
    lowCutFilter.setCutoffFrequency(lowCutFreq);
    highCutFilter.setCutoffFrequency(highCutFreq);

    // Low-mid bump
    float lowMidGainLinear = std::pow(10.0f, lowMidGain / 20.0f);
    auto lowMidCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, lowMidFreq, 0.8f, lowMidGainLinear);
    lowMidL.coefficients = lowMidCoeffs;
    lowMidR.coefficients = lowMidCoeffs;

    // High-mid cut/boost
    float highMidGainLinear = std::pow(10.0f, highMidGain / 20.0f);
    auto highMidCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, highMidFreq, 1.0f, highMidGainLinear);
    highMidL.coefficients = highMidCoeffs;
    highMidR.coefficients = highMidCoeffs;

    // Speaker resonance
    float resonanceGainLinear = std::pow(10.0f, resonanceDb / 20.0f);
    auto resonanceCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, resonanceFreq, 3.0f, resonanceGainLinear);
    resonanceL.coefficients = resonanceCoeffs;
    resonanceR.coefficients = resonanceCoeffs;
}

void CabinetEffect::applyCabinetModel(int model)
{
    switch (model)
    {
        case 0: // 1x12 Combo - small, focused
            lowCutFreq = 100.0f;
            highCutFreq = 5000.0f;
            lowMidFreq = 500.0f;
            lowMidGain = 2.0f;
            highMidFreq = 3000.0f;
            highMidGain = -1.0f;
            resonanceFreq = 120.0f;
            resonanceDb = 3.0f;
            break;

        case 1: // 2x12 Combo - balanced
            lowCutFreq = 80.0f;
            highCutFreq = 6000.0f;
            lowMidFreq = 400.0f;
            lowMidGain = 3.0f;
            highMidFreq = 2500.0f;
            highMidGain = -2.0f;
            resonanceFreq = 100.0f;
            resonanceDb = 4.0f;
            break;

        case 2: // 4x12 Closed - big, tight
            lowCutFreq = 70.0f;
            highCutFreq = 5500.0f;
            lowMidFreq = 350.0f;
            lowMidGain = 4.0f;
            highMidFreq = 2200.0f;
            highMidGain = -3.0f;
            resonanceFreq = 90.0f;
            resonanceDb = 5.0f;
            break;

        case 3: // 4x12 Open - big, loose
            lowCutFreq = 60.0f;
            highCutFreq = 6500.0f;
            lowMidFreq = 300.0f;
            lowMidGain = 3.0f;
            highMidFreq = 2800.0f;
            highMidGain = -1.0f;
            resonanceFreq = 80.0f;
            resonanceDb = 6.0f;
            break;

        case 4: // 1x15 Bass - deep, full
            lowCutFreq = 40.0f;
            highCutFreq = 4000.0f;
            lowMidFreq = 250.0f;
            lowMidGain = 4.0f;
            highMidFreq = 1500.0f;
            highMidGain = -4.0f;
            resonanceFreq = 60.0f;
            resonanceDb = 6.0f;
            break;

        case 5: // 8x10 Bass - massive, punchy
            lowCutFreq = 35.0f;
            highCutFreq = 4500.0f;
            lowMidFreq = 200.0f;
            lowMidGain = 5.0f;
            highMidFreq = 2000.0f;
            highMidGain = 2.0f;
            resonanceFreq = 50.0f;
            resonanceDb = 7.0f;
            break;
    }

    updateFilters();
}

void CabinetEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Apply low cut and high cut
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    lowCutFilter.process(context);
    highCutFilter.process(context);

    // Apply EQ filters per sample
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = lowMidL.processSample(leftChannel[i]);
        leftChannel[i] = highMidL.processSample(leftChannel[i]);
        leftChannel[i] = resonanceL.processSample(leftChannel[i]);

        if (rightChannel != nullptr)
        {
            rightChannel[i] = lowMidR.processSample(rightChannel[i]);
            rightChannel[i] = highMidR.processSample(rightChannel[i]);
            rightChannel[i] = resonanceR.processSample(rightChannel[i]);
        }
    }
}

void CabinetEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "cabinet")
    {
        cabinetModel = static_cast<int>(value);
        applyCabinetModel(cabinetModel);
    }
    else if (name == "lowCut")
    {
        lowCutFreq = value;
        lowCutFilter.setCutoffFrequency(value);
    }
    else if (name == "highCut")
    {
        highCutFreq = value;
        highCutFilter.setCutoffFrequency(value);
    }
    else if (name == "resonance")
    {
        resonanceDb = value;
        updateFilters();
    }
    else if (name == "size")
    {
        sizePercent = value;
        float sizeFactor = value / 100.0f;
        lowMidFreq = 300.0f + sizeFactor * 200.0f;
        resonanceFreq = 60.0f + sizeFactor * 80.0f;
        updateFilters();
    }
}

std::vector<EffectPreset> CabinetEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "1x12 Combo";
        p.values["cabinet"] = 0.0f;
        p.values["lowCut"] = 100.0f;
        p.values["highCut"] = 5000.0f;
        p.values["resonance"] = 3.0f;
        p.values["size"] = 30.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "2x12 Combo";
        p.values["cabinet"] = 1.0f;
        p.values["lowCut"] = 80.0f;
        p.values["highCut"] = 6000.0f;
        p.values["resonance"] = 4.0f;
        p.values["size"] = 50.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "4x12 Closed";
        p.values["cabinet"] = 2.0f;
        p.values["lowCut"] = 70.0f;
        p.values["highCut"] = 5500.0f;
        p.values["resonance"] = 5.0f;
        p.values["size"] = 70.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "4x12 Open";
        p.values["cabinet"] = 3.0f;
        p.values["lowCut"] = 60.0f;
        p.values["highCut"] = 6500.0f;
        p.values["resonance"] = 6.0f;
        p.values["size"] = 80.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "1x15 Bass";
        p.values["cabinet"] = 4.0f;
        p.values["lowCut"] = 40.0f;
        p.values["highCut"] = 4000.0f;
        p.values["resonance"] = 6.0f;
        p.values["size"] = 60.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "8x10 Bass";
        p.values["cabinet"] = 5.0f;
        p.values["lowCut"] = 35.0f;
        p.values["highCut"] = 4500.0f;
        p.values["resonance"] = 7.0f;
        p.values["size"] = 100.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Bright";
        p.values["cabinet"] = 1.0f;
        p.values["lowCut"] = 100.0f;
        p.values["highCut"] = 8000.0f;
        p.values["resonance"] = 2.0f;
        p.values["size"] = 40.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Dark";
        p.values["cabinet"] = 2.0f;
        p.values["lowCut"] = 60.0f;
        p.values["highCut"] = 3500.0f;
        p.values["resonance"] = 6.0f;
        p.values["size"] = 80.0f;
        presets.push_back(p);
    }

    return presets;
}
