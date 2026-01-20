#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <memory>

/**
 * AudioClip - Represents an audio region on the timeline
 *
 * Features:
 * - Stores audio buffer with sample rate
 * - Non-destructive trim (start/end points)
 * - Gain control
 * - Fade in/out
 * - Playback rate (time stretch)
 * - Position on timeline (beats)
 */
class AudioClip
{
public:
    AudioClip();
    ~AudioClip() = default;

    //==========================================================================
    // Identity
    juce::String getId() const { return id; }

    void setName(const juce::String& newName) { name = newName; }
    juce::String getName() const { return name; }

    //==========================================================================
    // Timeline position
    void setStartBeat(double beat) { startBeat = beat; }
    double getStartBeat() const { return startBeat; }

    double getEndBeat() const;
    double getDurationInBeats(double bpm) const;

    //==========================================================================
    // Audio buffer
    void setAudioBuffer(const juce::AudioBuffer<float>& buffer, double sampleRate);
    void setAudioBuffer(juce::AudioBuffer<float>&& buffer, double sampleRate);

    const juce::AudioBuffer<float>& getAudioBuffer() const { return audioBuffer; }
    bool hasAudio() const { return audioBuffer.getNumSamples() > 0; }

    int getNumChannels() const { return audioBuffer.getNumChannels(); }
    juce::int64 getDurationInSamples() const { return audioBuffer.getNumSamples(); }
    double getDurationInSeconds() const;
    double getSampleRate() const { return fileSampleRate; }

    //==========================================================================
    // Sample access
    float getSample(int channel, juce::int64 sampleIndex) const;

    //==========================================================================
    // Gain
    void setGain(float newGain);
    float getGain() const { return gain; }

    //==========================================================================
    // Fades
    void setFadeInSamples(juce::int64 samples);
    juce::int64 getFadeInSamples() const { return fadeInSamples; }

    void setFadeOutSamples(juce::int64 samples);
    juce::int64 getFadeOutSamples() const { return fadeOutSamples; }

    //==========================================================================
    // Non-destructive trim
    void setTrimStartSample(juce::int64 sample);
    juce::int64 getTrimStartSample() const { return trimStart; }

    void setTrimEndSample(juce::int64 sample);
    juce::int64 getTrimEndSample() const { return trimEnd; }

    juce::int64 getTrimmedDurationInSamples() const;

    //==========================================================================
    // Playback rate (time stretch)
    void setPlaybackRate(double rate);
    double getPlaybackRate() const { return playbackRate; }

    //==========================================================================
    // File reference (for reload/save)
    void setFilePath(const juce::String& path) { filePath = path; }
    juce::String getFilePath() const { return filePath; }

    //==========================================================================
    // Serialization
    juce::var toVar() const;
    static std::unique_ptr<AudioClip> fromVar(const juce::var& var);

private:
    // Unique identifier
    juce::String id;

    // Display name
    juce::String name;

    // Position on timeline
    double startBeat = 0.0;

    // Audio data
    juce::AudioBuffer<float> audioBuffer;
    double fileSampleRate = 44100.0;

    // Gain (linear, 0.0 - 4.0 for up to +12dB)
    float gain = 1.0f;

    // Fades (in samples)
    juce::int64 fadeInSamples = 0;
    juce::int64 fadeOutSamples = 0;

    // Non-destructive trim points (in samples)
    juce::int64 trimStart = 0;
    juce::int64 trimEnd = 0;

    // Playback rate (0.25 - 4.0)
    double playbackRate = 1.0;

    // Original file path
    juce::String filePath;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioClip)
};
