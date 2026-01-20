#include "SidechainCompressorEffect.h"
#include <cmath>

SidechainCompressorEffect::SidechainCompressorEffect()
{
    addParameter("threshold", "Threshold", -20.0f, -60.0f, 0.0f, "dB");
    addParameter("ratio", "Ratio", 4.0f, 1.0f, 20.0f, ":1");
    addParameter("attack", "Attack", 10.0f, 0.1f, 100.0f, "ms");
    addParameter("release", "Release", 100.0f, 10.0f, 1000.0f, "ms");
    addParameter("makeupGain", "Makeup", 0.0f, 0.0f, 24.0f, "dB");
    addParameter("listen", "Listen SC", 0.0f, 0.0f, 1.0f, "", 1.0f);

    thresholdDb = -20.0f;
    ratio = 4.0f;
    attackMs = 10.0f;
    releaseMs = 100.0f;
}

void SidechainCompressorEffect::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    EffectBase::prepareToPlay(newSampleRate, newSamplesPerBlock);

    // Allocate sidechain buffer
    sidechainBuffer.setSize(2, samplesPerBlock);
    sidechainBuffer.clear();

    // Reset envelope
    envelopeDb = -100.0f;

    // Calculate time constants
    updateCoefficients();
}

void SidechainCompressorEffect::reset()
{
    EffectBase::reset();
    envelopeDb = -100.0f;
    sidechainInputProvided = false;
    sidechainBuffer.clear();
    gainReduction = 0.0f;
    sidechainLevel = 0.0f;
}

void SidechainCompressorEffect::updateCoefficients()
{
    if (sampleRate <= 0.0)
        return;

    // Convert ms to samples, then to coefficient (exponential decay)
    float attackSamples = static_cast<float>(attackMs * 0.001 * sampleRate);
    float releaseSamples = static_cast<float>(releaseMs * 0.001 * sampleRate);

    // Time constant: coeff = exp(-1/samples) for reaching ~63% in that time
    attackCoeff = std::exp(-1.0f / std::max(1.0f, attackSamples));
    releaseCoeff = std::exp(-1.0f / std::max(1.0f, releaseSamples));
}

float SidechainCompressorEffect::computeGain(float inputDb) const
{
    // Below threshold: no compression
    if (inputDb <= thresholdDb)
        return 0.0f;

    // Above threshold: apply ratio
    // gainReduction = (inputDb - threshold) * (1 - 1/ratio)
    float overDb = inputDb - thresholdDb;
    float gainReductionDb = overDb * (1.0f - 1.0f / ratio);
    return -gainReductionDb;  // Negative because it's reduction
}

void SidechainCompressorEffect::setSidechainSource(int trackIndex)
{
    sidechainSourceTrack = trackIndex;
}

