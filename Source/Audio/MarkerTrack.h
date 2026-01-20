#pragma once

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>
#include <vector>

/**
 * Marker - A named location on the timeline
 */
struct Marker
{
    juce::String id;           // Unique identifier
    juce::String name;         // Display name
    double beatPosition = 0.0; // Position in beats
    juce::Colour colour;       // Display color

    Marker() : id(juce::Uuid().toString()), colour(0xffff9500) {}

    // For serialization
    juce::var toVar() const;
    static Marker fromVar(const juce::var& var);
};

/**
 * MarkerTrack - Manages markers throughout the project
 *
 * Features:
 * - Add/remove/rename markers
 * - Jump to marker positions
 * - Navigate between markers
 */
class MarkerTrack
{
public:
    MarkerTrack() = default;

    //==========================================================================
    // Marker management

    /**
     * Add a new marker
     * @return Pointer to the added marker
     */
    Marker* addMarker(double beatPosition, const juce::String& name = "");

    /**
     * Add an existing marker object
     */
    Marker* addMarker(const Marker& marker);

    /**
     * Remove marker by ID
     */
    void removeMarker(const juce::String& id);

    /**
     * Remove marker at beat position (within tolerance)
     */
    void removeMarkerAt(double beatPosition, double tolerance = 0.1);

    /**
     * Clear all markers
     */
    void clearMarkers();

    /**
     * Get marker by ID
     */
    Marker* getMarker(const juce::String& id);
    const Marker* getMarker(const juce::String& id) const;

    /**
     * Get all markers
     */
    const std::vector<Marker>& getMarkers() const { return markers; }

    /**
     * Get number of markers
     */
    size_t getNumMarkers() const { return markers.size(); }

    //==========================================================================
    // Navigation

    /**
     * Get the next marker after the given beat position
     * @return Pointer to next marker, or nullptr if none
     */
    const Marker* getNextMarker(double beatPosition) const;

    /**
     * Get the previous marker before the given beat position
     * @return Pointer to previous marker, or nullptr if none
     */
    const Marker* getPreviousMarker(double beatPosition) const;

    /**
     * Get the nearest marker to the given beat position
     */
    const Marker* getNearestMarker(double beatPosition) const;

    /**
     * Get marker at beat position (within tolerance)
     */
    Marker* getMarkerAt(double beatPosition, double tolerance = 0.1);

    //==========================================================================
    // Editing

    /**
     * Rename a marker
     */
    void renameMarker(const juce::String& id, const juce::String& newName);

    /**
     * Move a marker to a new position
     */
    void moveMarker(const juce::String& id, double newBeatPosition);

    /**
     * Set marker color
     */
    void setMarkerColour(const juce::String& id, juce::Colour colour);

    //==========================================================================
    // Serialization

    juce::var toVar() const;
    void fromVar(const juce::var& var);

private:
    std::vector<Marker> markers;

    // Sort markers by beat position
    void sortMarkers();

    // Generate a default name for a new marker
    juce::String generateMarkerName() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkerTrack)
};
