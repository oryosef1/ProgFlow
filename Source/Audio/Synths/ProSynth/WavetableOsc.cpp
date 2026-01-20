#include "WavetableOsc.h"
#include <cmath>

//==============================================================================
// Static initialization
std::vector<Wavetable> WavetableOsc::builtInWavetables;
bool WavetableOsc::wavetablesInitialized = false;

//==============================================================================
// Wavetable generation helpers

std::vector<float> WavetableOsc::generateWaveform(const juce::String& type, float param)
{
    std::vector<float> samples(Wavetable::WAVETABLE_SIZE);

    for (int i = 0; i < Wavetable::WAVETABLE_SIZE; ++i)
    {
        float phase = static_cast<float>(i) / static_cast<float>(Wavetable::WAVETABLE_SIZE);

        if (type == "sine")
        {
            samples[i] = std::sin(phase * juce::MathConstants<float>::twoPi);
        }
        else if (type == "triangle")
        {
            samples[i] = 2.0f * std::abs(2.0f * (phase - std::floor(phase + 0.5f))) - 1.0f;
        }
        else if (type == "sawtooth")
        {
            samples[i] = 2.0f * (phase - std::floor(phase + 0.5f));
        }
        else if (type == "square")
        {
            samples[i] = phase < 0.5f ? 1.0f : -1.0f;
        }
        else if (type == "pulse")
        {
            samples[i] = phase < param ? 1.0f : -1.0f;
        }
    }

    return samples;
}

std::vector<float> WavetableOsc::generateHarmonics(const std::vector<int>& harmonics,
                                                   const std::vector<float>& amplitudes)
{
    std::vector<float> samples(Wavetable::WAVETABLE_SIZE, 0.0f);

    for (size_t h = 0; h < harmonics.size() && h < amplitudes.size(); ++h)
    {
        int harmonic = harmonics[h];
        float amplitude = amplitudes[h];

        for (int i = 0; i < Wavetable::WAVETABLE_SIZE; ++i)
        {
            float phase = static_cast<float>(i) / static_cast<float>(Wavetable::WAVETABLE_SIZE);
            samples[i] += amplitude * std::sin(phase * juce::MathConstants<float>::twoPi * harmonic);
        }
    }

    // Normalize
    float maxVal = 0.0f;
    for (float s : samples)
        maxVal = std::max(maxVal, std::abs(s));

    if (maxVal > 0.0f)
    {
        for (float& s : samples)
            s /= maxVal;
    }

    return samples;
}

void WavetableOsc::initializeBuiltInWavetables()
{
    if (wavetablesInitialized)
        return;

    // Basic Wavetables
    {
        Wavetable wt;
        wt.id = "wt-basic-saw";
        wt.name = "Basic Saw";
        wt.category = "Basic";
        wt.frames.push_back(generateWaveform("sawtooth"));
        builtInWavetables.push_back(wt);
    }

    {
        Wavetable wt;
        wt.id = "wt-basic-square";
        wt.name = "Basic Square";
        wt.category = "Basic";
        wt.frames.push_back(generateWaveform("square"));
        builtInWavetables.push_back(wt);
    }

    {
        Wavetable wt;
        wt.id = "wt-basic-triangle";
        wt.name = "Basic Triangle";
        wt.category = "Basic";
        wt.frames.push_back(generateWaveform("triangle"));
        builtInWavetables.push_back(wt);
    }

    // Analog Wavetables (pulse width morphing)
    {
        Wavetable wt;
        wt.id = "wt-analog-pwm";
        wt.name = "Pulse Width Morph";
        wt.category = "Analog";
        for (float pw = 0.1f; pw <= 0.9f; pw += 0.1f)
            wt.frames.push_back(generateWaveform("pulse", pw));
        builtInWavetables.push_back(wt);
    }

    // Digital Wavetables (harmonic morphing)
    {
        Wavetable wt;
        wt.id = "wt-digital-harm";
        wt.name = "Harmonic Morph";
        wt.category = "Digital";

        // Add frames with increasing odd harmonics
        for (int maxHarm = 1; maxHarm <= 9; maxHarm += 2)
        {
            std::vector<int> harms;
            std::vector<float> amps;
            for (int h = 1; h <= maxHarm; h += 2)
            {
                harms.push_back(h);
                amps.push_back(1.0f / static_cast<float>(h));
            }
            wt.frames.push_back(generateHarmonics(harms, amps));
        }
        builtInWavetables.push_back(wt);
    }

    // Bass Wavetable
    {
        Wavetable wt;
        wt.id = "wt-bass-deep";
        wt.name = "Deep Bass";
        wt.category = "Bass";
        wt.frames.push_back(generateHarmonics({1, 2, 3}, {1.0f, 0.5f, 0.3f}));
        builtInWavetables.push_back(wt);
    }

    wavetablesInitialized = true;
}

