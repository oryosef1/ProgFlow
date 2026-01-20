#include "ProSynthFilter.h"
#include <cmath>

ProSynthFilter::ProSynthFilter()
{
    updateFilterCharacter();
}

void ProSynthFilter::prepareToPlay(double sr, int blockSize)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(cutoff);

    updateFilterCharacter();
}

void ProSynthFilter::reset()
{
    filter.reset();
    feedbackSample = 0.0f;
}

void ProSynthFilter::setModel(ProFilterModel m)
{
    model = m;
    updateFilterCharacter();
}

void ProSynthFilter::setType(ProFilterType type)
{
    filterType = type;

    switch (type)
    {
        case ProFilterType::LowPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case ProFilterType::HighPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case ProFilterType::BandPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
        case ProFilterType::Notch:
            // Use lowpass for now (Notch not directly available in StateVariableTPT)
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
    }
}

void ProSynthFilter::setCutoff(float frequency)
{
    cutoff = juce::jlimit(20.0f, 20000.0f, frequency);
    filter.setCutoffFrequency(cutoff);
}

void ProSynthFilter::setResonance(float res)
{
    resonance = juce::jlimit(0.0f, 1.0f, res);
    updateResonance();
}

void ProSynthFilter::setDrive(float drv)
{
    drive = juce::jlimit(0.0f, 1.0f, drv);
    updateDrive();
}

void ProSynthFilter::updateFilterCharacter()
{
    updateResonance();
    updateDrive();
}

void ProSynthFilter::updateResonance()
{
    float Q = 0.5f;
    float feedback = 0.0f;

    switch (model)
    {
        case ProFilterModel::Moog:
            // Smooth, musical resonance with self-oscillation
            Q = 0.5f + resonance * resonance * 18.0f;
            feedback = resonance * resonance * 0.3f;
            break;

        case ProFilterModel::MS20:
            // Aggressive, screamy resonance
            Q = 0.5f + resonance * resonance * 25.0f;
            feedback = resonance * resonance * 0.5f;
            break;

        case ProFilterModel::Jupiter:
            // Smooth, polished resonance
            Q = 0.5f + resonance * 12.0f;
            feedback = resonance * 0.1f;
            break;

        case ProFilterModel::Oberheim:
            // Thick, punchy resonance
            Q = 0.5f + resonance * 15.0f;
            feedback = resonance * 0.2f;
            break;

        case ProFilterModel::Clean:
        default:
            // Clean, transparent
            Q = 0.5f + resonance * 12.0f;
            feedback = 0.0f;
            break;
    }

    filter.setResonance(juce::jmin(Q, 20.0f));
    feedbackAmount = feedback;
}

void ProSynthFilter::updateDrive()
{
    switch (model)
    {
        case ProFilterModel::Moog:
            // Warm, tube-like saturation
            inputGain = 1.0f + drive * 2.0f;
            outputGain = 1.0f / (1.0f + drive * 0.5f);
            break;

        case ProFilterModel::MS20:
            // Harsh, distorted drive
            inputGain = 1.0f + drive * 4.0f;
            outputGain = 1.0f / (1.0f + drive);
            break;

        case ProFilterModel::Jupiter:
            // Subtle, clean drive
            inputGain = 1.0f + drive * 1.5f;
            outputGain = 1.0f / (1.0f + drive * 0.3f);
            break;

        case ProFilterModel::Oberheim:
            // Punchy, colored drive
            inputGain = 1.0f + drive * 3.0f;
            outputGain = 1.0f / (1.0f + drive * 0.7f);
            break;

        case ProFilterModel::Clean:
        default:
            // No coloration
            inputGain = 1.0f;
            outputGain = 1.0f;
            break;
    }
}

float ProSynthFilter::applySaturation(float input)
{
    switch (model)
    {
        case ProFilterModel::Moog:
            // Warm tanh saturation
            return std::tanh(input * (1.0f + drive * 2.0f));

        case ProFilterModel::MS20:
            // Harsh clipping
            {
                float amount = 1.0f + drive * 3.0f;
                return juce::jlimit(-1.0f, 1.0f, input * amount);
            }

        case ProFilterModel::Jupiter:
            // Soft saturation
            return input / (1.0f + std::abs(input) * drive * 0.5f);

        case ProFilterModel::Oberheim:
            // Asymmetric saturation (punchy)
            if (input >= 0.0f)
                return std::tanh(input * (1.0f + drive * 1.5f));
            else
                return std::tanh(input * (1.0f + drive * 2.5f));

        case ProFilterModel::Clean:
        default:
            // Linear (no saturation)
            return input;
    }
}

float ProSynthFilter::processSample(float input)
{
    // Apply input gain
    float signal = input * inputGain;

    // Add feedback for enhanced resonance
    signal += feedbackSample * feedbackAmount;

    // Apply saturation
    signal = applySaturation(signal);

    // Process through filter
    float filtered = filter.processSample(0, signal);

    // Store for feedback
    feedbackSample = filtered;

    // Apply output gain
    return filtered * outputGain;
}
