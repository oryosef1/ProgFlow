#include "AudioEngine.h"
#include "../Utils/PerformanceProfiler.h"
#include "../Utils/SIMDUtils.h"

AudioEngine::AudioEngine()
{
    DBG("AudioEngine created");

    // Initialize effect chain with default effects
    effectChain.addEffect(std::make_unique<ChorusEffect>());
    effectChain.addEffect(std::make_unique<DelayEffect>());
    effectChain.addEffect(std::make_unique<ReverbEffect>());

    // Set up default effect parameters
    if (auto* chorus = effectChain.getEffect(0))
    {
        chorus->setWetDry(0.0f); // Start bypassed
    }
    if (auto* delay = effectChain.getEffect(1))
    {
        delay->setWetDry(0.0f); // Start bypassed
    }
    if (auto* reverb = effectChain.getEffect(2))
    {
        reverb->setWetDry(0.2f); // Small amount of reverb
        reverb->setParameter("roomSize", 0.3f);
    }
}

AudioEngine::~AudioEngine()
{
    DBG("AudioEngine destroyed");
}

//==============================================================================
// AudioSource interface
//==============================================================================

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    sampleRate = newSampleRate;
    samplesPerBlock = samplesPerBlockExpected;

    DBG("AudioEngine::prepareToPlay - Sample rate: " << sampleRate << ", Block size: " << samplesPerBlock);

    // Prepare synth
    analogSynth.prepareToPlay(sampleRate, samplesPerBlock);

    // Prepare effect chain
    effectChain.prepareToPlay(sampleRate, samplesPerBlock);

    // Prepare master chain
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    masterChain.prepare(spec);

    // Configure high-pass filter (30Hz to remove subsonic rumble)
    auto& hpFilter = masterChain.get<0>();
    hpFilter.coefficients = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 30.0f);

    // Configure master compressor (gentle glue compression)
    auto& compressor = masterChain.get<1>();
    compressor.setThreshold(-24.0f);   // dB
    compressor.setRatio(3.0f);         // 3:1 ratio
    compressor.setAttack(10.0f);       // ms
    compressor.setRelease(100.0f);     // ms

    // Configure master limiter
    auto& limiter = masterChain.get<2>();
    limiter.setThreshold(-0.3f);       // dB (prevents clipping)
    limiter.setRelease(100.0f);        // ms

    // Prepare all tracks
    juce::ScopedLock sl(trackLock);
    for (auto& track : tracks)
    {
        track->prepareToPlay(sampleRate, samplesPerBlock);
    }
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    // Early return if not yet initialized (prepareToPlay not called yet)
    if (sampleRate <= 0.0 || samplesPerBlock <= 0)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    PROFILE_SCOPE("AudioEngine::getNextAudioBlock");

    // Clear the buffer first
    bufferToFill.clearActiveBufferRegion();

    auto* buffer = bufferToFill.buffer;
    auto numSamples = bufferToFill.numSamples;

    // Test tone for initial testing
    if (testToneEnabled.load())
    {
        processTestTone(*buffer);
    }

    // Calculate beat range for this block (used for clip scheduling)
    const double bpm = currentBpm.load();
    const double beatsPerSample = (bpm / 60.0) / sampleRate;
    const double blockStartBeat = positionInBeats.load();
    const double blockEndBeat = blockStartBeat + (numSamples * beatsPerSample);

    // Schedule MIDI from clips to each track's synth when playing
    if (playing.load())
    {
        scheduleClipMidiToTracks(blockStartBeat, blockEndBeat, numSamples);
    }

    // Process keyboard input through the selected track's synth
    {
        juce::ScopedLock sl(midiLock);
        if (!midiBuffer.isEmpty())
        {
            // Route keyboard MIDI to selected track's synth
            juce::ScopedLock trackLk(trackLock);
            int trackIdx = keyboardTrackIndex.load();
            if (trackIdx >= 0 && trackIdx < static_cast<int>(tracks.size()) && tracks[trackIdx]->getSynth())
            {
                tracks[trackIdx]->getSynth()->processBlock(*buffer, midiBuffer);
            }
            else if (!tracks.empty() && tracks[0]->getSynth())
            {
                // Fallback to first track
                tracks[0]->getSynth()->processBlock(*buffer, midiBuffer);
            }
            else
            {
                // Fallback to global synth
                analogSynth.processBlock(*buffer, midiBuffer);
            }
            midiBuffer.clear();
        }
    }

    // Process all tracks - each track renders its synth to a temp buffer and mixes in
    {
        PROFILE_SCOPE("AudioEngine::ProcessTracks");
        juce::ScopedLock sl(trackLock);

        // Create a temp buffer for each track
        juce::AudioBuffer<float> trackBuffer(buffer->getNumChannels(), numSamples);

        for (auto& track : tracks)
        {
            if (!track || track->isMuted()) continue;

            // Clear track buffer
            trackBuffer.clear();

            // Track renders its synth into trackBuffer
            track->processBlock(trackBuffer, numSamples, positionInBeats.load(), currentBpm.load());

            // Mix track into main buffer
            for (int ch = 0; ch < buffer->getNumChannels(); ++ch)
            {
                buffer->addFrom(ch, 0, trackBuffer, ch, 0, numSamples);
            }
        }
    }

    // Process through effect chain
    {
        PROFILE_SCOPE("AudioEngine::EffectChain");
        effectChain.processBlock(*buffer);
    }

    // Process metronome (must be before advancing position for accurate timing)
    if (playing.load() || inCountIn.load())
    {
        processMetronome(*buffer, blockStartBeat, blockEndBeat);
    }

    // Advance transport position when playing (but not during count-in)
    if (playing.load() && !inCountIn.load())
    {
        advancePosition(numSamples);
    }

    // Apply master volume
    {
        float volume = masterVolumeLevel.load();
        if (volume != 1.0f)
        {
            buffer->applyGain(volume);
        }
    }

    // Apply master chain (HP filter, compressor, limiter)
    {
        PROFILE_SCOPE("AudioEngine::MasterChain");
        juce::dsp::AudioBlock<float> block(*buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        masterChain.process(context);
    }

    // Update meters
    {
        PROFILE_SCOPE("AudioEngine::Metering");
        updateMeters(*buffer);
    }
}

