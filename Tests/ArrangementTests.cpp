/**
 * Arrangement Tests - TempoTrack, TimeSignatureTrack, MarkerTrack
 */

#include <juce_core/juce_core.h>
#include "../Source/Audio/TempoTrack.h"
#include "../Source/Audio/TimeSignatureTrack.h"
#include "../Source/Audio/MarkerTrack.h"

class ArrangementTests : public juce::UnitTest
{
public:
    ArrangementTests() : UnitTest("Arrangement") {}

    void runTest() override
    {
        //======================================================================
        // TempoTrack Tests
        //======================================================================
        beginTest("TempoTrack has initial tempo of 120");
        {
            TempoTrack track;
            expectEquals(track.getInitialTempo(), 120.0);
        }

        beginTest("TempoTrack can set initial tempo");
        {
            TempoTrack track;
            track.setInitialTempo(140.0);
            expectEquals(track.getInitialTempo(), 140.0);
        }

        beginTest("TempoTrack can add tempo events");
        {
            TempoTrack track;
            TempoEvent event;
            event.beatPosition = 16.0;
            event.bpm = 160.0;
            track.addEvent(event);

            expectEquals(track.getNumEvents(), (size_t)2);
            expectEquals(track.getTempoAtBeat(0.0), 120.0);
            expectEquals(track.getTempoAtBeat(16.0), 160.0);
        }

        beginTest("TempoTrack handles linear ramp");
        {
            TempoTrack track;
            track.setInitialTempo(120.0);

            TempoEvent rampEvent;
            rampEvent.beatPosition = 0.0;
            rampEvent.bpm = 120.0;
            rampEvent.rampType = TempoRampType::Linear;
            track.addEvent(rampEvent);

            TempoEvent endEvent;
            endEvent.beatPosition = 16.0;
            endEvent.bpm = 180.0;
            track.addEvent(endEvent);

            // At beat 8, tempo should be halfway (150 BPM)
            double tempoAtMidpoint = track.getTempoAtBeat(8.0);
            expectWithinAbsoluteError(tempoAtMidpoint, 150.0, 0.1);
        }

        beginTest("TempoTrack beatsToSeconds basic");
        {
            TempoTrack track;
            track.setInitialTempo(120.0);  // 2 beats per second

            // 4 beats at 120 BPM = 2 seconds
            expectWithinAbsoluteError(track.beatsToSeconds(4.0), 2.0, 0.01);
        }

        beginTest("TempoTrack can remove events");
        {
            TempoTrack track;
            TempoEvent event;
            event.beatPosition = 16.0;
            event.bpm = 160.0;
            track.addEvent(event);

            track.removeEventAt(16.0);
            expectEquals(track.getNumEvents(), (size_t)1);
        }

        beginTest("TempoTrack serialization");
        {
            TempoTrack track;
            track.setInitialTempo(130.0);

            TempoEvent event;
            event.beatPosition = 32.0;
            event.bpm = 170.0;
            track.addEvent(event);

            auto var = track.toVar();

            TempoTrack restored;
            restored.fromVar(var);

            expectEquals(restored.getInitialTempo(), 130.0);
            expectEquals(restored.getNumEvents(), (size_t)2);
        }

        //======================================================================
        // TimeSignatureTrack Tests
        //======================================================================
        beginTest("TimeSignatureTrack has initial 4/4");
        {
            TimeSignatureTrack track;
            auto sig = track.getInitialTimeSignature();
            expectEquals(sig.numerator, 4);
            expectEquals(sig.denominator, 4);
        }

        beginTest("TimeSignatureTrack can set initial time signature");
        {
            TimeSignatureTrack track;
            track.setInitialTimeSignature(3, 4);
            auto sig = track.getInitialTimeSignature();
            expectEquals(sig.numerator, 3);
            expectEquals(sig.denominator, 4);
        }

        beginTest("TimeSignatureTrack can add events");
        {
            TimeSignatureTrack track;

            TimeSignatureEvent event;
            event.barPosition = 8.0;
            event.numerator = 6;
            event.denominator = 8;
            track.addEvent(event);

            expectEquals(track.getNumEvents(), (size_t)2);

            auto sigAtBar10 = track.getTimeSignatureAtBar(10.0);
            expectEquals(sigAtBar10.numerator, 6);
            expectEquals(sigAtBar10.denominator, 8);
        }

        beginTest("TimeSignatureTrack barsToBeats with 4/4");
        {
            TimeSignatureTrack track;
            // 4/4 means 4 beats per bar
            expectWithinAbsoluteError(track.barsToBeats(1.0), 4.0, 0.01);
            expectWithinAbsoluteError(track.barsToBeats(4.0), 16.0, 0.01);
        }

        beginTest("TimeSignatureTrack barsToBeats with 3/4");
        {
            TimeSignatureTrack track;
            track.setInitialTimeSignature(3, 4);
            // 3/4 means 3 beats per bar
            expectWithinAbsoluteError(track.barsToBeats(1.0), 3.0, 0.01);
            expectWithinAbsoluteError(track.barsToBeats(4.0), 12.0, 0.01);
        }

        beginTest("TimeSignatureTrack serialization");
        {
            TimeSignatureTrack track;
            track.setInitialTimeSignature(6, 8);

            auto var = track.toVar();

            TimeSignatureTrack restored;
            restored.fromVar(var);

            auto sig = restored.getInitialTimeSignature();
            expectEquals(sig.numerator, 6);
            expectEquals(sig.denominator, 8);
        }

        //======================================================================
        // MarkerTrack Tests
        //======================================================================
        beginTest("MarkerTrack starts empty");
        {
            MarkerTrack track;
            expectEquals(track.getNumMarkers(), (size_t)0);
        }

        beginTest("MarkerTrack can add markers");
        {
            MarkerTrack track;
            auto* marker = track.addMarker(0.0, "Intro");

            expect(marker != nullptr);
            expectEquals(track.getNumMarkers(), (size_t)1);
            expectEquals(marker->name, juce::String("Intro"));
        }

        beginTest("MarkerTrack can remove markers");
        {
            MarkerTrack track;
            auto* marker = track.addMarker(0.0, "Intro");
            juce::String id = marker->id;

            track.removeMarker(id);
            expectEquals(track.getNumMarkers(), (size_t)0);
        }

        beginTest("MarkerTrack getNextMarker");
        {
            MarkerTrack track;
            track.addMarker(0.0, "Intro");
            track.addMarker(16.0, "Verse");
            track.addMarker(32.0, "Chorus");

            auto* next = track.getNextMarker(8.0);
            expect(next != nullptr);
            expectEquals(next->name, juce::String("Verse"));
        }

        beginTest("MarkerTrack getPreviousMarker");
        {
            MarkerTrack track;
            track.addMarker(0.0, "Intro");
            track.addMarker(16.0, "Verse");
            track.addMarker(32.0, "Chorus");

            auto* prev = track.getPreviousMarker(20.0);
            expect(prev != nullptr);
            expectEquals(prev->name, juce::String("Verse"));
        }

        beginTest("MarkerTrack can rename markers");
        {
            MarkerTrack track;
            auto* marker = track.addMarker(0.0, "Intro");
            juce::String id = marker->id;

            track.renameMarker(id, "Introduction");

            auto* renamed = track.getMarker(id);
            expect(renamed != nullptr);
            expectEquals(renamed->name, juce::String("Introduction"));
        }

        beginTest("MarkerTrack serialization");
        {
            MarkerTrack track;
            track.addMarker(0.0, "Intro");
            track.addMarker(16.0, "Verse");

            auto var = track.toVar();

            MarkerTrack restored;
            restored.fromVar(var);

            expectEquals(restored.getNumMarkers(), (size_t)2);
        }
    }
};

// Register the test
static ArrangementTests arrangementTests;
