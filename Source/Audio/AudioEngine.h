#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "TransportEngine.h"
#include "Track.h"
#include "MidiClip.h"
#include "Synths/AnalogSynth.h"
#include "Effects/EffectChain.h"
#include "Effects/ReverbEffect.h"
#include "Effects/DelayEffect.h"
#include "Effects/ChorusEffect.h"
#include "TempoTrack.h"
#include "TimeSignatureTrack.h"
#include "MarkerTrack.h"
#include <vector>

/**
 * AudioEngine - The main audio processor for ProgFlow
 *
 * Responsibilities:
 * - Manages all audio processing
 * - Owns and processes all tracks
 * - Handles transport (play/stop/position)
 * - Provides master output chain (EQ, compression, limiting)
 * - Thread-safe communication with UI via lock-free queues
 */
class AudioEngine : public juce::AudioSource
{
public:
    AudioEngine();
    ~AudioEngine() override;

    //==========================================================================
    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==========================================================================
    // Transport control
    void play();
    void stop();
    void setPlaying(bool shouldPlay);
    bool isPlaying() const { return playing.load(); }

    void setBpm(double bpm);
    double getBpm() const { return currentBpm.load(); }

    double getPositionInBeats() const { return positionInBeats.load(); }
    double getPositionInSeconds() const;

    //==========================================================================
    // Track management (called from message thread)
    void addTrack(std::unique_ptr<Track> track);
    void removeTrack(int index);
    Track* getTrack(int index);
    int getNumTracks() const { return static_cast<int>(tracks.size()); }

    //==========================================================================
    // Playback position
    void setPositionInBeats(double beats);
    void setPositionInBars(double bars);

    //==========================================================================
    // Test tone (for initial testing)
    void setTestToneEnabled(bool enabled) { testToneEnabled.store(enabled); }
    bool isTestToneEnabled() const { return testToneEnabled.load(); }
    void setTestToneFrequency(float freq) { testToneFrequency.store(freq); }

    //==========================================================================
    // Synth control (for keyboard input)
    void synthNoteOn(int midiNote, float velocity);
    void synthNoteOff(int midiNote);
    void synthAllNotesOff();
    AnalogSynth& getSynth() { return analogSynth; }

    // Keyboard input track selection (which track receives keyboard MIDI)
    void setKeyboardTrackIndex(int index) { keyboardTrackIndex.store(index); }
    int getKeyboardTrackIndex() const { return keyboardTrackIndex.load(); }

    //==========================================================================
    // Effects chain
    EffectChain& getEffectChain() { return effectChain; }

    //==========================================================================
    // Metronome
    void setMetronomeEnabled(bool enabled);
    bool isMetronomeEnabled() const { return metronomeEnabled.load(); }
    void setMetronomeVolume(float volume);
    float getMetronomeVolume() const { return metronomeVolume.load(); }

    // Count-in (plays metronome clicks before starting actual playback)
    void setCountInBars(int bars);  // 0 = disabled, 1-4 = number of bars
    int getCountInBars() const { return countInBars.load(); }
    bool isInCountIn() const { return inCountIn.load(); }
    void playWithCountIn();  // Start playback with count-in

    //==========================================================================
    // Master volume
    void setMasterVolume(float volume) { this->masterVolumeLevel.store(juce::jlimit(0.0f, 2.0f, volume)); }
    float getMasterVolume() const { return this->masterVolumeLevel.load(); }

    //==========================================================================
    // Loop
    void setLoopEnabled(bool enabled) { loopEnabled.store(enabled); }
    bool isLoopEnabled() const { return loopEnabled.load(); }
    void toggleLoop() { loopEnabled.store(!loopEnabled.load()); }

    void setLoopRange(double startBeat, double endBeat);
    double getLoopStartBeat() const { return loopStartBeat.load(); }
    double getLoopEndBeat() const { return loopEndBeat.load(); }

