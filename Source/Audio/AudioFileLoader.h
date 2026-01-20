#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "AudioClip.h"
#include <memory>

/**
 * AudioFileLoader - Utility for loading audio files into AudioClip
 *
 * Supports: WAV, AIFF, FLAC, MP3, OGG
 * Features:
 * - Automatic format detection
 * - Sample rate conversion (if needed)
 * - Mono/stereo support
 */
class AudioFileLoader
{
public:
    AudioFileLoader();
    ~AudioFileLoader() = default;

    //==========================================================================
    // File Loading

    /**
     * Load an audio file into a new AudioClip
     * @param file The file to load
     * @param targetSampleRate The target sample rate for resampling (0 = keep original)
     * @return A unique_ptr to the loaded AudioClip, or nullptr on failure
     */
    std::unique_ptr<AudioClip> loadFile(const juce::File& file, double targetSampleRate = 0.0);

    /**
     * Load audio data into an existing AudioClip
     * @param file The file to load
     * @param clip The clip to load into
     * @param targetSampleRate The target sample rate for resampling (0 = keep original)
     * @return True if successful
     */
    bool loadIntoClip(const juce::File& file, AudioClip& clip, double targetSampleRate = 0.0);

    //==========================================================================
    // Supported Formats

    /**
     * Get a wildcard string for all supported formats
     * e.g., "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg"
     */
    juce::String getSupportedFormatsWildcard() const;

    /**
     * Check if a file extension is supported
     */
    bool isFormatSupported(const juce::String& extension) const;

    //==========================================================================
    // Access to format manager (for waveform thumbnail, etc.)
    juce::AudioFormatManager& getFormatManager() { return formatManager; }

private:
    juce::AudioFormatManager formatManager;

    /**
     * Resample audio buffer to target sample rate
     */
    void resample(juce::AudioBuffer<float>& buffer, double sourceSampleRate,
                  double targetSampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFileLoader)
};

/**
 * Global AudioFileLoader singleton for shared use
 */
AudioFileLoader& getAudioFileLoader();
