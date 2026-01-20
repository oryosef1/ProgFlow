#include "SubOscillator.h"
#include <cmath>

SubOscillator::SubOscillator()
{
}

void SubOscillator::prepareToPlay(double sr)
{
    sampleRate = sr;
    reset();
}

void SubOscillator::reset()
{
    phase = 0.0;
}

void SubOscillator::setOctave(int oct)
{
    octave = juce::jlimit(-2, -1, oct);
}

void SubOscillator::setWaveform(SubOscWaveform wave)
{
    waveform = wave;
}

void SubOscillator::setLevel(float lvl)
{
    level = juce::jlimit(0.0f, 1.0f, lvl);
}

void SubOscillator::trigger(float baseFrequency)
{
    if (level <= 0.0f)
        return;

    // Calculate sub frequency
    float octaveMultiplier = std::pow(2.0f, static_cast<float>(octave));
    frequency = baseFrequency * octaveMultiplier;

    playing = true;
    phase = 0.0;
}

void SubOscillator::release()
{
    playing = false;
}

float SubOscillator::generateSample()
{
    float normalizedPhase = std::fmod(phase, 1.0);
    if (normalizedPhase < 0.0f)
        normalizedPhase += 1.0f;

    switch (waveform)
    {
        case SubOscWaveform::Sine:
            return std::sin(normalizedPhase * juce::MathConstants<float>::twoPi);

        case SubOscWaveform::Triangle:
            return 2.0f * std::abs(2.0f * (normalizedPhase - std::floor(normalizedPhase + 0.5f))) - 1.0f;

        case SubOscWaveform::Square:
            return normalizedPhase < 0.5f ? 1.0f : -1.0f;
    }

    return 0.0f;
}

float SubOscillator::processSample()
{
    if (!playing || level <= 0.0f)
        return 0.0f;

    float sample = generateSample() * level;

    // Advance phase
    double phaseIncrement = frequency / sampleRate;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample;
}
