#pragma once

#include <juce_core/juce_core.h>

/**
 * TransportEngine - Manages playback timing and position
 *
 * Provides:
 * - Play/stop/pause control
 * - BPM and time signature
 * - Position tracking (bars, beats, ticks)
 * - Loop region support
 * - Tempo changes (future)
 */
class TransportEngine
{
public:
    TransportEngine() = default;
    ~TransportEngine() = default;

    //==========================================================================
    // Transport state
    void play() { playing.store(true); }
    void stop() { playing.store(false); position.store(0.0); }
    void pause() { playing.store(false); }

    bool isPlaying() const { return playing.load(); }

    //==========================================================================
    // Position
    void setPosition(double positionInBeats) { position.store(positionInBeats); }
    double getPosition() const { return position.load(); }

    // Convert position to bars:beats:ticks format
    struct PositionInfo
    {
        int bars = 1;
        int beats = 1;
        int ticks = 0;
    };

    PositionInfo getPositionInfo() const
    {
        double pos = position.load();
        int beatsPerBar = timeSignatureNumerator.load();

        PositionInfo info;
        int totalBeats = static_cast<int>(pos);
        info.bars = (totalBeats / beatsPerBar) + 1;
        info.beats = (totalBeats % beatsPerBar) + 1;
        info.ticks = static_cast<int>((pos - totalBeats) * 960); // 960 ticks per beat (MIDI standard)

        return info;
    }

    //==========================================================================
    // Tempo
    void setBpm(double bpm) { this->bpm.store(juce::jlimit(20.0, 300.0, bpm)); }
    double getBpm() const { return bpm.load(); }

    //==========================================================================
    // Time signature
    void setTimeSignature(int numerator, int denominator)
    {
        timeSignatureNumerator.store(numerator);
        timeSignatureDenominator.store(denominator);
    }

    int getTimeSignatureNumerator() const { return timeSignatureNumerator.load(); }
    int getTimeSignatureDenominator() const { return timeSignatureDenominator.load(); }

    //==========================================================================
    // Loop
    void setLoopEnabled(bool enabled) { loopEnabled.store(enabled); }
    bool isLoopEnabled() const { return loopEnabled.load(); }

    void setLoopRegion(double startBeat, double endBeat)
    {
        loopStart.store(startBeat);
        loopEnd.store(endBeat);
    }

    double getLoopStart() const { return loopStart.load(); }
    double getLoopEnd() const { return loopEnd.load(); }

    //==========================================================================
    // Called from audio thread
    void advancePosition(int numSamples, double sampleRate)
    {
        if (!playing.load())
            return;

        double beatsPerSecond = bpm.load() / 60.0;
        double beatsPerSample = beatsPerSecond / sampleRate;
        double advance = numSamples * beatsPerSample;

        double newPosition = position.load() + advance;

        // Handle looping
        if (loopEnabled.load())
        {
            double loopEndPos = loopEnd.load();
            double loopStartPos = loopStart.load();

            if (newPosition >= loopEndPos)
            {
                newPosition = loopStartPos + (newPosition - loopEndPos);
            }
        }

        position.store(newPosition);
    }

private:
    std::atomic<bool> playing{false};
    std::atomic<double> position{0.0};
    std::atomic<double> bpm{120.0};
    std::atomic<int> timeSignatureNumerator{4};
    std::atomic<int> timeSignatureDenominator{4};
    std::atomic<bool> loopEnabled{false};
    std::atomic<double> loopStart{0.0};
    std::atomic<double> loopEnd{4.0};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportEngine)
};
