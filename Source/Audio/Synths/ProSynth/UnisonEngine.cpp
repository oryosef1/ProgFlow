#include "UnisonEngine.h"
#include <cmath>

UnisonEngine::UnisonEngine()
{
}

void UnisonEngine::setVoiceCount(int count)
{
    voiceCount = juce::jlimit(1, 16, count);
}

void UnisonEngine::setDetune(float cents)
{
    detune = juce::jlimit(0.0f, 100.0f, cents);
}

void UnisonEngine::setSpreadMode(UnisonSpreadMode mode)
{
    spreadMode = mode;
}

void UnisonEngine::setStereoSpread(float spread)
{
    stereoSpread = juce::jlimit(0.0f, 1.0f, spread);
}

void UnisonEngine::setBlend(float blendAmount)
{
    blend = juce::jlimit(0.0f, 1.0f, blendAmount);
}

float UnisonEngine::getDetuneForVoice(int voiceIndex) const
{
    if (voiceCount <= 1)
        return 0.0f;

    float detuneValue = 0.0f;

    switch (spreadMode)
    {
        case UnisonSpreadMode::Linear:
        {
            // Evenly distributed from -detune to +detune
            float t = static_cast<float>(voiceIndex) / static_cast<float>(voiceCount - 1);
            detuneValue = -detune + t * 2.0f * detune;
            break;
        }

        case UnisonSpreadMode::Exponential:
        {
            // Exponential distribution (more voices near center)
            float t = static_cast<float>(voiceIndex) / static_cast<float>(voiceCount - 1);
            float sign = t < 0.5f ? -1.0f : 1.0f;
            float normalized = std::abs(t - 0.5f) * 2.0f;
            float exp = std::pow(normalized, 0.5f); // Square root for softer curve
            detuneValue = sign * exp * detune;
            break;
        }

        case UnisonSpreadMode::Random:
        {
            // Pseudo-random based on voice index (deterministic)
            float seed = std::sin(static_cast<float>(voiceIndex) * 12.9898f) * 43758.5453f;
            float rand = seed - std::floor(seed);
            detuneValue = (rand * 2.0f - 1.0f) * detune;
            break;
        }

        case UnisonSpreadMode::Center:
        {
            // One voice at center, others spread out
            if (voiceCount % 2 == 1)
            {
                // Odd number: middle voice at 0
                int half = voiceCount / 2;
                if (voiceIndex == half)
                {
                    detuneValue = 0.0f;
                }
                else
                {
                    int offset = voiceIndex < half ? -(half - voiceIndex) : (voiceIndex - half);
                    float normalizedOffset = static_cast<float>(offset) / static_cast<float>(half);
                    detuneValue = normalizedOffset * detune;
                }
            }
            else
            {
                // Even number: pair voices around center
                int half = voiceCount / 2;
                float offset = voiceIndex < half ?
                              -(static_cast<float>(half - voiceIndex) - 0.5f) :
                              (static_cast<float>(voiceIndex - half) + 0.5f);
                float normalizedOffset = offset / static_cast<float>(half);
                detuneValue = normalizedOffset * detune;
            }
            break;
        }
    }

    return detuneValue;
}

float UnisonEngine::getPanForVoice(int voiceIndex) const
{
    if (voiceCount <= 1)
        return 0.0f;

    // Map voice index to pan position -1 to 1
    float t = static_cast<float>(voiceIndex) / static_cast<float>(voiceCount - 1);
    float pan = (t * 2.0f - 1.0f) * stereoSpread;

    return juce::jlimit(-1.0f, 1.0f, pan);
}

float UnisonEngine::getGainForVoice(int /*voiceIndex*/) const
{
    // Normalize gain based on voice count to prevent clipping
    float normalizedGain = 1.0f / std::sqrt(static_cast<float>(voiceCount));

    // Apply blend
    return normalizedGain * blend;
}