void AudioEngine::releaseResources()
{
    DBG("AudioEngine::releaseResources");

    masterChain.reset();
    analogSynth.releaseResources();
    effectChain.releaseResources();

    juce::ScopedLock sl(trackLock);
    for (auto& track : tracks)
    {
        track->releaseResources();
    }
}

//==============================================================================
// Synth control
//==============================================================================

void AudioEngine::synthNoteOn(int midiNote, float velocity)
{
    juce::ScopedLock sl(midiLock);
    auto msg = juce::MidiMessage::noteOn(1, midiNote, velocity);
    midiBuffer.addEvent(msg, 0);
}

void AudioEngine::synthNoteOff(int midiNote)
{
    juce::ScopedLock sl(midiLock);
    auto msg = juce::MidiMessage::noteOff(1, midiNote);
    midiBuffer.addEvent(msg, 0);
}

void AudioEngine::synthAllNotesOff()
{
    juce::ScopedLock sl(midiLock);
    auto msg = juce::MidiMessage::allNotesOff(1);
    midiBuffer.addEvent(msg, 0);
}

//==============================================================================
// Transport control
//==============================================================================

void AudioEngine::play()
{
    playing.store(true);
    DBG("AudioEngine: Play");
}

void AudioEngine::stop()
{
    playing.store(false);
    positionInBeats.store(0.0);
    positionInSamples.store(0.0);

    // Kill all notes on all tracks to prevent stuck notes
    {
        juce::ScopedLock sl(trackLock);
        for (auto& track : tracks)
        {
            track->synthAllNotesOff();
        }
    }

    // Also kill notes on the global synth
    analogSynth.allNotesOff();

    // Clear pending note-offs
    trackPendingNoteOffs.clear();

    DBG("AudioEngine: Stop");
}

void AudioEngine::setPlaying(bool shouldPlay)
{
    if (shouldPlay)
        play();
    else
        stop();
}

