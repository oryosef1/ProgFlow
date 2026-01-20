/**
 * AudioClip Unit Tests
 *
 * Tests for audio file playback and manipulation.
 * Following TDD: tests written BEFORE implementation.
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/Audio/AudioClip.h"

class AudioClipTests : public juce::UnitTest
{
public:
    AudioClipTests() : UnitTest("AudioClip") {}

    void runTest() override
    {
        //======================================================================
        // Construction and Properties
        //======================================================================
        beginTest("AudioClip can be constructed");
        {
            AudioClip clip;
            expect(true);
        }

        beginTest("Empty clip has zero duration");
        {
            AudioClip clip;
            expectEquals(clip.getDurationInSamples(), (juce::int64)0);
            expectEquals(clip.getDurationInSeconds(), 0.0);
        }

        beginTest("Clip has start position in beats");
        {
            AudioClip clip;
            clip.setStartBeat(4.0);
            expectEquals(clip.getStartBeat(), 4.0);
        }

        beginTest("Clip has name");
        {
            AudioClip clip;
            clip.setName("My Audio");
            expectEquals(clip.getName(), juce::String("My Audio"));
        }

        //======================================================================
        // Audio Buffer Management
        //======================================================================
        beginTest("Can set audio buffer");
        {
            AudioClip clip;

            // Create a test buffer
            juce::AudioBuffer<float> buffer(2, 44100); // 1 second stereo
            buffer.clear();

            clip.setAudioBuffer(buffer, 44100.0);

            expectEquals(clip.getNumChannels(), 2);
            expectEquals(clip.getDurationInSamples(), (juce::int64)44100);
            expectEquals(clip.getSampleRate(), 44100.0);
        }

        beginTest("Duration in seconds calculated correctly");
        {
            AudioClip clip;
            juce::AudioBuffer<float> buffer(2, 88200); // 2 seconds at 44100
            clip.setAudioBuffer(buffer, 44100.0);

            expectWithinAbsoluteError(clip.getDurationInSeconds(), 2.0, 0.001);
        }

        //======================================================================
        // Playback
        //======================================================================
        beginTest("Can get sample at position");
        {
            AudioClip clip;

            // Create buffer with known values
            juce::AudioBuffer<float> buffer(1, 100);
            for (int i = 0; i < 100; ++i)
                buffer.setSample(0, i, static_cast<float>(i) / 100.0f);

            clip.setAudioBuffer(buffer, 44100.0);

            // Sample at position 50 should be 0.5
            expectWithinAbsoluteError(clip.getSample(0, 50), 0.5f, 0.01f);
        }

        beginTest("Returns zero for out of bounds sample");
        {
            AudioClip clip;
            juce::AudioBuffer<float> buffer(1, 100);
            buffer.clear();
            clip.setAudioBuffer(buffer, 44100.0);

            // Out of bounds should return 0
            expectEquals(clip.getSample(0, -10), 0.0f);
            expectEquals(clip.getSample(0, 200), 0.0f);
        }

        //======================================================================
        // Gain and Fades
        //======================================================================
        beginTest("Default gain is 1.0");
        {
            AudioClip clip;
            expectEquals(clip.getGain(), 1.0f);
        }

        beginTest("Can set gain");
        {
            AudioClip clip;
            clip.setGain(0.5f);
            expectEquals(clip.getGain(), 0.5f);
        }

        beginTest("Gain clamps to valid range");
        {
            AudioClip clip;
            clip.setGain(-1.0f);
            expect(clip.getGain() >= 0.0f);

            clip.setGain(10.0f);
            expect(clip.getGain() <= 4.0f); // Max +12dB
        }

        beginTest("Can set fade in length");
        {
            AudioClip clip;
            clip.setFadeInSamples(1000);
            expectEquals(clip.getFadeInSamples(), (juce::int64)1000);
        }

        beginTest("Can set fade out length");
        {
            AudioClip clip;
            clip.setFadeOutSamples(2000);
            expectEquals(clip.getFadeOutSamples(), (juce::int64)2000);
        }

        //======================================================================
        // Trim Points (Non-destructive editing)
        //======================================================================
        beginTest("Default trim is full clip");
        {
            AudioClip clip;
            juce::AudioBuffer<float> buffer(1, 44100);
            clip.setAudioBuffer(buffer, 44100.0);

            expectEquals(clip.getTrimStartSample(), (juce::int64)0);
            expectEquals(clip.getTrimEndSample(), (juce::int64)44100);
        }

        beginTest("Can set trim start");
        {
            AudioClip clip;
            juce::AudioBuffer<float> buffer(1, 44100);
            clip.setAudioBuffer(buffer, 44100.0);

            clip.setTrimStartSample(1000);
            expectEquals(clip.getTrimStartSample(), (juce::int64)1000);
        }

        beginTest("Can set trim end");
        {
            AudioClip clip;
            juce::AudioBuffer<float> buffer(1, 44100);
            clip.setAudioBuffer(buffer, 44100.0);

            clip.setTrimEndSample(40000);
            expectEquals(clip.getTrimEndSample(), (juce::int64)40000);
        }

        beginTest("Trimmed duration reflects trim points");
        {
            AudioClip clip;
            juce::AudioBuffer<float> buffer(1, 44100);
            clip.setAudioBuffer(buffer, 44100.0);

            clip.setTrimStartSample(10000);
            clip.setTrimEndSample(30000);

            expectEquals(clip.getTrimmedDurationInSamples(), (juce::int64)20000);
        }

        //======================================================================
        // Pitch/Tempo
        //======================================================================
        beginTest("Default playback rate is 1.0");
        {
            AudioClip clip;
            expectEquals(clip.getPlaybackRate(), 1.0);
        }

        beginTest("Can set playback rate");
        {
            AudioClip clip;
            clip.setPlaybackRate(0.5); // Half speed
            expectEquals(clip.getPlaybackRate(), 0.5);
        }

        beginTest("Playback rate clamps to valid range");
        {
            AudioClip clip;
            clip.setPlaybackRate(0.1);
            expect(clip.getPlaybackRate() >= 0.25);

            clip.setPlaybackRate(10.0);
            expect(clip.getPlaybackRate() <= 4.0);
        }

        //======================================================================
        // Serialization
        //======================================================================
        beginTest("Can serialize to DynamicObject");
        {
            AudioClip clip;
            clip.setName("Test Clip");
            clip.setStartBeat(8.0);
            clip.setGain(0.75f);

            auto var = clip.toVar();
            expect(var.isObject());
        }

        beginTest("Can deserialize from DynamicObject");
        {
            AudioClip original;
            original.setName("Serialized");
            original.setStartBeat(4.0);
            original.setGain(0.8f);

            auto var = original.toVar();
            auto restored = AudioClip::fromVar(var);

            expect(restored != nullptr);
            expectEquals(restored->getName(), juce::String("Serialized"));
            expectEquals(restored->getStartBeat(), 4.0);
            expectWithinAbsoluteError(restored->getGain(), 0.8f, 0.001f);
        }
    }
};

// Register the test
static AudioClipTests audioClipTests;
