#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>

/**
 * Filter model types (emulating analog filter characteristics)
 */
enum class ProFilterModel
{
    Clean = 0,    // Transparent, neutral
    Moog,         // Creamy, warm ladder filter
    MS20,         // Aggressive, screamy
    Jupiter,      // Smooth, polished
    Oberheim      // Thick, punchy
};

/**
 * Filter types
 */
enum class ProFilterType
{
    LowPass = 0,
    HighPass,
    BandPass,
    Notch
};

/**
 * ProSynthFilter - Advanced filter with multiple analog models
 *
 * Features:
 * - Multiple filter models with character
 * - Drive/saturation per model
 * - Self-oscillation at high resonance
 * - Frequency modulation input
 */
class ProSynthFilter
{
public:
    ProSynthFilter();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void reset();

    //==========================================================================
    // Model and type
    void setModel(ProFilterModel model);
    ProFilterModel getModel() const { return model; }

    void setType(ProFilterType type);
    ProFilterType getType() const { return filterType; }

    //==========================================================================
    // Parameters
    void setCutoff(float frequency);
    float getCutoff() const { return cutoff; }

    void setResonance(float resonance); // 0-1
    float getResonance() const { return resonance; }

    void setDrive(float drive); // 0-1
    float getDrive() const { return drive; }

    //==========================================================================
    // Processing
    float processSample(float input);

private:
    ProFilterModel model = ProFilterModel::Clean;
    ProFilterType filterType = ProFilterType::LowPass;

    float cutoff = 5000.0f;
    float resonance = 0.0f;
    float drive = 0.0f;

    // Filter implementation
    juce::dsp::StateVariableTPTFilter<float> filter;

    // Drive/saturation
    float inputGain = 1.0f;
    float outputGain = 1.0f;

    // Feedback for enhanced resonance
    float feedbackSample = 0.0f;
    float feedbackAmount = 0.0f;

    double sampleRate = 44100.0;

    // Update internal parameters based on model
    void updateFilterCharacter();
    void updateResonance();
    void updateDrive();

    // Saturation function per model
    float applySaturation(float input);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProSynthFilter)
};