void AudioEngine::setBpm(double bpm)
{
    double clampedBpm = juce::jlimit(20.0, 300.0, bpm);
    currentBpm.store(clampedBpm);
    tempoTrack.setInitialTempo(clampedBpm);
    DBG("AudioEngine: BPM set to " << currentBpm.load());
}

double AudioEngine::getCurrentTempo() const
{
    return tempoTrack.getTempoAtBeat(positionInBeats.load());
}

TimeSignatureEvent AudioEngine::getCurrentTimeSignature() const
{
    double bars = timeSignatureTrack.beatsToBar(positionInBeats.load());
    return timeSignatureTrack.getTimeSignatureAtBar(bars);
}

double AudioEngine::getPositionInSeconds() const
{
    return positionInSamples.load() / sampleRate;
}

//==============================================================================
// Track management
//==============================================================================

void AudioEngine::addTrack(std::unique_ptr<Track> track)
{
    juce::ScopedLock sl(trackLock);

    track->prepareToPlay(sampleRate, samplesPerBlock);
    tracks.push_back(std::move(track));

    DBG("AudioEngine: Added track. Total tracks: " << tracks.size());
}

void AudioEngine::removeTrack(int index)
{
    juce::ScopedLock sl(trackLock);

    if (index >= 0 && index < static_cast<int>(tracks.size()))
    {
        tracks.erase(tracks.begin() + index);
        DBG("AudioEngine: Removed track at index " << index);
    }
}

Track* AudioEngine::getTrack(int index)
{
    juce::ScopedLock sl(trackLock);

    if (index >= 0 && index < static_cast<int>(tracks.size()))
    {
        return tracks[static_cast<size_t>(index)].get();
    }
    return nullptr;
}

void AudioEngine::setPositionInBeats(double beats)
{
    positionInBeats.store(std::max(0.0, beats));
    // Use TempoTrack for accurate beat-to-seconds conversion
    positionInSamples.store(tempoTrack.beatsToSeconds(beats) * sampleRate);

    // Clear pending note-offs when seeking
    trackPendingNoteOffs.clear();

    // Send all notes off to prevent stuck notes
    synthAllNotesOff();
}

void AudioEngine::setPositionInBars(double bars)
{
    // Use TimeSignatureTrack for accurate bar-to-beats conversion
    double beats = timeSignatureTrack.barsToBeats(bars);
    setPositionInBeats(beats);
}

//==============================================================================
// Private helpers
//==============================================================================

void AudioEngine::processTestTone(juce::AudioBuffer<float>& buffer)
{
    const float frequency = testToneFrequency.load();
    const float amplitude = 0.2f; // -14dB to not be too loud
    const double phaseIncrement = (2.0 * juce::MathConstants<double>::pi * frequency) / sampleRate;

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float sampleValue = static_cast<float>(std::sin(testTonePhase)) * amplitude;

        leftChannel[sample] += sampleValue;
        rightChannel[sample] += sampleValue;

        testTonePhase += phaseIncrement;

        // Keep phase in bounds to avoid precision loss
        if (testTonePhase >= 2.0 * juce::MathConstants<double>::pi)
            testTonePhase -= 2.0 * juce::MathConstants<double>::pi;
    }
}

void AudioEngine::updateMeters(const juce::AudioBuffer<float>& buffer)
{
    // Calculate RMS levels using SIMD-optimized function
    const auto* leftChannel = buffer.getReadPointer(0);
    const auto* rightChannel = buffer.getReadPointer(1);
    const int numSamples = buffer.getNumSamples();

    float rmsL, rmsR;
    SIMDUtils::calculateStereoRMS(leftChannel, rightChannel, numSamples, rmsL, rmsR);

    // Smooth the meter values (simple exponential smoothing)
    const float smoothing = 0.8f;
    masterLevelL.store(masterLevelL.load() * smoothing + rmsL * (1.0f - smoothing));
    masterLevelR.store(masterLevelR.load() * smoothing + rmsR * (1.0f - smoothing));
}

