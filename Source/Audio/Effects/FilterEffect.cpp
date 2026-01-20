#include "FilterEffect.h"
#include <cmath>

FilterEffect::FilterEffect()
{
    addParameter("frequency", "Cutoff", 1000.0f, 20.0f, 20000.0f, "Hz");
    addParameter("resonance", "Resonance", 1.0f, 0.1f, 20.0f);
    addParameter("type", "Type", 0.0f, 0.0f, 3.0f, "", 1.0f);
    addParameter("lfoRate", "LFO Rate", 0.0f, 0.0f, 10.0f, "Hz");
    addParameter("lfoDepth", "LFO Depth", 0.0f, 0.0f, 5000.0f, "Hz");
}

void FilterEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    filter.reset();
    filter.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });

    cutoffFrequency = getParameter("frequency");
    resonance = getParameter("resonance");
    filterType = static_cast<int>(getParameter("type"));
    lfoRate = getParameter("lfoRate");
    lfoDepth = getParameter("lfoDepth");

    updateFilterType();
    filter.setCutoffFrequency(cutoffFrequency);
    filter.setResonance(resonance);
}

void FilterEffect::releaseResources()
{
    EffectBase::releaseResources();
    filter.reset();
}

void FilterEffect::reset()
{
    EffectBase::reset();
    filter.reset();
    lfoPhase = 0.0f;
}

void FilterEffect::updateFilterType()
{
    // Note: JUCE StateVariableTPTFilter only has lowpass, highpass, bandpass
    // For notch (type 3), we use bandpass with inverted mix (implemented in processEffect)
    switch (filterType)
    {
        case 0:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case 1:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case 2:
        case 3: // Notch uses bandpass internally
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
        default:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    }
}

void FilterEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const bool isNotch = (filterType == 3);

    // If LFO is active, process sample by sample
    if (lfoRate > 0.0f && lfoDepth > 0.0f)
    {
        const float lfoIncrement = lfoRate / static_cast<float>(sampleRate);

        for (int i = 0; i < numSamples; ++i)
        {
            // Calculate LFO modulation
            float lfoValue = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase);
            float modulatedFreq = cutoffFrequency + lfoValue * lfoDepth;
            modulatedFreq = juce::jlimit(20.0f, 20000.0f, modulatedFreq);

            filter.setCutoffFrequency(modulatedFreq);

            // Process each channel
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float* channelData = buffer.getWritePointer(ch);
                float input = channelData[i];
                float filtered = filter.processSample(ch, input);

                // For notch: output = input - bandpass (removes the band)
                channelData[i] = isNotch ? (input - filtered) : filtered;
            }

            lfoPhase += lfoIncrement;
            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;
        }
    }
    else
    {
        // No LFO - process block
        filter.setCutoffFrequency(cutoffFrequency);

        if (isNotch)
        {
            // For notch, we need to subtract the bandpass from the original
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            {
                float* channelData = buffer.getWritePointer(ch);
                for (int i = 0; i < numSamples; ++i)
                {
                    float input = channelData[i];
                    float filtered = filter.processSample(ch, input);
                    channelData[i] = input - filtered;
                }
            }
        }
        else
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            filter.process(context);
        }
    }
}

void FilterEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "frequency")
    {
        cutoffFrequency = value;
        filter.setCutoffFrequency(value);
    }
    else if (name == "resonance")
    {
        resonance = value;
        filter.setResonance(value);
    }
    else if (name == "type")
    {
        filterType = static_cast<int>(value);
        updateFilterType();
    }
    else if (name == "lfoRate")
        lfoRate = value;
    else if (name == "lfoDepth")
        lfoDepth = value;
}

std::vector<EffectPreset> FilterEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "Soft Low Pass";
        p.values["frequency"] = 2000.0f;
        p.values["resonance"] = 1.0f;
        p.values["type"] = 0.0f;
        p.values["lfoRate"] = 0.0f;
        p.values["lfoDepth"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Dark";
        p.values["frequency"] = 500.0f;
        p.values["resonance"] = 2.0f;
        p.values["type"] = 0.0f;
        p.values["lfoRate"] = 0.0f;
        p.values["lfoDepth"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Thin";
        p.values["frequency"] = 800.0f;
        p.values["resonance"] = 1.0f;
        p.values["type"] = 1.0f;
        p.values["lfoRate"] = 0.0f;
        p.values["lfoDepth"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Bright";
        p.values["frequency"] = 2000.0f;
        p.values["resonance"] = 1.5f;
        p.values["type"] = 1.0f;
        p.values["lfoRate"] = 0.0f;
        p.values["lfoDepth"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Vocal Band";
        p.values["frequency"] = 1500.0f;
        p.values["resonance"] = 5.0f;
        p.values["type"] = 2.0f;
        p.values["lfoRate"] = 0.0f;
        p.values["lfoDepth"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Notch";
        p.values["frequency"] = 1000.0f;
        p.values["resonance"] = 10.0f;
        p.values["type"] = 3.0f;
        p.values["lfoRate"] = 0.0f;
        p.values["lfoDepth"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Auto-Wah";
        p.values["frequency"] = 500.0f;
        p.values["resonance"] = 8.0f;
        p.values["type"] = 2.0f;
        p.values["lfoRate"] = 2.0f;
        p.values["lfoDepth"] = 2000.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Slow Sweep";
        p.values["frequency"] = 1000.0f;
        p.values["resonance"] = 4.0f;
        p.values["type"] = 0.0f;
        p.values["lfoRate"] = 0.2f;
        p.values["lfoDepth"] = 3000.0f;
        presets.push_back(p);
    }

    return presets;
}
