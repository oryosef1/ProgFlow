#include "NoiseGenerator.h"

NoiseGenerator::NoiseGenerator()
{
}

void NoiseGenerator::prepareToPlay(double sr, int blockSize)
{
    sampleRate = sr;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sr;
    spec.maximumBlockSize = static_cast<juce::uint32>(blockSize);
    spec.numChannels = 1;

    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    filter.setCutoffFrequency(filterCutoff);
    filter.setResonance(filterResonance);

    reset();
}

void NoiseGenerator::reset()
{
    filter.reset();
    for (int i = 0; i < 7; ++i)
        pinkState[i] = 0.0f;
    brownState = 0.0f;
}

void NoiseGenerator::setNoiseType(NoiseType type)
{
    noiseType = type;
}

void NoiseGenerator::setLevel(float lvl)
{
    level = juce::jlimit(0.0f, 1.0f, lvl);
}

void NoiseGenerator::setFilterEnabled(bool enabled)
{
    filterEnabled = enabled;
}

void NoiseGenerator::setFilterType(NoiseFilterType type)
{
    filterType = type;

    switch (type)
    {
        case NoiseFilterType::LowPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            break;
        case NoiseFilterType::HighPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            break;
        case NoiseFilterType::BandPass:
            filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
            break;
    }
}

void NoiseGenerator::setFilterCutoff(float hz)
{
    filterCutoff = juce::jlimit(20.0f, 20000.0f, hz);
    filter.setCutoffFrequency(filterCutoff);
}

void NoiseGenerator::setFilterResonance(float resonance)
{
    filterResonance = juce::jlimit(0.0f, 1.0f, resonance);
    // Map 0-1 to Q of 0.5-12
    float Q = 0.5f + filterResonance * 11.5f;
    filter.setResonance(Q);
}

void NoiseGenerator::trigger()
{
    if (level > 0.0f)
        playing = true;
}

void NoiseGenerator::release()
{
    playing = false;
}

float NoiseGenerator::generateNoise()
{
    auto& random = juce::Random::getSystemRandom();

    switch (noiseType)
    {
        case NoiseType::White:
            return random.nextFloat() * 2.0f - 1.0f;

        case NoiseType::Pink:
        {
            // Paul Kellett's refined pink noise algorithm
            float white = random.nextFloat() * 2.0f - 1.0f;
            pinkState[0] = 0.99886f * pinkState[0] + white * 0.0555179f;
            pinkState[1] = 0.99332f * pinkState[1] + white * 0.0750759f;
            pinkState[2] = 0.96900f * pinkState[2] + white * 0.1538520f;
            pinkState[3] = 0.86650f * pinkState[3] + white * 0.3104856f;
            pinkState[4] = 0.55000f * pinkState[4] + white * 0.5329522f;
            pinkState[5] = -0.7616f * pinkState[5] - white * 0.0168980f;
            float pink = pinkState[0] + pinkState[1] + pinkState[2] + pinkState[3] +
                        pinkState[4] + pinkState[5] + pinkState[6] + white * 0.5362f;
            pinkState[6] = white * 0.115926f;
            return pink * 0.11f; // Scale to approximately -1 to 1
        }

        case NoiseType::Brown:
        {
            // Brownian noise (integrated white noise)
            float white = random.nextFloat() * 2.0f - 1.0f;
            brownState = (brownState + white * 0.02f);
            brownState = juce::jlimit(-1.0f, 1.0f, brownState);
            return brownState;
        }
    }

    return 0.0f;
}

float NoiseGenerator::processSample()
{
    if (!playing || level <= 0.0f)
        return 0.0f;

    float noise = generateNoise();

    // Apply filter if enabled
    if (filterEnabled)
    {
        noise = filter.processSample(0, noise);
    }

    return noise * level;
}
