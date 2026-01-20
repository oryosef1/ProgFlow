#pragma once

#include "EffectBase.h"

/**
 * SidechainCompressorEffect - Compressor with external key input
 *
 * Uses audio from a sidechain source track to drive gain reduction
 * on the main signal. Common use: ducking pads/bass when kick drum hits.
 *
 * Parameters:
 * - threshold: dB level where compression starts (-60 to 0)
 * - ratio: Compression ratio (1:1 to 20:1)
 * - attack: Attack time in ms (0.1-100)
 * - release: Release time in ms (10-1000)
 * - makeupGain: Output gain in dB (0-24)
 * - listen: 0=output, 1=sidechain signal (for monitoring)
 */
class SidechainCompressorEffect : public EffectBase
{
public:
    SidechainCompressorEffect();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void reset() override;

    juce::String getName() const override { return "Sidechain Compressor"; }
    juce::String getCategory() const override { return "Dynamics"; }

    std::vector<EffectPreset> getPresets() const override;

    //==========================================================================
    // Sidechain routing

    /**
     * Set the source track index for sidechain input
     * -1 means no source (compressor acts like normal compressor)
     */
    void setSidechainSource(int trackIndex);
    int getSidechainSource() const { return sidechainSourceTrack; }

    /**
     * Provide sidechain audio for the current processing block.
     * Must be called before processBlock() if sidechainSource >= 0.
     * @param buffer The sidechain audio buffer (key signal)
     */
    void setSidechainInput(const juce::AudioBuffer<float>& buffer);

    /**
     * Check if sidechain input has been provided for this block
     */
    bool hasSidechainInput() const { return sidechainInputProvided; }

    // For metering
    float getGainReduction() const { return gainReduction.load(); }
    float getSidechainLevel() const { return sidechainLevel.load(); }

protected:
    void processEffect(juce::AudioBuffer<float>& buffer) override;
    void onParameterChanged(const juce::String& name, float value) override;

private:
    // Compressor envelope follower state
    float envelopeDb = -100.0f;

    // Sidechain routing
    int sidechainSourceTrack = -1;  // -1 = no sidechain (use input signal)
    juce::AudioBuffer<float> sidechainBuffer;
    bool sidechainInputProvided = false;

    // Parameters
    float thresholdDb = -20.0f;
    float ratio = 4.0f;
    float attackMs = 10.0f;
    float releaseMs = 100.0f;
    float makeupGainDb = 0.0f;
    bool listenToSidechain = false;

    // Calculated coefficients
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Metering (atomic for thread-safe UI access)
    std::atomic<float> gainReduction{0.0f};
    std::atomic<float> sidechainLevel{0.0f};

    // Helper methods
    void updateCoefficients();
    float computeGain(float inputDb) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainCompressorEffect)
};
