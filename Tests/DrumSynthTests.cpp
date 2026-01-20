/**
 * DrumSynth Unit Tests
 *
 * Tests for the synthesis-based drum machine.
 * Following TDD: tests written BEFORE implementation.
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/Audio/Synths/DrumSynth.h"

class DrumSynthTests : public juce::UnitTest
{
public:
    DrumSynthTests() : UnitTest("DrumSynth") {}

    void runTest() override
    {
        //======================================================================
        // Construction and Initialization
        //======================================================================
        beginTest("DrumSynth can be constructed");
        {
            DrumSynth drums;
            expect(true);
        }

        beginTest("DrumSynth extends SynthBase");
        {
            DrumSynth drums;
            SynthBase* base = &drums;
            expect(base != nullptr);
        }

        //======================================================================
        // Drum Sounds - 16 pads standard
        //======================================================================
        beginTest("Has 16 drum pads");
        {
            DrumSynth drums;
            expectEquals(drums.getNumPads(), 16);
        }

        beginTest("Default kit is 808");
        {
            DrumSynth drums;
            expectEquals(drums.getCurrentKit(), juce::String("808"));
        }

        beginTest("Pad 0 is Kick");
        {
            DrumSynth drums;
            expectEquals(drums.getPadName(0), juce::String("Kick"));
        }

        beginTest("Pad 1 is Snare");
        {
            DrumSynth drums;
            expectEquals(drums.getPadName(1), juce::String("Snare"));
        }

        beginTest("Pad 2 is Closed Hi-Hat");
        {
            DrumSynth drums;
            expectEquals(drums.getPadName(2), juce::String("Closed HH"));
        }

        beginTest("Pad 3 is Open Hi-Hat");
        {
            DrumSynth drums;
            expectEquals(drums.getPadName(3), juce::String("Open HH"));
        }

        //======================================================================
        // MIDI Note Mapping
        //======================================================================
        beginTest("MIDI note 36 triggers Kick (pad 0)");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);
            drums.noteOn(36, 0.8f);
            expect(drums.hasActiveNotes());
        }

        beginTest("MIDI note 38 triggers Snare (pad 1)");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);
            drums.noteOn(38, 0.8f);
            expect(drums.hasActiveNotes());
        }

        beginTest("MIDI note 42 triggers Closed Hi-Hat (pad 2)");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);
            drums.noteOn(42, 0.8f);
            expect(drums.hasActiveNotes());
        }

        beginTest("MIDI note 46 triggers Open Hi-Hat (pad 3)");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);
            drums.noteOn(46, 0.8f);
            expect(drums.hasActiveNotes());
        }

        //======================================================================
        // Choke Groups
        //======================================================================
        beginTest("Closed hi-hat chokes open hi-hat");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);

            // Trigger open hi-hat
            drums.noteOn(46, 0.8f); // Open HH

            // Trigger closed hi-hat - should choke open
            drums.noteOn(42, 0.8f); // Closed HH

            // Open hi-hat should be choked (not in active notes)
            expect(!drums.isNoteActive(46));
            expect(drums.isNoteActive(42));
        }

        //======================================================================
        // Per-Pad Parameters
        //======================================================================
        beginTest("Can set pad pitch");
        {
            DrumSynth drums;
            drums.setPadParameter(0, "pitch", 1.5f); // Kick pitch +50%
            expectEquals(drums.getPadParameter(0, "pitch"), 1.5f);
        }

        beginTest("Can set pad decay");
        {
            DrumSynth drums;
            drums.setPadParameter(0, "decay", 0.3f);
            expectEquals(drums.getPadParameter(0, "decay"), 0.3f);
        }

        beginTest("Can set pad tone");
        {
            DrumSynth drums;
            drums.setPadParameter(1, "tone", 0.7f); // Snare tone
            expectEquals(drums.getPadParameter(1, "tone"), 0.7f);
        }

        beginTest("Can set pad level");
        {
            DrumSynth drums;
            drums.setPadParameter(2, "level", 0.5f);
            expectEquals(drums.getPadParameter(2, "level"), 0.5f);
        }

        beginTest("Can set pad pan");
        {
            DrumSynth drums;
            drums.setPadParameter(3, "pan", -0.5f); // Left
            expectEquals(drums.getPadParameter(3, "pan"), -0.5f);
        }

        //======================================================================
        // Kit Selection
        //======================================================================
        beginTest("Can load 808 kit");
        {
            DrumSynth drums;
            drums.loadKit("808");
            expectEquals(drums.getCurrentKit(), juce::String("808"));
        }

        beginTest("Can load 909 kit");
        {
            DrumSynth drums;
            drums.loadKit("909");
            expectEquals(drums.getCurrentKit(), juce::String("909"));
        }

        beginTest("Can load Acoustic kit");
        {
            DrumSynth drums;
            drums.loadKit("Acoustic");
            expectEquals(drums.getCurrentKit(), juce::String("Acoustic"));
        }

        beginTest("Can load Lo-Fi kit");
        {
            DrumSynth drums;
            drums.loadKit("Lo-Fi");
            expectEquals(drums.getCurrentKit(), juce::String("Lo-Fi"));
        }

        beginTest("Can load Trap kit");
        {
            DrumSynth drums;
            drums.loadKit("Trap");
            expectEquals(drums.getCurrentKit(), juce::String("Trap"));
        }

        beginTest("Get available kits");
        {
            DrumSynth drums;
            auto kits = drums.getAvailableKits();
            expect(kits.size() >= 5);
            expect(kits.contains("808"));
            expect(kits.contains("909"));
        }

        //======================================================================
        // Audio Processing
        //======================================================================
        beginTest("prepareToPlay initializes audio settings");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);
            expectEquals(drums.getSampleRate(), 44100.0);
            expectEquals(drums.getBlockSize(), 512);
        }

        beginTest("processBlock produces silent output with no notes");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;

            drums.processBlock(buffer, midi);

            float rms = buffer.getRMSLevel(0, 0, 512);
            expect(rms < 0.001f);
        }

        beginTest("Kick produces output when triggered");
        {
            DrumSynth drums;
            drums.prepareToPlay(44100.0, 512);

            drums.noteOn(36, 0.9f); // Kick

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;

            drums.processBlock(buffer, midi);

            // Should have audio output
            float rms = buffer.getRMSLevel(0, 0, 512);
            expect(rms > 0.01f);
        }

        //======================================================================
        // Presets
        //======================================================================
        beginTest("Has kit presets");
        {
            DrumSynth drums;
            auto presets = drums.getPresets();
            expect(presets.size() >= 5);
        }

        beginTest("Loading preset changes kit");
        {
            DrumSynth drums;
            drums.loadKit("808");

            auto presets = drums.getPresets();
            // Find 909 preset
            for (const auto& preset : presets)
            {
                if (preset.name == "909")
                {
                    drums.loadPreset(preset);
                    break;
                }
            }

            expectEquals(drums.getCurrentKit(), juce::String("909"));
        }

        //======================================================================
        // Master Parameters
        //======================================================================
        beginTest("Has master volume parameter");
        {
            DrumSynth drums;
            drums.setParameter("volume", 0.5f);
            expectEquals(drums.getParameter("volume"), 0.5f);
        }

        beginTest("Has swing parameter");
        {
            DrumSynth drums;
            drums.setParameter("swing", 0.5f); // 50% swing
            expectEquals(drums.getParameter("swing"), 0.5f);
        }
    }
};

// Register the test
static DrumSynthTests drumSynthTests;
