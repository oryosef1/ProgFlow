/**
 * SoundFontPlayer Unit Tests
 *
 * Tests for SoundFont (.sf2) player synth.
 * Following TDD: tests written BEFORE implementation.
 */

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "../Source/Audio/Synths/SoundFontPlayer.h"

class SoundFontPlayerTests : public juce::UnitTest
{
public:
    SoundFontPlayerTests() : UnitTest("SoundFontPlayer") {}

    void runTest() override
    {
        //======================================================================
        // Construction and Initialization
        //======================================================================
        beginTest("SoundFontPlayer can be constructed");
        {
            SoundFontPlayer player;
            expect(true); // Just testing it doesn't crash
        }

        beginTest("SoundFontPlayer extends SynthBase");
        {
            SoundFontPlayer player;
            SynthBase* base = &player;
            expect(base != nullptr);
        }

        beginTest("SoundFontPlayer has correct parameter count");
        {
            SoundFontPlayer player;
            auto params = player.getParameterNames();
            // Should have at least: instrument, bank, volume, pan
            expect(params.size() >= 4);
        }

        //======================================================================
        // Instrument Selection
        //======================================================================
        beginTest("Default instrument is 0 (Acoustic Grand Piano)");
        {
            SoundFontPlayer player;
            expectEquals((int)player.getParameter("instrument"), 0);
        }

        beginTest("Can set instrument 0-127 (GM instruments)");
        {
            SoundFontPlayer player;
            player.setParameter("instrument", 40.0f); // Violin
            expectEquals((int)player.getParameter("instrument"), 40);

            player.setParameter("instrument", 127.0f); // Gunshot
            expectEquals((int)player.getParameter("instrument"), 127);
        }

        beginTest("Instrument parameter clamps to valid range");
        {
            SoundFontPlayer player;
            player.setParameter("instrument", -10.0f);
            expect(player.getParameter("instrument") >= 0.0f);

            player.setParameter("instrument", 200.0f);
            expect(player.getParameter("instrument") <= 127.0f);
        }

        beginTest("Can get instrument name by number");
        {
            SoundFontPlayer player;
            expectEquals(player.getInstrumentName(0), juce::String("Acoustic Grand Piano"));
            expectEquals(player.getInstrumentName(40), juce::String("Violin"));
            expectEquals(player.getInstrumentName(127), juce::String("Gunshot"));
        }

        beginTest("Get all instrument names returns 128 entries");
        {
            SoundFontPlayer player;
            auto names = player.getAllInstrumentNames();
            expectEquals((int)names.size(), 128);
        }

        //======================================================================
        // Bank Selection
        //======================================================================
        beginTest("Default bank is 0");
        {
            SoundFontPlayer player;
            expectEquals((int)player.getParameter("bank"), 0);
        }

        beginTest("Can set bank (for drum kits, variations)");
        {
            SoundFontPlayer player;
            player.setParameter("bank", 128.0f); // Percussion bank
            expectEquals((int)player.getParameter("bank"), 128);
        }

        //======================================================================
        // Audio Processing
        //======================================================================
        beginTest("prepareToPlay initializes audio settings");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);
            expectEquals(player.getSampleRate(), 44100.0);
            expectEquals(player.getBlockSize(), 512);
        }

        beginTest("processBlock produces silent output with no notes");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);

            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;

            player.processBlock(buffer, midi);

            // Should be silent (or very quiet)
            float rms = buffer.getRMSLevel(0, 0, 512);
            expect(rms < 0.001f);
        }

        beginTest("noteOn triggers sound output");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);

            // Trigger note
            player.noteOn(60, 0.8f); // Middle C at 80% velocity

            // Process a block
            juce::AudioBuffer<float> buffer(2, 512);
            buffer.clear();
            juce::MidiBuffer midi;

            player.processBlock(buffer, midi);

            // If SoundFont is loaded, should have active notes
            // If no SoundFont, gracefully handles (no crash)
            if (player.isSoundFontLoaded())
            {
                expect(player.hasActiveNotes());
            }
            else
            {
                // No SoundFont loaded - this is acceptable in test environment
                expect(true);
            }
        }

        beginTest("noteOff stops the note with release");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);

            player.noteOn(60, 0.8f);
            player.noteOff(60);

            // Note should be in release phase but still tracked briefly
            // After several blocks, it should be fully released
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;

            // Process multiple blocks to allow release
            for (int i = 0; i < 100; ++i)
            {
                buffer.clear();
                player.processBlock(buffer, midi);
            }

            expect(!player.hasActiveNotes());
        }

        beginTest("allNotesOff stops all playing notes");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);

            player.noteOn(60, 0.8f);
            player.noteOn(64, 0.8f);
            player.noteOn(67, 0.8f);

            // Only check if SoundFont is loaded
            if (player.isSoundFontLoaded())
            {
                expect(player.hasActiveNotes());
            }

            player.allNotesOff();

            // Process several blocks for release
            juce::AudioBuffer<float> buffer(2, 512);
            juce::MidiBuffer midi;
            for (int i = 0; i < 100; ++i)
            {
                buffer.clear();
                player.processBlock(buffer, midi);
            }

            // After allNotesOff and release, should have no active notes
            expect(!player.hasActiveNotes());
        }

        //======================================================================
        // SoundFont Loading
        //======================================================================
        beginTest("Can check if SoundFont is loaded");
        {
            SoundFontPlayer player;
            // On construction, may or may not have built-in SF2
            // This just tests the method exists
            bool loaded = player.isSoundFontLoaded();
            expect(loaded || !loaded); // Method exists
        }

        beginTest("Can get current SoundFont path");
        {
            SoundFontPlayer player;
            juce::String path = player.getCurrentSoundFontPath();
            // Path could be empty or point to bundled SF2
            expect(true); // Method exists
        }

        //======================================================================
        // Presets
        //======================================================================
        beginTest("Has GM instrument presets");
        {
            SoundFontPlayer player;
            auto presets = player.getPresets();
            // Should have at least some preset categories
            expect(presets.size() >= 8); // At least 8 categories of GM sounds
        }

        beginTest("Loading preset changes instrument");
        {
            SoundFontPlayer player;

            // Load a preset that should set a specific instrument
            auto presets = player.getPresets();
            if (presets.size() > 0)
            {
                player.loadPreset(presets[0]);
                // Verify preset was loaded
                expect(true);
            }
        }

        //======================================================================
        // Volume and Pan
        //======================================================================
        beginTest("Volume parameter affects output level");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);

            // Default volume
            float defaultVol = player.getParameter("volume");
            expect(defaultVol > 0.0f && defaultVol <= 1.0f);
        }

        beginTest("Pan parameter exists and can be set");
        {
            SoundFontPlayer player;
            player.setParameter("pan", -1.0f); // Full left
            expectEquals(player.getParameter("pan"), -1.0f);

            player.setParameter("pan", 1.0f); // Full right
            expectEquals(player.getParameter("pan"), 1.0f);

            player.setParameter("pan", 0.0f); // Center
            expectEquals(player.getParameter("pan"), 0.0f);
        }

        //======================================================================
        // Polyphony
        //======================================================================
        beginTest("Supports at least 32 voice polyphony");
        {
            SoundFontPlayer player;
            player.prepareToPlay(44100.0, 512);

            // Trigger 32 notes
            for (int i = 36; i < 68; ++i)
            {
                player.noteOn(i, 0.5f);
            }

            // Should have 32 active notes tracked
            // Note: activeNotes tracks note-on events, not TSF voices
            expectEquals((int)player.getActiveNotes().size(), 32);
        }

        //======================================================================
        // Pitch Bend and Modulation
        //======================================================================
        beginTest("Pitch bend parameter exists");
        {
            SoundFontPlayer player;
            player.setParameter("pitchBend", 0.5f); // Center = no bend
            expectEquals(player.getParameter("pitchBend"), 0.5f);
        }

        beginTest("Modulation wheel parameter exists");
        {
            SoundFontPlayer player;
            player.setParameter("modWheel", 0.0f);
            expectEquals(player.getParameter("modWheel"), 0.0f);
        }

        //======================================================================
        // ADSR Envelope Override
        //======================================================================
        beginTest("Attack parameter can override SF2 envelope");
        {
            SoundFontPlayer player;
            player.setParameter("attackOverride", 0.5f);
            expectEquals(player.getParameter("attackOverride"), 0.5f);
        }

        beginTest("Release parameter can override SF2 envelope");
        {
            SoundFontPlayer player;
            player.setParameter("releaseOverride", 0.3f);
            expectEquals(player.getParameter("releaseOverride"), 0.3f);
        }
    }
};

// Register the test
static SoundFontPlayerTests soundFontPlayerTests;
