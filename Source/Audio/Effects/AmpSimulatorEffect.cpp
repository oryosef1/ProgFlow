#include "AmpSimulatorEffect.h"
#include <cmath>

AmpSimulatorEffect::AmpSimulatorEffect()
{
    addParameter("drive", "Drive", 5.0f, 0.0f, 10.0f);
    addParameter("bass", "Bass", 5.0f, 0.0f, 10.0f);
    addParameter("mid", "Mid", 5.0f, 0.0f, 10.0f);
    addParameter("treble", "Treble", 5.0f, 0.0f, 10.0f);
    addParameter("presence", "Presence", 5.0f, 0.0f, 10.0f);
    addParameter("master", "Master", 5.0f, 0.0f, 10.0f);
    addParameter("model", "Model", 1.0f, 0.0f, 3.0f, "", 1.0f);
}

void AmpSimulatorEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    inputGain.prepare(spec);
    outputGain.prepare(spec);
    lowCut.prepare(spec);

    // Initialize filters with default coefficients
    auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 250.0f, 0.707f, 1.0f);
    toneStackLowL.coefficients = lowShelfCoeffs;
    toneStackLowR.coefficients = lowShelfCoeffs;

    auto midPeakCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 800.0f, 1.0f, 1.0f);
    toneStackMidL.coefficients = midPeakCoeffs;
    toneStackMidR.coefficients = midPeakCoeffs;

    auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 3000.0f, 0.707f, 1.0f);
    toneStackHighL.coefficients = highShelfCoeffs;
    toneStackHighR.coefficients = highShelfCoeffs;

    auto presenceCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 5000.0f, 0.707f, 1.0f);
    presenceL.coefficients = presenceCoeffs;
    presenceR.coefficients = presenceCoeffs;

    lowCut.setType(juce::dsp::StateVariableTPTFilterType::highpass);
    lowCut.setCutoffFrequency(80.0f);

    // Get initial parameter values
    drive = getParameter("drive");
    bass = getParameter("bass");
    mid = getParameter("mid");
    treble = getParameter("treble");
    presenceAmount = getParameter("presence");
    master = getParameter("master");
    ampModel = static_cast<int>(getParameter("model"));

    applyAmpModel(ampModel);
}

void AmpSimulatorEffect::releaseResources()
{
    EffectBase::releaseResources();
    inputGain.reset();
    outputGain.reset();
    lowCut.reset();
}

void AmpSimulatorEffect::reset()
{
    EffectBase::reset();
    inputGain.reset();
    outputGain.reset();
    lowCut.reset();
    toneStackLowL.reset();
    toneStackLowR.reset();
    toneStackMidL.reset();
    toneStackMidR.reset();
    toneStackHighL.reset();
    toneStackHighR.reset();
    presenceL.reset();
    presenceR.reset();
}

float AmpSimulatorEffect::waveshape(float input, float amount)
{
    // Soft clipping waveshaper with adjustable amount
    if (amount < 0.01f)
        return input;

    float k = 2.0f * amount / (1.0f - amount);
    return (1.0f + k) * input / (1.0f + k * std::abs(input));
}

void AmpSimulatorEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Apply input gain
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Calculate drive gain
    float driveGain = 0.5f + (drive / 10.0f) * 3.0f;
    inputGain.setGainLinear(driveGain);
    inputGain.process(context);

    // Apply low cut
    lowCut.process(context);

    // Apply waveshaping (distortion)
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
        {
            channelData[i] = waveshape(channelData[i], distortionAmount);
        }
    }

    // Apply tone stack
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    for (int i = 0; i < numSamples; ++i)
    {
        leftChannel[i] = toneStackLowL.processSample(leftChannel[i]);
        leftChannel[i] = toneStackMidL.processSample(leftChannel[i]);
        leftChannel[i] = toneStackHighL.processSample(leftChannel[i]);
        leftChannel[i] = presenceL.processSample(leftChannel[i]);

        if (rightChannel != nullptr)
        {
            rightChannel[i] = toneStackLowR.processSample(rightChannel[i]);
            rightChannel[i] = toneStackMidR.processSample(rightChannel[i]);
            rightChannel[i] = toneStackHighR.processSample(rightChannel[i]);
            rightChannel[i] = presenceR.processSample(rightChannel[i]);
        }
    }

    // Apply output gain
    float masterGain = (master / 10.0f) * 1.5f;
    outputGain.setGainLinear(masterGain);
    outputGain.process(context);
}

