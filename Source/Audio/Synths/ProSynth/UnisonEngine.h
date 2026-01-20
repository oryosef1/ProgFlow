#pragma once

#include <juce_core/juce_core.h>
#include <vector>

enum class UnisonSpreadMode
{
    Linear = 0,
    Exponential,
    Random,
    Center
};

/**
 * UnisonEngine - Voice spreading and detuning calculator
 *
 * Features:
 * - 1-16 voice unison
 * - Multiple spread modes
 * - Stereo pan distribution
 * - Detune calculation
 *
 * Note: This class only calculates detune/pan values.
 * Actual voice creation is handled by the synth.
 */
class UnisonEngine
{
public:
    UnisonEngine();

    //==========================================================================
    // Voice count
    void setVoiceCount(int count); // 1-16
    int getVoiceCount() const { return voiceCount; }

    //==========================================================================
    // Detune
    void setDetune(float cents); // 0-100
    float getDetune() const { return detune; }

    //==========================================================================
    // Spread mode
    void setSpreadMode(UnisonSpreadMode mode);
    UnisonSpreadMode getSpreadMode() const { return spreadMode; }

    //==========================================================================
    // Stereo spread
    void setStereoSpread(float spread); // 0-1
    float getStereoSpread() const { return stereoSpread; }

    //==========================================================================
    // Blend (wet/dry)
    void setBlend(float blend); // 0-1
    float getBlend() const { return blend; }

    //==========================================================================
    // Calculate values for voice index
    float getDetuneForVoice(int voiceIndex) const;
    float getPanForVoice(int voiceIndex) const;
    float getGainForVoice(int voiceIndex) const;

private:
    int voiceCount = 1;
    float detune = 0.0f;
    UnisonSpreadMode spreadMode = UnisonSpreadMode::Linear;
    float stereoSpread = 0.0f;
    float blend = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnisonEngine)
};