//==============================================================================
// WavetableOsc Implementation

WavetableOsc::WavetableOsc()
{
    initializeBuiltInWavetables();
    interpolatedFrame.resize(Wavetable::WAVETABLE_SIZE, 0.0f);

    // Set default wavetable
    if (!builtInWavetables.empty())
    {
        setWavetable(builtInWavetables[0]);
    }
}

void WavetableOsc::prepareToPlay(double sr, int /*samplesPerBlock*/)
{
    sampleRate = sr;
    reset();
}

void WavetableOsc::reset()
{
    phase = 0.0;
}

void WavetableOsc::setWavetableById(const juce::String& id)
{
    for (const auto& wt : builtInWavetables)
    {
        if (wt.id == id)
        {
            setWavetable(wt);
            return;
        }
    }
}

void WavetableOsc::setWavetable(const Wavetable& wavetable)
{
    currentWavetable = &wavetable;
    updateInterpolatedFrame();
}

void WavetableOsc::setPosition(float pos)
{
    position = juce::jlimit(0.0f, 1.0f, pos);
    updateInterpolatedFrame();
}

void WavetableOsc::updateInterpolatedFrame()
{
    if (!currentWavetable || currentWavetable->frames.empty())
        return;

    const auto& frames = currentWavetable->frames;

    if (frames.size() == 1)
    {
        // Single frame - direct copy
        interpolatedFrame = frames[0];
        return;
    }

    // Multi-frame: interpolate between frames based on position
    float frameIndex = position * static_cast<float>(frames.size() - 1);
    int frame1 = static_cast<int>(std::floor(frameIndex));
    int frame2 = std::min(frame1 + 1, static_cast<int>(frames.size() - 1));
    float frac = frameIndex - static_cast<float>(frame1);

    const auto& f1 = frames[frame1];
    const auto& f2 = frames[frame2];

    for (size_t i = 0; i < interpolatedFrame.size(); ++i)
    {
        interpolatedFrame[i] = f1[i] * (1.0f - frac) + f2[i] * frac;
    }
}

void WavetableOsc::setFrequency(float freq)
{
    frequency = juce::jlimit(1.0f, 20000.0f, freq);
}

void WavetableOsc::setLevel(float lvl)
{
    level = juce::jlimit(0.0f, 1.0f, lvl);
}

float WavetableOsc::processSample()
{
    if (!playing || interpolatedFrame.empty())
        return 0.0f;

    // Get sample from wavetable using linear interpolation
    double indexDouble = phase * static_cast<double>(interpolatedFrame.size());
    int index1 = static_cast<int>(indexDouble) % interpolatedFrame.size();
    int index2 = (index1 + 1) % interpolatedFrame.size();
    float frac = static_cast<float>(indexDouble - std::floor(indexDouble));

    float sample = interpolatedFrame[index1] * (1.0f - frac) + interpolatedFrame[index2] * frac;

    // Advance phase
    double phaseIncrement = frequency / sampleRate;
    phase += phaseIncrement;
    if (phase >= 1.0)
        phase -= 1.0;

    return sample * level;
}

void WavetableOsc::start()
{
    playing = true;
}

void WavetableOsc::stop()
{
    playing = false;
}

std::vector<Wavetable> WavetableOsc::getBuiltInWavetables()
{
    initializeBuiltInWavetables();
    return builtInWavetables;
}
