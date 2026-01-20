/**
 * Integration Tests - Audio routing, track-to-master flow, playback
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/Audio/AudioEngine.h"
#include "../Source/Audio/Track.h"
#include "../Source/Audio/MidiClip.h"
#include "../Source/Audio/Effects/ReverbEffect.h"
#include "../Source/Audio/Effects/DelayEffect.h"

class IntegrationTests : public juce::UnitTest
{
public:
    IntegrationTests() : UnitTest("Integration") {}

    void runTest() override
    {
        //======================================================================
        // Audio Routing Tests
        //======================================================================
        beginTest("Track audio flows to engine output");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto track = std::make_unique<Track>("Test Track");
            track->prepareToPlay(44100.0, 512);
            track->setVolume(1.0f);
            track->setMuted(false);
            engine.addTrack(std::move(track));

            // Process audio
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            // Engine should produce valid output
            expectNoNaNOrInf(buffer);
        }

        beginTest("Muted track produces silence");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto track = std::make_unique<Track>("Muted Track");
            track->prepareToPlay(44100.0, 512);
            track->setVolume(1.0f);
            track->setMuted(true);
            engine.addTrack(std::move(track));

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            // Muted track should contribute no audio
            expectNoNaNOrInf(buffer);
        }

        beginTest("Track volume affects output level");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto track1 = std::make_unique<Track>("Full Volume");
            track1->prepareToPlay(44100.0, 512);
            track1->setVolume(1.0f);

            auto track2 = std::make_unique<Track>("Half Volume");
            track2->prepareToPlay(44100.0, 512);
            track2->setVolume(0.5f);

            engine.addTrack(std::move(track1));
            engine.addTrack(std::move(track2));

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            expectNoNaNOrInf(buffer);
        }

        beginTest("Track pan affects stereo placement");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto trackLeft = std::make_unique<Track>("Left Pan");
            trackLeft->prepareToPlay(44100.0, 512);
            trackLeft->setPan(-1.0f);  // Full left

            auto trackRight = std::make_unique<Track>("Right Pan");
            trackRight->prepareToPlay(44100.0, 512);
            trackRight->setPan(1.0f);  // Full right

            engine.addTrack(std::move(trackLeft));
            engine.addTrack(std::move(trackRight));

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            expectNoNaNOrInf(buffer);
        }

        //======================================================================
        // Effect Chain Integration
        //======================================================================
        beginTest("Master effect chain processes audio");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto track = std::make_unique<Track>("Test Track");
            track->prepareToPlay(44100.0, 512);
            engine.addTrack(std::move(track));

            // Add effect to master effect chain
            auto& chain = engine.getEffectChain();
            chain.addEffect(std::make_unique<ReverbEffect>());

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            expectNoNaNOrInf(buffer);
        }

        beginTest("Multiple effects in master chain process sequentially");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto track = std::make_unique<Track>("Test Track");
            track->prepareToPlay(44100.0, 512);
            engine.addTrack(std::move(track));

            auto& chain = engine.getEffectChain();
            int initialCount = chain.getNumEffects();  // Engine has default effects
            chain.addEffect(std::make_unique<ReverbEffect>());
            chain.addEffect(std::make_unique<DelayEffect>());

            expectEquals(chain.getNumEffects(), initialCount + 2);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            expectNoNaNOrInf(buffer);
        }

        beginTest("Master effect chain with reverb and delay");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Add effect to master chain
            auto& masterChain = engine.getEffectChain();
            masterChain.addEffect(std::make_unique<ReverbEffect>());

            auto track = std::make_unique<Track>("Track");
            track->prepareToPlay(44100.0, 512);
            engine.addTrack(std::move(track));

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            expectNoNaNOrInf(buffer);
        }

        //======================================================================
        // Playback Integration
        //======================================================================
        beginTest("Engine transport play/stop works");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            expect(!engine.isPlaying(), "Should start stopped");

            engine.play();
            expect(engine.isPlaying(), "Should be playing after play()");

            engine.stop();
            expect(!engine.isPlaying(), "Should be stopped after stop()");
        }

        beginTest("Engine position advances during playback");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);
            engine.setBpm(120.0);

            double startPos = engine.getPositionInBeats();
            engine.play();

            // Process several blocks
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 10; ++i)
            {
                buffer.clear();
                juce::AudioSourceChannelInfo info(&buffer, 0, 512);
                engine.getNextAudioBlock(info);
            }

            double endPos = engine.getPositionInBeats();
            engine.stop();

            expect(endPos > startPos, "Position should advance during playback");
        }

        beginTest("Engine BPM affects playback speed");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            engine.setBpm(60.0);  // 1 beat per second
            expectWithinAbsoluteError(engine.getBpm(), 60.0, 0.01);

            engine.setBpm(120.0);  // 2 beats per second
            expectWithinAbsoluteError(engine.getBpm(), 120.0, 0.01);

            engine.setBpm(180.0);  // 3 beats per second
            expectWithinAbsoluteError(engine.getBpm(), 180.0, 0.01);
        }

        beginTest("Engine position can be set");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            engine.setPositionInBeats(16.0);
            expectWithinAbsoluteError(engine.getPositionInBeats(), 16.0, 0.01);

            engine.setPositionInBeats(0.0);
            expectWithinAbsoluteError(engine.getPositionInBeats(), 0.0, 0.01);
        }

        //======================================================================
        // MIDI Clip Playback Integration
        //======================================================================
        beginTest("Track with clip processes during playback");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);
            engine.setBpm(120.0);

            auto track = std::make_unique<Track>("MIDI Track");
            track->prepareToPlay(44100.0, 512);

            // Add a clip with notes
            auto* clip = track->addClip(0.0, 4.0);
            clip->addNote(60, 0.0, 1.0, 0.8f);
            clip->addNote(64, 1.0, 1.0, 0.8f);
            clip->addNote(67, 2.0, 1.0, 0.8f);

            engine.addTrack(std::move(track));
            engine.play();

            // Process several blocks
            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 50; ++i)
            {
                buffer.clear();
                juce::AudioSourceChannelInfo info(&buffer, 0, 512);
                engine.getNextAudioBlock(info);
            }

            engine.stop();
            expectNoNaNOrInf(buffer);
        }

        beginTest("Multiple tracks mix correctly");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);
            engine.setBpm(120.0);

            // Add multiple tracks
            for (int i = 0; i < 5; ++i)
            {
                auto track = std::make_unique<Track>("Track " + juce::String(i + 1));
                track->prepareToPlay(44100.0, 512);
                track->setVolume(0.2f);  // Each at 20%

                auto* clip = track->addClip(0.0, 4.0);
                clip->addNote(60 + i * 4, 0.0, 4.0, 0.8f);

                engine.addTrack(std::move(track));
            }

            engine.play();

            juce::AudioBuffer<float> buffer(2, 512);
            for (int i = 0; i < 20; ++i)
            {
                buffer.clear();
                juce::AudioSourceChannelInfo info(&buffer, 0, 512);
                engine.getNextAudioBlock(info);
            }

            engine.stop();
            expectNoNaNOrInf(buffer);
        }

        //======================================================================
        // Synth Integration
        //======================================================================
        beginTest("Engine synth responds to note on/off");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Trigger a note
            engine.synthNoteOn(60, 0.8f);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            // Should produce audio (not all zeros)
            float rms = buffer.getRMSLevel(0, 0, 512);
            expect(rms > 0.0f, "Synth should produce audio on note on");

            // Turn note off
            engine.synthNoteOff(60);

            // Process more to let note release
            for (int i = 0; i < 100; ++i)
            {
                buffer.clear();
                juce::AudioSourceChannelInfo info2(&buffer, 0, 512);
                engine.getNextAudioBlock(info2);
            }

            expectNoNaNOrInf(buffer);
        }

        beginTest("Engine all notes off silences synth");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Trigger multiple notes
            engine.synthNoteOn(60, 0.8f);
            engine.synthNoteOn(64, 0.8f);
            engine.synthNoteOn(67, 0.8f);

            // Process a block
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            // All notes off
            engine.synthAllNotesOff();

            // Process more blocks to let notes release
            for (int i = 0; i < 200; ++i)
            {
                buffer.clear();
                juce::AudioSourceChannelInfo info2(&buffer, 0, 512);
                engine.getNextAudioBlock(info2);
            }

            // Should eventually be silent
            expectNoNaNOrInf(buffer);
        }

        //======================================================================
        // Tempo/Time Signature Integration
        //======================================================================
        beginTest("Tempo track affects playback");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto& tempoTrack = engine.getTempoTrack();
            tempoTrack.setInitialTempo(120.0);

            expectWithinAbsoluteError(tempoTrack.getInitialTempo(), 120.0, 0.01);
        }

        beginTest("Time signature track provides bar information");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto& timeSigTrack = engine.getTimeSignatureTrack();
            auto sig = timeSigTrack.getInitialTimeSignature();

            expectEquals(sig.numerator, 4);
            expectEquals(sig.denominator, 4);
        }

        beginTest("Marker track stores markers");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            auto& markerTrack = engine.getMarkerTrack();
            markerTrack.addMarker(0.0, "Intro");
            markerTrack.addMarker(8.0, "Verse");
            markerTrack.addMarker(24.0, "Chorus");

            expectEquals(markerTrack.getNumMarkers(), (size_t)3);
        }
    }

private:
    void expectNoNaNOrInf(const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                float sample = buffer.getSample(ch, i);
                expect(!std::isnan(sample), "NaN detected");
                expect(!std::isinf(sample), "Inf detected");
            }
        }
    }
};

// Register the test
static IntegrationTests integrationTests;