void AmpSimulatorEffect::applyAmpModel(int model)
{
    switch (model)
    {
        case 0: // Clean
            lowCut.setCutoffFrequency(80.0f);
            distortionAmount = 0.1f;
            break;
        case 1: // Crunch
            lowCut.setCutoffFrequency(100.0f);
            distortionAmount = 0.3f;
            break;
        case 2: // Lead
            lowCut.setCutoffFrequency(120.0f);
            distortionAmount = 0.5f;
            break;
        case 3: // High Gain
            lowCut.setCutoffFrequency(150.0f);
            distortionAmount = 0.7f;
            break;
    }
}

void AmpSimulatorEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "drive")
    {
        drive = value;
        distortionAmount = (value / 10.0f) * 0.8f;
    }
    else if (name == "bass")
    {
        bass = value;
        float bassGainDb = (value - 5.0f) * 3.0f;
        float bassGain = std::pow(10.0f, bassGainDb / 20.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, 250.0f, 0.707f, bassGain);
        toneStackLowL.coefficients = coeffs;
        toneStackLowR.coefficients = coeffs;
    }
    else if (name == "mid")
    {
        mid = value;
        float midGainDb = (value - 5.0f) * 3.0f;
        float midGain = std::pow(10.0f, midGainDb / 20.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, 800.0f, 1.0f, midGain);
        toneStackMidL.coefficients = coeffs;
        toneStackMidR.coefficients = coeffs;
    }
    else if (name == "treble")
    {
        treble = value;
        float trebleGainDb = (value - 5.0f) * 3.0f;
        float trebleGain = std::pow(10.0f, trebleGainDb / 20.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 3000.0f, 0.707f, trebleGain);
        toneStackHighL.coefficients = coeffs;
        toneStackHighR.coefficients = coeffs;
    }
    else if (name == "presence")
    {
        presenceAmount = value;
        float presenceGainDb = (value - 5.0f) * 2.0f;
        float presenceGain = std::pow(10.0f, presenceGainDb / 20.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, 5000.0f, 0.707f, presenceGain);
        presenceL.coefficients = coeffs;
        presenceR.coefficients = coeffs;
    }
    else if (name == "master")
        master = value;
    else if (name == "model")
    {
        ampModel = static_cast<int>(value);
        applyAmpModel(ampModel);
    }
}

std::vector<EffectPreset> AmpSimulatorEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Clean";
        p.values["drive"] = 2.0f;
        p.values["bass"] = 5.0f;
        p.values["mid"] = 5.0f;
        p.values["treble"] = 6.0f;
        p.values["presence"] = 5.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Warm Clean";
        p.values["drive"] = 3.0f;
        p.values["bass"] = 6.0f;
        p.values["mid"] = 4.0f;
        p.values["treble"] = 4.0f;
        p.values["presence"] = 4.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Crunch";
        p.values["drive"] = 5.0f;
        p.values["bass"] = 5.0f;
        p.values["mid"] = 6.0f;
        p.values["treble"] = 5.0f;
        p.values["presence"] = 6.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "British Stack";
        p.values["drive"] = 6.0f;
        p.values["bass"] = 6.0f;
        p.values["mid"] = 7.0f;
        p.values["treble"] = 6.0f;
        p.values["presence"] = 7.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 1.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Lead";
        p.values["drive"] = 7.0f;
        p.values["bass"] = 5.0f;
        p.values["mid"] = 7.0f;
        p.values["treble"] = 6.0f;
        p.values["presence"] = 7.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 2.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Metal";
        p.values["drive"] = 9.0f;
        p.values["bass"] = 7.0f;
        p.values["mid"] = 3.0f;
        p.values["treble"] = 7.0f;
        p.values["presence"] = 8.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 3.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "High Gain";
        p.values["drive"] = 8.0f;
        p.values["bass"] = 6.0f;
        p.values["mid"] = 5.0f;
        p.values["treble"] = 6.0f;
        p.values["presence"] = 7.0f;
        p.values["master"] = 5.0f;
        p.values["model"] = 3.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Bass Amp";
        p.values["drive"] = 4.0f;
        p.values["bass"] = 8.0f;
        p.values["mid"] = 4.0f;
        p.values["treble"] = 3.0f;
        p.values["presence"] = 3.0f;
        p.values["master"] = 6.0f;
        p.values["model"] = 1.0f;
        presets.push_back(p);
    }

    return presets;
}
