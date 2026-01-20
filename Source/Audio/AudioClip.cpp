#include "AudioClip.h"

AudioClip::AudioClip()
    : id(juce::Uuid().toString())
{
}

//==============================================================================
double AudioClip::getEndBeat() const
{
    // Calculate end beat using a default BPM of 120
    // For accurate results, callers should use: startBeat + getDurationInBeats(actualBpm)
    constexpr double defaultBpm = 120.0;
    return startBeat + getDurationInBeats(defaultBpm);
}

double AudioClip::getDurationInBeats(double bpm) const
{
    if (bpm <= 0) return 0;

    double durationSeconds = getTrimmedDurationInSamples() / fileSampleRate / playbackRate;
    double beatsPerSecond = bpm / 60.0;
    return durationSeconds * beatsPerSecond;
}

//==============================================================================
void AudioClip::setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    audioBuffer = buffer;
    fileSampleRate = sampleRate;

    // Reset trim to full clip
    trimStart = 0;
    trimEnd = buffer.getNumSamples();
}

void AudioClip::setAudioBuffer(juce::AudioBuffer<float>&& buffer, double sampleRate)
{
    audioBuffer = std::move(buffer);
    fileSampleRate = sampleRate;

    // Reset trim to full clip
    trimStart = 0;
    trimEnd = audioBuffer.getNumSamples();
}

double AudioClip::getDurationInSeconds() const
{
    if (fileSampleRate <= 0) return 0;
    return static_cast<double>(getDurationInSamples()) / fileSampleRate;
}

//==============================================================================
float AudioClip::getSample(int channel, juce::int64 sampleIndex) const
{
    if (channel < 0 || channel >= audioBuffer.getNumChannels())
        return 0.0f;

    if (sampleIndex < 0 || sampleIndex >= audioBuffer.getNumSamples())
        return 0.0f;

    return audioBuffer.getSample(channel, static_cast<int>(sampleIndex));
}

//==============================================================================
void AudioClip::setGain(float newGain)
{
    gain = juce::jlimit(0.0f, 4.0f, newGain);
}

//==============================================================================
void AudioClip::setFadeInSamples(juce::int64 samples)
{
    fadeInSamples = std::max((juce::int64)0, samples);
}

void AudioClip::setFadeOutSamples(juce::int64 samples)
{
    fadeOutSamples = std::max((juce::int64)0, samples);
}

//==============================================================================
void AudioClip::setTrimStartSample(juce::int64 sample)
{
    trimStart = juce::jlimit((juce::int64)0, trimEnd - 1, sample);
}

void AudioClip::setTrimEndSample(juce::int64 sample)
{
    trimEnd = juce::jlimit(trimStart + 1, (juce::int64)audioBuffer.getNumSamples(), sample);
}

juce::int64 AudioClip::getTrimmedDurationInSamples() const
{
    return trimEnd - trimStart;
}

//==============================================================================
void AudioClip::setPlaybackRate(double rate)
{
    playbackRate = juce::jlimit(0.25, 4.0, rate);
}

//==============================================================================
juce::var AudioClip::toVar() const
{
    auto* obj = new juce::DynamicObject();

    obj->setProperty("id", id);
    obj->setProperty("name", name);
    obj->setProperty("startBeat", startBeat);
    obj->setProperty("filePath", filePath);
    obj->setProperty("gain", gain);
    obj->setProperty("fadeInSamples", fadeInSamples);
    obj->setProperty("fadeOutSamples", fadeOutSamples);
    obj->setProperty("trimStart", trimStart);
    obj->setProperty("trimEnd", trimEnd);
    obj->setProperty("playbackRate", playbackRate);
    obj->setProperty("sampleRate", fileSampleRate);

    return juce::var(obj);
}

std::unique_ptr<AudioClip> AudioClip::fromVar(const juce::var& var)
{
    if (!var.isObject())
        return nullptr;

    auto clip = std::make_unique<AudioClip>();

    if (var.hasProperty("id"))
        clip->id = var["id"].toString();

    if (var.hasProperty("name"))
        clip->name = var["name"].toString();

    if (var.hasProperty("startBeat"))
        clip->startBeat = var["startBeat"];

    if (var.hasProperty("filePath"))
        clip->filePath = var["filePath"].toString();

    if (var.hasProperty("gain"))
        clip->gain = static_cast<float>(var["gain"]);

    if (var.hasProperty("fadeInSamples"))
        clip->fadeInSamples = static_cast<juce::int64>((int)var["fadeInSamples"]);

    if (var.hasProperty("fadeOutSamples"))
        clip->fadeOutSamples = static_cast<juce::int64>((int)var["fadeOutSamples"]);

    if (var.hasProperty("trimStart"))
        clip->trimStart = static_cast<juce::int64>((int)var["trimStart"]);

    if (var.hasProperty("trimEnd"))
        clip->trimEnd = static_cast<juce::int64>((int)var["trimEnd"]);

    if (var.hasProperty("playbackRate"))
        clip->playbackRate = var["playbackRate"];

    if (var.hasProperty("sampleRate"))
        clip->fileSampleRate = var["sampleRate"];

    return clip;
}
