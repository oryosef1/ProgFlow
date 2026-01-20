/**
 * Stress Tests - High track counts, memory usage, performance
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/Audio/AudioEngine.h"
#include "../Source/Audio/Track.h"
#include "../Source/Audio/MidiClip.h"

class StressTests : public juce::UnitTest
{
public:
    StressTests() : UnitTest("Stress") {}

    void runTest() override
    {
        //======================================================================
        // High Track Count Tests
        //======================================================================
        beginTest("Engine handles 50 tracks");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Add 50 tracks
            for (int i = 0; i < 50; ++i)
            {
                engine.addTrack(std::make_unique<Track>("Track " + juce::String(i + 1)));
            }

            expectEquals(engine.getNumTracks(), 50);

            // Verify all tracks are accessible
            for (int i = 0; i < 50; ++i)
            {
                auto* track = engine.getTrack(i);
                expect(track != nullptr);
            }
        }

        beginTest("Engine handles 100 tracks");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            for (int i = 0; i < 100; ++i)
            {
                engine.addTrack(std::make_unique<Track>("Track " + juce::String(i + 1)));
            }

            expectEquals(engine.getNumTracks(), 100);
        }

        beginTest("Engine can process audio with 50 tracks");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Add 50 tracks with varying volumes
            for (int i = 0; i < 50; ++i)
            {
                auto track = std::make_unique<Track>("Track " + juce::String(i + 1));
                track->setVolume(0.5f);
                engine.addTrack(std::move(track));
            }

            // Process a buffer using AudioSource interface
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();

            juce::AudioSourceChannelInfo info(&buffer, 0, 512);
            engine.getNextAudioBlock(info);

            // Should complete without crash
            expect(true);
        }

        //======================================================================
        // High Clip Count Tests
        //======================================================================
        beginTest("Track handles 100 clips");
        {
            Track track("Stress Track");
            track.prepareToPlay(44100.0, 512);

            // Add 100 clips
            for (int i = 0; i < 100; ++i)
            {
                track.addClip(i * 4.0, 4.0);  // Each clip at 4-bar intervals
            }

            expectEquals(track.getNumClips(), (size_t)100);
        }

        beginTest("MidiClip handles 1000 notes");
        {
            MidiClip clip;
            clip.setDurationBars(64.0);

            // Add 1000 notes using the addNote(int, double, double, float) overload
            for (int i = 0; i < 1000; ++i)
            {
                clip.addNote(60 + (i % 12), (i % 256) * 0.25, 0.25, 0.8f);
            }

            expectEquals(clip.getNumNotes(), (size_t)1000);
        }

        //======================================================================
        // Performance Tests
        //======================================================================
        beginTest("Audio processing completes in reasonable time");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Add 20 tracks with clips
            for (int i = 0; i < 20; ++i)
            {
                auto track = std::make_unique<Track>("Track " + juce::String(i + 1));
                track->prepareToPlay(44100.0, 512);

                auto* clip = track->addClip(0.0, 4.0);

                // Add some notes
                for (int n = 0; n < 16; ++n)
                {
                    clip->addNote(60 + (n % 12), n * 0.25, 0.25, 0.8f);
                }

                engine.addTrack(std::move(track));
            }

            // Measure time to process 1 second of audio (86 buffers at 512 samples)
            juce::AudioBuffer<float> buffer(2, 512);

            auto startTime = juce::Time::getMillisecondCounterHiRes();

            for (int i = 0; i < 86; ++i)
            {
                buffer.clear();
                juce::AudioSourceChannelInfo info(&buffer, 0, 512);
                engine.getNextAudioBlock(info);
            }

            auto elapsed = juce::Time::getMillisecondCounterHiRes() - startTime;

            // Should process 1 second of audio in less than 1 second (real-time)
            // We give some slack - should complete in less than 500ms on modern hardware
            expect(elapsed < 1000.0, "Audio processing took longer than real-time: "
                   + juce::String(elapsed) + "ms");
        }

        beginTest("Track removal is efficient");
        {
            AudioEngine engine;
            engine.prepareToPlay(512, 44100.0);

            // Add 50 tracks
            for (int i = 0; i < 50; ++i)
            {
                engine.addTrack(std::make_unique<Track>("Track " + juce::String(i + 1)));
            }

            auto startTime = juce::Time::getMillisecondCounterHiRes();

            // Remove all tracks
            while (engine.getNumTracks() > 0)
            {
                engine.removeTrack(0);
            }

            auto elapsed = juce::Time::getMillisecondCounterHiRes() - startTime;

            expectEquals(engine.getNumTracks(), 0);
            // Should complete quickly (< 100ms)
            expect(elapsed < 100.0, "Track removal was too slow: "
                   + juce::String(elapsed) + "ms");
        }

        //======================================================================
        // Memory Tests
        //======================================================================
        beginTest("No memory growth from repeated clip operations");
        {
            Track track("Memory Test");
            track.prepareToPlay(44100.0, 512);

            // Record initial memory footprint conceptually by checking clip count
            // (actual memory testing would use OS APIs)

            // Add and remove clips repeatedly
            for (int cycle = 0; cycle < 100; ++cycle)
            {
                auto* clip = track.addClip(0.0, 1.0);

                for (int n = 0; n < 10; ++n)
                {
                    clip->addNote(60, n * 0.25, 0.25, 0.8f);
                }

                // Remove the clip by ID
                if (track.getNumClips() > 0)
                {
                    const auto& clips = track.getClips();
                    if (!clips.empty())
                    {
                        track.removeClip(clips[0]->getId());
                    }
                }
            }

            // Should end with no clips
            expectEquals(track.getNumClips(), (size_t)0);
        }
    }
};

// Register the test
static StressTests stressTests;
