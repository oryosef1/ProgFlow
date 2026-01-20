#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <memory>

// Forward declaration - RubberBand will be included in .cpp
namespace RubberBand {
    class RubberBandStretcher;
}

/**
 * TimeStretchProcessor - High-quality time-stretching using RubberBand
 *
 * Allows changing tempo without affecting pitch, or pitch without affecting tempo.
 * Used for audio clips to match project tempo.
 */
class TimeStretchProcessor
{
public:
    TimeStretchProcessor();
    ~TimeStretchProcessor();

    // Non-copyable
    TimeStretchProcessor(const TimeStretchProcessor&) = delete;
    TimeStretchProcessor& operator=(const TimeStretchProcessor&) = delete;

    //==========================================================================
    // Configuration

    /**
     * Prepare the processor
     * @param sampleRate Audio sample rate
     * @param numChannels Number of audio channels (1 or 2)
     * @param maxBlockSize Maximum samples per process call
     */
    void prepare(double sampleRate, int numChannels, int maxBlockSize);

    /**
     * Reset processor state (call when seeking or changing parameters)
     */
    void reset();

    //==========================================================================
    // Time-stretch parameters

    /**
     * Set time ratio (1.0 = original speed, 2.0 = half speed/double length)
     * @param ratio Time stretch ratio (0.5 to 2.0 typical range)
     */
    void setTimeRatio(double ratio);
    double getTimeRatio() const { return timeRatio; }

    /**
     * Set pitch shift in semitones (0.0 = no shift)
     * @param semitones Pitch shift (-12 to +12 typical range)
     */
    void setPitchSemitones(double semitones);
    double getPitchSemitones() const { return pitchSemitones; }

    /**
     * Enable/disable formant preservation (better for vocals)
     */
    void setFormantPreservation(bool preserve);
    bool getFormantPreservation() const { return formantPreservation; }

    //==========================================================================
    // Processing

    /**
     * Process audio through time-stretcher
     * @param inputBuffer Source audio
     * @param outputBuffer Destination buffer (may be different size due to stretching)
     * @return Number of samples written to output
     */
    int process(const juce::AudioBuffer<float>& inputBuffer,
                juce::AudioBuffer<float>& outputBuffer);

    /**
     * Process entire audio buffer offline (for pre-rendering)
     * @param input Source audio buffer
     * @param output Will be resized to fit stretched result
     */
    void processOffline(const juce::AudioBuffer<float>& input,
                        juce::AudioBuffer<float>& output);

    /**
     * Get latency introduced by the processor
     */
    int getLatency() const;

    /**
     * Check if processor is ready
     */
    bool isReady() const { return stretcher != nullptr; }

    //==========================================================================
    // Utility

    /**
     * Calculate output length for given input length and time ratio
     */
    static int calculateOutputLength(int inputLength, double timeRatio);

    /**
     * Convert BPM change to time ratio
     * @param originalBpm Original tempo of audio
     * @param targetBpm Target tempo
     * @return Time ratio to achieve tempo change
     */
    static double bpmToTimeRatio(double originalBpm, double targetBpm);

private:
    void createStretcher();

    std::unique_ptr<RubberBand::RubberBandStretcher> stretcher;

    double sampleRate = 44100.0;
    int numChannels = 2;
    int maxBlockSize = 512;

    double timeRatio = 1.0;
    double pitchSemitones = 0.0;
    bool formantPreservation = false;

    bool needsReset = false;
};