void AudioEngine::advancePosition(int numSamples)
{
    // Get current tempo from the tempo track (supports tempo automation)
    double currentBeat = positionInBeats.load();
    double bpm = tempoTrack.getTempoAtBeat(currentBeat);

    // Also update the atomic BPM value for getters
    currentBpm.store(bpm);

    // Calculate beats per sample
    const double beatsPerSecond = bpm / 60.0;
    const double beatsPerSample = beatsPerSecond / sampleRate;

    // Advance position
    double newPositionInSamples = positionInSamples.load() + numSamples;
    double newPositionInBeats = currentBeat + (numSamples * beatsPerSample);

    // Handle looping
    if (loopEnabled.load())
    {
        double loopEnd = loopEndBeat.load();
        double loopStart = loopStartBeat.load();

        if (newPositionInBeats >= loopEnd && loopEnd > loopStart)
        {
            // Loop back to start
            double loopLength = loopEnd - loopStart;
            newPositionInBeats = loopStart + std::fmod(newPositionInBeats - loopStart, loopLength);

            // Recalculate sample position based on new beat position
            newPositionInSamples = (newPositionInBeats / beatsPerSecond) * sampleRate;
        }
    }

    positionInSamples.store(newPositionInSamples);
    positionInBeats.store(newPositionInBeats);
}

void AudioEngine::setLoopRange(double startBeat, double endBeat)
{
    if (startBeat < endBeat)
    {
        loopStartBeat.store(startBeat);
        loopEndBeat.store(endBeat);
    }
}

//==============================================================================
// Clip MIDI Scheduling
//==============================================================================

