#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <map>
#include <vector>

/**
 * EffectParameter - A single effect parameter with metadata
 */
struct EffectParameter
{
    juce::String name;
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float defaultValue = 0.0f;
    float step = 0.0f;      // 0 = continuous
    juce::String unit;      // "ms", "dB", "Hz", "%"

    float getNormalized() const
    {
        if (maxValue <= minValue) return 0.0f;
        return (value - minValue) / (maxValue - minValue);
    }

    void setFromNormalized(float normalized)
    {
        value = minValue + normalized * (maxValue - minValue);
        if (step > 0.0f)
            value = std::round(value / step) * step;
    }
};

/**
 * EffectPreset - A named collection of parameter values
 */
struct EffectPreset
{
    juce::String name;
    std::map<juce::String, float> values;
};

/**
 * EffectBase - Abstract base class for all audio effects
 *
 * Signal flow:
 * ```
 * input ──┬── dryGain ────┬── output
 *         └── [effect] ── wetGain ─┘
 * ```
 *
 * Subclasses must:
 * 1. Override processEffect() to do the actual processing
 * 2. Override onParameterChanged() to respond to parameter changes
 * 3. Override getPresets() to return available presets
 */
class EffectBase
{
public:
    EffectBase();
    virtual ~EffectBase() = default;

    //==========================================================================
    // Audio processing
    virtual void prepareToPlay(double sampleRate, int samplesPerBlock);
    virtual void processBlock(juce::AudioBuffer<float>& buffer);
    virtual void releaseResources();
    virtual void reset();

    //==========================================================================
    // Wet/dry mix control (0-1)
    void setWetDry(float wet);
    float getWetDry() const { return wetAmount; }

    //==========================================================================
    // Bypass control
    void setBypass(bool shouldBypass);
    bool isBypassed() const { return bypassed; }

    //==========================================================================
    // Parameter management
    void setParameter(const juce::String& name, float value);
    float getParameter(const juce::String& name) const;
    const EffectParameter* getParameterInfo(const juce::String& name) const;
    std::vector<juce::String> getParameterNames() const;

    //==========================================================================
    // Presets
    virtual std::vector<EffectPreset> getPresets() const { return {}; }
    void loadPreset(const EffectPreset& preset);
    void loadPreset(int index);

    //==========================================================================
    // Effect info
    virtual juce::String getName() const = 0;
    virtual juce::String getCategory() const { return "Effect"; }

protected:
    // Parameters storage
    std::map<juce::String, EffectParameter> parameters;

    // Audio settings
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Helper for subclasses to register parameters
    void addParameter(const juce::String& id, const juce::String& name,
                     float defaultValue, float minValue, float maxValue,
                     const juce::String& unit = "", float step = 0.0f);

    // Override in subclasses to respond to parameter changes
    virtual void onParameterChanged(const juce::String& name, float value) {}

    // Override in subclasses to process the wet signal
    virtual void processEffect(juce::AudioBuffer<float>& buffer) = 0;

private:
    // Wet/dry control
    float wetAmount = 1.0f;
    bool bypassed = false;

    // Dry buffer for wet/dry mixing
    juce::AudioBuffer<float> dryBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectBase)
};
