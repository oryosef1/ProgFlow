#include "ProSynthLFO.h"
#include <cmath>

ProSynthLFO::ProSynthLFO()
{
}

void ProSynthLFO::prepareToPlay(double sr)
{
    sampleRate = sr;
    reset();
}

void ProSynthLFO::reset()
{
    phase = phaseOffset / 360.0;
    delayEnvelope = delayTime > 0.0f ? 0.0f : 1.0f;
    delayCounter = 0.0f;
    sampleHoldValue = 0.0f;
    sampleHoldCounter = 0.0f;
    randomCurrentValue = 0.0f;
}

void ProSynthLFO::setRate(float hz)
{
    rate = juce::jlimit(0.01f, 50.0f, hz);
}

void ProSynthLFO::setShape(LFOShape s)
{
    shape = s;
    // Reset phase for consistent behavior
    if (shape == LFOShape::SampleHold || shape == LFOShape::Random)
    {
        sampleHoldCounter = 0.0f;
        sampleHoldValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
        randomTargetValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }
}

void ProSynthLFO::setPhase(float degrees)
{
    phaseOffset = std::fmod(degrees, 360.0f);
    if (phaseOffset < 0.0f)
        phaseOffset += 360.0f;
}

void ProSynthLFO::setDelay(float seconds)
{
    delayTime = juce::jlimit(0.0f, 5.0f, seconds);
}

void ProSynthLFO::triggerRetrigger()
{
    if (retrigger)
    {
        reset();
    }
}

void ProSynthLFO::setSync(bool enabled, LFOSyncValue sync)
{
    synced = enabled;
    syncValue = sync;
}

void ProSynthLFO::setBPM(float bpmValue)
{
    bpm = juce::jmax(1.0f, bpmValue);
}

void ProSynthLFO::setRange(float min, float max)
{
    minValue = min;
    maxValue = max;
}

void ProSynthLFO::start()
{
    running = true;
}

void ProSynthLFO::stop()
{
    running = false;
}

float ProSynthLFO::getEffectiveRate() const
{
    if (!synced)
        return rate;

    // Convert BPM to frequency based on sync value
    float quarterNoteHz = bpm / 60.0f; // Quarter notes per second
    float multiplier = 1.0f;

    switch (syncValue)
    {
        case LFOSyncValue::Whole:         multiplier = 0.25f; break;
        case LFOSyncValue::Half:          multiplier = 0.5f; break;
        case LFOSyncValue::Quarter:       multiplier = 1.0f; break;
        case LFOSyncValue::Eighth:        multiplier = 2.0f; break;
        case LFOSyncValue::Sixteenth:     multiplier = 4.0f; break;
        case LFOSyncValue::ThirtySecond:  multiplier = 8.0f; break;
        case LFOSyncValue::HalfTriplet:   multiplier = 0.5f * 1.5f; break;
        case LFOSyncValue::QuarterTriplet:multiplier = 1.0f * 1.5f; break;
        case LFOSyncValue::EighthTriplet: multiplier = 2.0f * 1.5f; break;
        case LFOSyncValue::QuarterDotted: multiplier = 1.0f / 1.5f; break;
        case LFOSyncValue::EighthDotted:  multiplier = 2.0f / 1.5f; break;
    }

    return quarterNoteHz * multiplier;
}

float ProSynthLFO::generateSample(LFOShape waveShape, double phaseValue)
{
    float normalizedPhase = std::fmod(phaseValue, 1.0);
    if (normalizedPhase < 0.0f)
        normalizedPhase += 1.0f;

    switch (waveShape)
    {
        case LFOShape::Sine:
            return std::sin(normalizedPhase * juce::MathConstants<float>::twoPi);

        case LFOShape::Triangle:
            return 2.0f * std::abs(2.0f * (normalizedPhase - std::floor(normalizedPhase + 0.5f))) - 1.0f;

        case LFOShape::Saw:
            return 2.0f * (normalizedPhase - std::floor(normalizedPhase + 0.5f));

        case LFOShape::Square:
            return normalizedPhase < 0.5f ? 1.0f : -1.0f;

        case LFOShape::SampleHold:
        case LFOShape::Random:
            // Handled separately in processSample()
            return 0.0f;
    }

    return 0.0f;
}

float ProSynthLFO::processSample()
{
    if (!running)
        return 0.0f;

    float effectiveRate = getEffectiveRate();

    // Handle delay envelope
    if (delayTime > 0.0f && delayCounter < delayTime)
    {
        delayCounter += static_cast<float>(1.0 / sampleRate);
        delayEnvelope = juce::jmin(1.0f, delayCounter / delayTime);
    }
    else
    {
        delayEnvelope = 1.0f;
    }

    float output = 0.0f;

    // Special handling for sample & hold and random
    if (shape == LFOShape::SampleHold || shape == LFOShape::Random)
    {
        float period = 1.0f / effectiveRate;
        sampleHoldCounter += static_cast<float>(1.0 / sampleRate);

        if (sampleHoldCounter >= period)
        {
            sampleHoldCounter = 0.0f;

            if (shape == LFOShape::SampleHold)
            {
                sampleHoldValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            }
            else // Random
            {
                randomTargetValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            }
        }

        if (shape == LFOShape::SampleHold)
        {
            output = sampleHoldValue;
        }
        else // Random - smooth interpolation
        {
            float t = sampleHoldCounter / period;
            randomCurrentValue = randomCurrentValue + (randomTargetValue - randomCurrentValue) * 0.05f;
            output = randomCurrentValue;
        }
    }
    else
    {
        // Standard waveform generation
        output = generateSample(shape, phase);

        // Advance phase
        double phaseIncrement = effectiveRate / sampleRate;
        phase += phaseIncrement;
        if (phase >= 1.0)
            phase -= 1.0;
    }

    // Apply delay envelope
    output *= delayEnvelope;

    // Map to output range
    output = minValue + (output * 0.5f + 0.5f) * (maxValue - minValue);

    return output;
}
