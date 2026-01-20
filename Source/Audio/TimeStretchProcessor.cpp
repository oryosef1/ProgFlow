#include "TimeStretchProcessor.h"
#include <rubberband/RubberBandStretcher.h>
#include <cmath>

TimeStretchProcessor::TimeStretchProcessor() = default;

TimeStretchProcessor::~TimeStretchProcessor() = default;

void TimeStretchProcessor::prepare(double newSampleRate, int channels, int blockSize)
{
    sampleRate = newSampleRate;
    numChannels = channels;
    maxBlockSize = blockSize;

    createStretcher();
}

void TimeStretchProcessor::reset()
{
    if (stretcher)
    {
        stretcher->reset();
    }
    needsReset = false;
}

void TimeStretchProcessor::createStretcher()
{
    using namespace RubberBand;

    // Configure options for real-time or offline processing
    RubberBandStretcher::Options options =
        RubberBandStretcher::OptionProcessRealTime |
        RubberBandStretcher::OptionStretchElastic |
        RubberBandStretcher::OptionTransientsCrisp |
        RubberBandStretcher::OptionDetectorCompound |
        RubberBandStretcher::OptionPhaseLaminar |
        RubberBandStretcher::OptionThreadingNever |
        RubberBandStretcher::OptionWindowStandard |
        RubberBandStretcher::OptionSmoothingOff |
        RubberBandStretcher::OptionChannelsApart;

    if (formantPreservation)
    {
        options |= RubberBandStretcher::OptionFormantPreserved;
    }

    stretcher = std::make_unique<RubberBandStretcher>(
        static_cast<size_t>(sampleRate),
        static_cast<size_t>(numChannels),
        options
    );

    stretcher->setTimeRatio(timeRatio);

    if (std::abs(pitchSemitones) > 0.001)
    {
        double pitchScale = std::pow(2.0, pitchSemitones / 12.0);
        stretcher->setPitchScale(pitchScale);
    }

    stretcher->setMaxProcessSize(static_cast<size_t>(maxBlockSize));
}

void TimeStretchProcessor::setTimeRatio(double ratio)
{
    timeRatio = juce::jlimit(0.1, 10.0, ratio);

    if (stretcher)
    {
        stretcher->setTimeRatio(timeRatio);
    }
}

void TimeStretchProcessor::setPitchSemitones(double semitones)
{
    pitchSemitones = juce::jlimit(-24.0, 24.0, semitones);

    if (stretcher)
    {
        double pitchScale = std::pow(2.0, pitchSemitones / 12.0);
        stretcher->setPitchScale(pitchScale);
    }
}

void TimeStretchProcessor::setFormantPreservation(bool preserve)
{
    if (formantPreservation != preserve)
    {
        formantPreservation = preserve;
        // Need to recreate stretcher to change formant option
        if (stretcher)
        {
            createStretcher();
        }
    }
}

int TimeStretchProcessor::process(const juce::AudioBuffer<float>& inputBuffer,
                                   juce::AudioBuffer<float>& outputBuffer)
{
    if (!stretcher || inputBuffer.getNumSamples() == 0)
        return 0;

    const int numSamples = inputBuffer.getNumSamples();
    const int channels = std::min(inputBuffer.getNumChannels(), numChannels);

    // Prepare input pointers
    std::vector<const float*> inputPtrs(static_cast<size_t>(channels));
    for (int ch = 0; ch < channels; ++ch)
    {
        inputPtrs[static_cast<size_t>(ch)] = inputBuffer.getReadPointer(ch);
    }

    // Feed input to stretcher
    stretcher->process(inputPtrs.data(), static_cast<size_t>(numSamples), false);

    // Get available output
    int available = static_cast<int>(stretcher->available());
    if (available <= 0)
        return 0;

    // Ensure output buffer is large enough
    if (outputBuffer.getNumSamples() < available)
    {
        outputBuffer.setSize(channels, available, false, false, true);
    }

    // Prepare output pointers
    std::vector<float*> outputPtrs(static_cast<size_t>(channels));
    for (int ch = 0; ch < channels; ++ch)
    {
        outputPtrs[static_cast<size_t>(ch)] = outputBuffer.getWritePointer(ch);
    }

    // Retrieve output
    int retrieved = static_cast<int>(stretcher->retrieve(outputPtrs.data(),
                                                          static_cast<size_t>(available)));

    return retrieved;
}

void TimeStretchProcessor::processOffline(const juce::AudioBuffer<float>& input,
                                           juce::AudioBuffer<float>& output)
{
    if (!stretcher || input.getNumSamples() == 0)
    {
        output.setSize(input.getNumChannels(), 0);
        return;
    }

    const int inputSamples = input.getNumSamples();
    const int channels = std::min(input.getNumChannels(), numChannels);
    const int expectedOutput = calculateOutputLength(inputSamples, timeRatio);

    // Pre-allocate output with some extra room
    output.setSize(channels, expectedOutput + 1024);
    output.clear();

    // Reset stretcher for offline processing
    stretcher->reset();

    // Study the input first (for better quality)
    std::vector<const float*> inputPtrs(static_cast<size_t>(channels));
    for (int ch = 0; ch < channels; ++ch)
    {
        inputPtrs[static_cast<size_t>(ch)] = input.getReadPointer(ch);
    }

    stretcher->study(inputPtrs.data(), static_cast<size_t>(inputSamples), true);

    // Process in chunks
    const int chunkSize = 4096;
    int inputPos = 0;
    int outputPos = 0;

    std::vector<float*> outputPtrs(static_cast<size_t>(channels));

    while (inputPos < inputSamples)
    {
        const int remaining = inputSamples - inputPos;
        const int toProcess = std::min(remaining, chunkSize);
        const bool isFinal = (inputPos + toProcess >= inputSamples);

        // Update input pointers
        for (int ch = 0; ch < channels; ++ch)
        {
            inputPtrs[static_cast<size_t>(ch)] = input.getReadPointer(ch, inputPos);
        }

        stretcher->process(inputPtrs.data(), static_cast<size_t>(toProcess), isFinal);
        inputPos += toProcess;

        // Retrieve available output
        while (stretcher->available() > 0)
        {
            int available = static_cast<int>(stretcher->available());

            // Ensure we have room
            if (outputPos + available > output.getNumSamples())
            {
                output.setSize(channels, outputPos + available + 1024, true, true, true);
            }

            for (int ch = 0; ch < channels; ++ch)
            {
                outputPtrs[static_cast<size_t>(ch)] = output.getWritePointer(ch, outputPos);
            }

            int retrieved = static_cast<int>(stretcher->retrieve(outputPtrs.data(),
                                                                  static_cast<size_t>(available)));
            outputPos += retrieved;
        }
    }

    // Trim output to actual size
    output.setSize(channels, outputPos, true, true, true);
}

int TimeStretchProcessor::getLatency() const
{
    if (stretcher)
    {
        return static_cast<int>(stretcher->getLatency());
    }
    return 0;
}

int TimeStretchProcessor::calculateOutputLength(int inputLength, double ratio)
{
    return static_cast<int>(std::ceil(inputLength * ratio));
}

double TimeStretchProcessor::bpmToTimeRatio(double originalBpm, double targetBpm)
{
    if (targetBpm <= 0.0 || originalBpm <= 0.0)
        return 1.0;

    // To slow down (lower BPM), we need ratio > 1 (stretch)
    // To speed up (higher BPM), we need ratio < 1 (compress)
    return originalBpm / targetBpm;
}