void AudioEngine::scheduleClipMidiToTracks(double blockStartBeat, double blockEndBeat, int numSamples)
{
    juce::ScopedLock sl(trackLock);

    // Process pending note-offs first
    auto it = trackPendingNoteOffs.begin();
    while (it != trackPendingNoteOffs.end())
    {
        if (it->endBeat <= blockEndBeat && it->track)
        {
            it->track->synthNoteOff(it->midiNote);
            it = trackPendingNoteOffs.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Process each track's clips
    for (auto& track : tracks)
    {
        if (track->isMuted()) continue;

        // Convert beats to bars for the clip query (using TimeSignatureTrack for variable meters)
        double startBar = timeSignatureTrack.beatsToBar(blockStartBeat);
        double endBar = timeSignatureTrack.beatsToBar(blockEndBeat);

        std::vector<MidiClip*> activeClips;
        track->getClipsInRange(startBar, endBar, activeClips);

        // Process each clip for this track
        for (auto* clip : activeClips)
        {
            // Convert block range to clip-relative beats
            double clipStartBeat = clip->getStartBeat();
            double localStartBeat = blockStartBeat - clipStartBeat;
            double localEndBeat = blockEndBeat - clipStartBeat;

            // Get notes that start within this block
            std::vector<const Note*> notes;
            clip->getNotesInRange(localStartBeat, localEndBeat, notes);

            for (const auto* note : notes)
            {
                double noteAbsoluteStartBeat = clipStartBeat + note->startBeat;
                double noteAbsoluteEndBeat = noteAbsoluteStartBeat + note->durationBeats;

                // Send note-on to this track's synth
                track->synthNoteOn(note->midiNote, note->velocity);

                // Check if note-off is within this block
                if (noteAbsoluteEndBeat <= blockEndBeat)
                {
                    // Note ends within this block
                    track->synthNoteOff(note->midiNote);
                }
                else
                {
                    // Note extends beyond this block - add to pending
                    trackPendingNoteOffs.push_back({ track.get(), note->midiNote, noteAbsoluteEndBeat });
                }
            }
        }
    }
}

//==============================================================================
// Metronome
//==============================================================================

void AudioEngine::setMetronomeEnabled(bool enabled)
{
    metronomeEnabled.store(enabled);
    lastMetronomeBeat = -1.0;  // Reset to ensure first click happens
    DBG("Metronome " << (enabled ? "enabled" : "disabled"));
}

void AudioEngine::setMetronomeVolume(float volume)
{
    metronomeVolume.store(juce::jlimit(0.0f, 1.0f, volume));
}

void AudioEngine::setCountInBars(int bars)
{
    countInBars.store(juce::jlimit(0, 4, bars));
}

void AudioEngine::playWithCountIn()
{
    int bars = countInBars.load();
    if (bars > 0)
    {
        // Calculate beats for count-in (assuming 4/4)
        auto timeSig = getCurrentTimeSignature();
        int beatsPerBar = timeSig.numerator;
        countInBeatsRemaining.store(bars * beatsPerBar);
        inCountIn.store(true);
        lastMetronomeBeat = -1.0;

        // Start playback - metronome will play during count-in
        playing.store(true);
        DBG("AudioEngine: Play with " << bars << " bar count-in (" << countInBeatsRemaining.load() << " beats)");
    }
    else
    {
        // No count-in, just play normally
        play();
    }
}

void AudioEngine::processMetronome(juce::AudioBuffer<float>& buffer, double blockStartBeat, double blockEndBeat)
{
    if (!metronomeEnabled.load() && !inCountIn.load())
        return;

    // Get time signature to know where downbeats are
    auto timeSig = getCurrentTimeSignature();
    int beatsPerBar = timeSig.numerator;

    // Calculate beats per sample
    double bpm = currentBpm.load();
    double beatsPerSample = (bpm / 60.0) / sampleRate;
    int numSamples = buffer.getNumSamples();

    // Find clicks that occur in this block
    for (int sample = 0; sample < numSamples; ++sample)
    {
        double currentBeat = blockStartBeat + (sample * beatsPerSample);

        // Check if we've crossed a beat boundary
        double beatFloor = std::floor(currentBeat);

        if (beatFloor > lastMetronomeBeat && currentBeat >= beatFloor)
        {
            lastMetronomeBeat = beatFloor;

            // If in count-in, decrement remaining beats
            if (inCountIn.load())
            {
                int remaining = countInBeatsRemaining.load();
                if (remaining > 0)
                {
                    countInBeatsRemaining.store(remaining - 1);

                    // Generate click for count-in
                    int beatInBar = static_cast<int>(beatFloor) % beatsPerBar;
                    bool isDownbeat = (beatInBar == 0);
                    generateClick(buffer, sample, isDownbeat);

                    // Check if count-in is complete
                    if (remaining - 1 <= 0)
                    {
                        inCountIn.store(false);
                        // Reset position to start of playback
                        positionInBeats.store(0.0);
                        positionInSamples.store(0.0);
                    }
                }
            }
            else if (metronomeEnabled.load())
            {
                // Normal metronome during playback
                int beatInBar = static_cast<int>(beatFloor) % beatsPerBar;
                bool isDownbeat = (beatInBar == 0);
                generateClick(buffer, sample, isDownbeat);
            }
        }
    }
}

void AudioEngine::generateClick(juce::AudioBuffer<float>& buffer, int sampleOffset, bool isDownbeat)
{
    // Simple synthesized click - higher pitch and louder for downbeat
    float frequency = isDownbeat ? 1500.0f : 1000.0f;
    float volume = metronomeVolume.load() * (isDownbeat ? 1.0f : 0.7f);

    // Click duration in samples (short, percussive)
    int clickDuration = static_cast<int>(sampleRate * 0.015);  // 15ms
    int samplesRemaining = buffer.getNumSamples() - sampleOffset;
    int clickSamples = std::min(clickDuration, samplesRemaining);

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    double phaseIncrement = (2.0 * juce::MathConstants<double>::pi * frequency) / sampleRate;
    double phase = 0.0;

    for (int i = 0; i < clickSamples; ++i)
    {
        // Exponential decay envelope
        float envelope = std::exp(-5.0f * static_cast<float>(i) / static_cast<float>(clickDuration));

        float sample = static_cast<float>(std::sin(phase)) * volume * envelope;

        int bufferIndex = sampleOffset + i;
        leftChannel[bufferIndex] += sample;
        rightChannel[bufferIndex] += sample;

        phase += phaseIncrement;
    }
}
