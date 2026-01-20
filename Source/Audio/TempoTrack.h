#pragma once

#include <juce_core/juce_core.h>
#include <vector>

/**
 * TempoRampType - How tempo transitions between events
 */
enum class TempoRampType
{
    Instant,  // Immediate tempo change
    Linear    // Gradual ramp to new tempo
};

/**
 * TempoEvent - A tempo change point on the timeline
 */
struct TempoEvent
{
    double beatPosition = 0.0;   // Position in beats
    double bpm = 120.0;          // Tempo in BPM
    TempoRampType rampType = TempoRampType::Instant;

    // For serialization
    juce::var toVar() const;
    static TempoEvent fromVar(const juce::var& var);
};

/**
 * TempoTrack - Manages tempo automation throughout the project
 *
 * Features:
 * - Multiple tempo change points
 * - Instant or linear ramp transitions
 * - Query tempo at any beat position
 * - Convert between beats and time
 */
class TempoTrack
{
public:
    TempoTrack();

    //==========================================================================
    // Event management

    /**
     * Add a tempo event. If an event exists at the same position, it's replaced.
     */
    void addEvent(const TempoEvent& event);

    /**
     * Remove event at the given beat position
     */
    void removeEventAt(double beatPosition);

    /**
     * Remove all events except the initial tempo
     */
    void clearEvents();

    /**
     * Get all tempo events
     */
    const std::vector<TempoEvent>& getEvents() const { return events; }

    /**
     * Get number of events
     */
    size_t getNumEvents() const { return events.size(); }

    //==========================================================================
    // Tempo queries

    /**
     * Get the tempo (BPM) at a specific beat position
     * Handles ramping between events
     */
    double getTempoAtBeat(double beatPosition) const;

    /**
     * Get the initial/default tempo
     */
    double getInitialTempo() const;

    /**
     * Set the initial tempo (first event at beat 0)
     */
    void setInitialTempo(double bpm);

    //==========================================================================
    // Time conversion

    /**
     * Convert beat position to time in seconds
     * Accounts for tempo changes
     */
    double beatsToSeconds(double beats) const;

    /**
     * Convert time in seconds to beat position
     * Accounts for tempo changes
     */
    double secondsToBeats(double seconds) const;

    /**
     * Get the duration in seconds for a range of beats
     */
    double getBeatRangeDuration(double startBeat, double endBeat) const;

    //==========================================================================
    // Serialization

    juce::var toVar() const;
    void fromVar(const juce::var& var);

private:
    std::vector<TempoEvent> events;

    // Sort events by beat position
    void sortEvents();

    // Find the event index at or before a beat position
    int findEventIndexAt(double beatPosition) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TempoTrack)
};
