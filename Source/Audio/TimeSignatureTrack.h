#pragma once

#include <juce_core/juce_core.h>
#include <vector>

/**
 * TimeSignatureEvent - A time signature change point
 */
struct TimeSignatureEvent
{
    double barPosition = 0.0;  // Position in bars
    int numerator = 4;         // Beats per bar
    int denominator = 4;       // Note value that gets one beat

    // Convenience methods
    int getBeatsPerBar() const { return numerator; }
    double getBeatLength() const { return 4.0 / denominator; }  // In quarter notes

    // For serialization
    juce::var toVar() const;
    static TimeSignatureEvent fromVar(const juce::var& var);
};

/**
 * TimeSignatureTrack - Manages time signature changes throughout the project
 *
 * Features:
 * - Multiple time signature change points
 * - Query time signature at any bar position
 * - Convert between bars and beats
 */
class TimeSignatureTrack
{
public:
    TimeSignatureTrack();

    //==========================================================================
    // Event management

    /**
     * Add a time signature event. If an event exists at the same position, it's replaced.
     */
    void addEvent(const TimeSignatureEvent& event);

    /**
     * Remove event at the given bar position
     */
    void removeEventAt(double barPosition);

    /**
     * Remove all events except the initial time signature
     */
    void clearEvents();

    /**
     * Get all time signature events
     */
    const std::vector<TimeSignatureEvent>& getEvents() const { return events; }

    /**
     * Get number of events
     */
    size_t getNumEvents() const { return events.size(); }

    //==========================================================================
    // Time signature queries

    /**
     * Get the time signature at a specific bar position
     */
    TimeSignatureEvent getTimeSignatureAtBar(double barPosition) const;

    /**
     * Get the initial/default time signature
     */
    TimeSignatureEvent getInitialTimeSignature() const;

    /**
     * Set the initial time signature (first event at bar 0)
     */
    void setInitialTimeSignature(int numerator, int denominator);

    //==========================================================================
    // Conversion

    /**
     * Convert bar position to beat position
     * Accounts for time signature changes
     */
    double barsToBeats(double bars) const;

    /**
     * Convert beat position to bar position
     * Accounts for time signature changes
     */
    double beatsToBar(double beats) const;

    /**
     * Get the number of beats in a specific bar
     */
    int getBeatsInBar(double barPosition) const;

    //==========================================================================
    // Serialization

    juce::var toVar() const;
    void fromVar(const juce::var& var);

private:
    std::vector<TimeSignatureEvent> events;

    // Sort events by bar position
    void sortEvents();

    // Find the event index at or before a bar position
    int findEventIndexAt(double barPosition) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimeSignatureTrack)
};