    //==========================================================================
    // Metering (read from UI thread)
    float getMasterLevelL() const { return masterLevelL.load(); }
    float getMasterLevelR() const { return masterLevelR.load(); }

    //==========================================================================
    // Arrangement tracks
    TempoTrack& getTempoTrack() { return tempoTrack; }
    const TempoTrack& getTempoTrack() const { return tempoTrack; }

    TimeSignatureTrack& getTimeSignatureTrack() { return timeSignatureTrack; }
    const TimeSignatureTrack& getTimeSignatureTrack() const { return timeSignatureTrack; }

    MarkerTrack& getMarkerTrack() { return markerTrack; }
    const MarkerTrack& getMarkerTrack() const { return markerTrack; }

    /** Get tempo at current playhead position */
    double getCurrentTempo() const;

    /** Get time signature at current bar position */
    TimeSignatureEvent getCurrentTimeSignature() const;

private:
    // Audio settings
    double sampleRate = 44100.0;
    int samplesPerBlock = 512;

    // Transport state (atomic for thread-safe access)
    std::atomic<bool> playing{false};
    std::atomic<double> currentBpm{120.0};
    std::atomic<double> positionInBeats{0.0};
    std::atomic<double> positionInSamples{0.0};

    // Loop state
    std::atomic<bool> loopEnabled{false};
    std::atomic<double> loopStartBeat{0.0};
    std::atomic<double> loopEndBeat{16.0};  // Default 4 bars

    // Tracks
    std::vector<std::unique_ptr<Track>> tracks;
    juce::CriticalSection trackLock; // For track list modifications

    // Master output chain
    juce::dsp::ProcessorChain<
        juce::dsp::IIR::Filter<float>,    // High-pass filter (30Hz)
        juce::dsp::Compressor<float>,      // Master compressor
        juce::dsp::Limiter<float>          // Master limiter
    > masterChain;

    // Master volume (0.0 - 2.0, 1.0 = unity)
    std::atomic<float> masterVolumeLevel{1.0f};

    // Metering
    std::atomic<float> masterLevelL{0.0f};
    std::atomic<float> masterLevelR{0.0f};

    // Test tone for initial testing
    std::atomic<bool> testToneEnabled{false};
    std::atomic<float> testToneFrequency{440.0f};
    double testTonePhase = 0.0;

    // Synth
    AnalogSynth analogSynth;
    juce::MidiBuffer midiBuffer;
    juce::CriticalSection midiLock;
    std::atomic<int> keyboardTrackIndex{0};  // Which track receives keyboard MIDI

    // Effects chain (processes synth output before master chain)
    EffectChain effectChain;

    // Arrangement tracks
    TempoTrack tempoTrack;
    TimeSignatureTrack timeSignatureTrack;
    MarkerTrack markerTrack;

    // Metronome
    std::atomic<bool> metronomeEnabled{false};
    std::atomic<float> metronomeVolume{0.7f};
    std::atomic<int> countInBars{0};  // 0 = disabled
    std::atomic<bool> inCountIn{false};
    std::atomic<int> countInBeatsRemaining{0};
    double lastMetronomeBeat = -1.0;

    // Helper methods
    void processMetronome(juce::AudioBuffer<float>& buffer, double blockStartBeat, double blockEndBeat);
    void generateClick(juce::AudioBuffer<float>& buffer, int sampleOffset, bool isDownbeat);
    void processTestTone(juce::AudioBuffer<float>& buffer);
    void updateMeters(const juce::AudioBuffer<float>& buffer);
    void advancePosition(int numSamples);

    // Clip playback scheduling (per-track)
    void scheduleClipMidiToTracks(double blockStartBeat, double blockEndBeat, int numSamples);

    // Pending note-offs per track
    struct TrackPendingNoteOff
    {
        Track* track;
        int midiNote;
        double endBeat;
    };
    std::vector<TrackPendingNoteOff> trackPendingNoteOffs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};
