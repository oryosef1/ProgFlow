#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "../Audio/AudioEngine.h"
#include <functional>

/**
 * AudioExporter - Handles offline rendering and audio file export
 *
 * Supports:
 * - WAV export (44.1kHz, 16/24/32-bit)
 * - MP3 export (via LAME encoder, if available)
 *
 * Usage:
 *   AudioExporter exporter(audioEngine);
 *   exporter.exportToWav(outputFile, 0.0, 16.0, [](float) {}, [](bool) {});
 */
class AudioExporter
{
public:
    AudioExporter(AudioEngine& engine);
    ~AudioExporter() = default;

    //==========================================================================
    // Export settings
    struct ExportSettings
    {
        int sampleRate = 44100;
        int bitDepth = 16;        // 16, 24, or 32
        int mp3Bitrate = 192;     // kbps (128, 192, 256, 320)
        double startBar = 0.0;
        double endBar = 16.0;     // Project length in bars
        bool normalizeOutput = false;
    };

    //==========================================================================
    // Export formats
    enum class Format
    {
        WAV,
        MP3
    };

    //==========================================================================
    // Progress callback: receives progress 0.0 - 1.0
    using ProgressCallback = std::function<void(float progress)>;

    // Completion callback: receives true if successful
    using CompletionCallback = std::function<void(bool success, const juce::String& errorMessage)>;

    //==========================================================================
    // Export methods (async, runs on background thread)
    void exportAsync(const juce::File& outputFile,
                     Format format,
                     const ExportSettings& settings,
                     ProgressCallback onProgress,
                     CompletionCallback onComplete);

    // Cancel ongoing export
    void cancelExport();
    bool isExporting() const { return exporting.load(); }

    //==========================================================================
    // Synchronous export (blocks until complete)
    bool exportToWav(const juce::File& outputFile,
                     const ExportSettings& settings,
                     ProgressCallback onProgress = nullptr);

    bool exportToMp3(const juce::File& outputFile,
                     const ExportSettings& settings,
                     ProgressCallback onProgress = nullptr);

    //==========================================================================
    // Utilities
    static double calculateProjectLengthBars(AudioEngine& engine);
    static juce::StringArray getSupportedFormats();
    static bool isMp3Supported();

private:
    AudioEngine& audioEngine;
    std::atomic<bool> exporting{false};
    std::atomic<bool> shouldCancel{false};

    // Perform offline render to buffer
    bool renderToBuffer(juce::AudioBuffer<float>& buffer,
                        const ExportSettings& settings,
                        ProgressCallback onProgress);

    // Write buffer to WAV file
    bool writeWavFile(const juce::File& file,
                      const juce::AudioBuffer<float>& buffer,
                      int sampleRate,
                      int bitDepth);

    // Write buffer to MP3 file (requires LAME)
    bool writeMp3File(const juce::File& file,
                      const juce::AudioBuffer<float>& buffer,
                      int sampleRate,
                      int bitrate);

    // Normalize buffer to -1.0 to 1.0
    void normalizeBuffer(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioExporter)
};