void SidechainCompressorEffect::setSidechainInput(const juce::AudioBuffer<float>& buffer)
{
    int numChannels = std::min(buffer.getNumChannels(), sidechainBuffer.getNumChannels());
    int numSamples = std::min(buffer.getNumSamples(), sidechainBuffer.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        sidechainBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // If input is mono but we have stereo, duplicate
    if (numChannels == 1 && sidechainBuffer.getNumChannels() > 1)
    {
        sidechainBuffer.copyFrom(1, 0, sidechainBuffer, 0, 0, numSamples);
    }

    sidechainInputProvided = true;
}

void SidechainCompressorEffect::processEffect(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // Determine which buffer to use for detection
    // If sidechain provided, use it; otherwise use input signal (internal sidechain)
    const juce::AudioBuffer<float>& keyBuffer =
        (sidechainSourceTrack >= 0 && sidechainInputProvided) ? sidechainBuffer : buffer;

    // Check if we should listen to sidechain (for debugging/setup)
    listenToSidechain = getParameter("listen") > 0.5f;

    float peakLevel = 0.0f;
    float avgGainReduction = 0.0f;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get key signal level (peak of all channels)
        float keyLevel = 0.0f;
        for (int ch = 0; ch < std::min(numChannels, keyBuffer.getNumChannels()); ++ch)
        {
            keyLevel = std::max(keyLevel, std::abs(keyBuffer.getSample(ch, sample)));
        }
        peakLevel = std::max(peakLevel, keyLevel);

        // Convert to dB
        float keyDb = (keyLevel > 1e-6f) ?
            juce::Decibels::gainToDecibels(keyLevel) : -100.0f;

        // Envelope follower with different attack/release
        if (keyDb > envelopeDb)
        {
            // Attack (fast response when signal increases)
            envelopeDb = attackCoeff * envelopeDb + (1.0f - attackCoeff) * keyDb;
        }
        else
        {
            // Release (slow decay when signal decreases)
            envelopeDb = releaseCoeff * envelopeDb + (1.0f - releaseCoeff) * keyDb;
        }

        // Compute gain reduction
        float gainDb = computeGain(envelopeDb);
        avgGainReduction += -gainDb;

        // Convert to linear gain
        float gain = juce::Decibels::decibelsToGain(gainDb + makeupGainDb);

        // Apply gain to all channels
        if (listenToSidechain && sidechainSourceTrack >= 0 && sidechainInputProvided)
        {
            // Listen mode: output the sidechain signal instead
            for (int ch = 0; ch < numChannels; ++ch)
            {
                if (ch < keyBuffer.getNumChannels())
                    buffer.setSample(ch, sample, keyBuffer.getSample(ch, sample));
            }
        }
        else
        {
            // Normal mode: apply compression
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float in = buffer.getSample(ch, sample);
                buffer.setSample(ch, sample, in * gain);
            }
        }
    }

    // Update meters (thread-safe)
    gainReduction.store(numSamples > 0 ? avgGainReduction / static_cast<float>(numSamples) : 0.0f);
    sidechainLevel.store(peakLevel);

    // Reset sidechain flag for next block
    sidechainInputProvided = false;
}

void SidechainCompressorEffect::onParameterChanged(const juce::String& name, float value)
{
    if (name == "threshold")
    {
        thresholdDb = value;
    }
    else if (name == "ratio")
    {
        ratio = value;
    }
    else if (name == "attack")
    {
        attackMs = value;
        updateCoefficients();
    }
    else if (name == "release")
    {
        releaseMs = value;
        updateCoefficients();
    }
    else if (name == "makeupGain")
    {
        makeupGainDb = value;
    }
    else if (name == "listen")
    {
        listenToSidechain = value > 0.5f;
    }
}

std::vector<EffectPreset> SidechainCompressorEffect::getPresets() const
{
    std::vector<EffectPreset> presets;

    {
        EffectPreset p;
        p.name = "EDM Pump";
        p.values["threshold"] = -25.0f;
        p.values["ratio"] = 8.0f;
        p.values["attack"] = 0.5f;
        p.values["release"] = 150.0f;
        p.values["makeupGain"] = 0.0f;
        p.values["listen"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Subtle Duck";
        p.values["threshold"] = -20.0f;
        p.values["ratio"] = 3.0f;
        p.values["attack"] = 5.0f;
        p.values["release"] = 200.0f;
        p.values["makeupGain"] = 0.0f;
        p.values["listen"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Bass Duck";
        p.values["threshold"] = -18.0f;
        p.values["ratio"] = 6.0f;
        p.values["attack"] = 1.0f;
        p.values["release"] = 100.0f;
        p.values["makeupGain"] = 2.0f;
        p.values["listen"] = 0.0f;
        presets.push_back(p);
    }

    {
        EffectPreset p;
        p.name = "Aggressive";
        p.values["threshold"] = -30.0f;
        p.values["ratio"] = 12.0f;
        p.values["attack"] = 0.1f;
        p.values["release"] = 80.0f;
        p.values["makeupGain"] = 3.0f;
        p.values["listen"] = 0.0f;
        presets.push_back(p);
    }

    return presets;
}
