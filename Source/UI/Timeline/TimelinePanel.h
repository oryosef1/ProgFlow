#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/AudioEngine.h"
#include "../../Audio/MidiClip.h"
#include "TimeRuler.h"
#include "TrackLane.h"
#include "PlayheadComponent.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>
#include <random>

// Forward declaration
class TimelinePanel;

/**
 * Custom Viewport that notifies TimelinePanel of scroll changes
 */
class TimelineViewport : public juce::Viewport
{
public:
    explicit TimelineViewport(TimelinePanel& panel) : owner(panel) {}

    void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override;

private:
    TimelinePanel& owner;
};

/**
 * TimelinePanel - Main timeline/arrangement view
 *
 * Contains:
 * - TimeRuler at top
 * - TrackLanes for each track
 * - PlayheadComponent overlay
 * - Zoom and scroll controls
 */
class TimelinePanel : public juce::Component,
                      public juce::Timer,
                      public juce::KeyListener
{
public:
    TimelinePanel(AudioEngine& engine);
    ~TimelinePanel() override;

    //==========================================================================
    // Zoom & scroll
    void setHorizontalZoom(float zoom);  // 0.25 - 4.0
    float getHorizontalZoom() const { return horizontalZoom; }

    void setScrollPosition(double bars);
    double getScrollPosition() const { return scrollPosition; }

    //==========================================================================
    // Snap
    void setSnapEnabled(bool enabled);
    bool isSnapEnabled() const { return snapEnabled; }
    void toggleSnap() { setSnapEnabled(!snapEnabled); }

    //==========================================================================
    // Selection
    void selectClip(MidiClip* clip);
    void selectClips(const std::set<MidiClip*>& clips);
    void selectAllClips();
    void selectNextClip();
    void selectPreviousClip();
    void addToSelection(MidiClip* clip);
    void clearSelection();
    MidiClip* getSelectedClip() const { return selectedClip; }
    const std::set<MidiClip*>& getSelectedClips() const { return selectedClips; }
    bool hasMultipleSelectedClips() const { return selectedClips.size() > 1; }

    //==========================================================================
    // Track/Clip management
    void updateTracks();  // Rebuild from AudioEngine
    void createClipOnTrack(Track* track, double barPosition);
    void deleteSelectedClip();
    void splitSelectedClip();  // Split at playhead (Cmd+E)

    // Clipboard operations
    void copySelectedClip();
    void cutSelectedClip();
    void pasteClip();
    void duplicateSelectedClip();

    // Navigation
    void goToStart();
    void goToEnd();
    void nudgeClip(double deltaBars);
    void zoomIn();
    void zoomOut();
    void zoomToFit();

    //==========================================================================
    // Callbacks
    std::function<void(MidiClip*)> onClipSelected;
    std::function<void(MidiClip*)> onClipDoubleClicked;  // Open piano roll

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    //==========================================================================
    // Timer for playhead updates
    void timerCallback() override;

    //==========================================================================
    // KeyListener for keyboard shortcuts
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    //==========================================================================
    // Called by TimelineViewport when scroll position changes
    void onViewportScrolled();

private:
    AudioEngine& audioEngine;

    // Child components
    std::unique_ptr<TimeRuler> timeRuler;
    std::unique_ptr<TimelineViewport> trackViewport;
    std::unique_ptr<juce::Component> trackContainer;
    std::vector<std::unique_ptr<TrackLane>> trackLanes;
    std::unique_ptr<PlayheadComponent> playhead;

    // State
    float horizontalZoom = 1.0f;
    double scrollPosition = 0.0;
    bool snapEnabled = true;  // Snap to grid by default
    MidiClip* selectedClip = nullptr;
    std::set<MidiClip*> selectedClips;
    Track* selectedTrack = nullptr;  // Track containing selected clip

    // Marquee selection state
    bool isDraggingMarquee = false;
    juce::Point<int> marqueeStart;
    juce::Rectangle<int> marqueeRect;

    // Clipboard for copy/paste
    struct ClipboardClip
    {
        juce::String name;
        juce::Colour colour;
        double durationBars;
        std::vector<Note> notes;
    };
    std::unique_ptr<ClipboardClip> clipboardClip;

    // Layout constants
    static constexpr int RULER_HEIGHT = 30;
    static constexpr int TRACK_HEIGHT = 100;
    static constexpr int BASE_BAR_WIDTH = 100;  // Pixels per bar at zoom 1.0
    static constexpr int DEFAULT_BARS = 32;     // Default timeline length

    int getBarWidth() const;
    void updatePlayheadPosition();
    void handleSeek(double bar);
    void handleClipSelected(MidiClip* clip);
    void handleClipDoubleClicked(MidiClip* clip);
    void handleCreateClip(Track* track, double barPosition);
    void handleAudioFileDropped(Track* track, const juce::File& file, double beatPosition);
    std::set<MidiClip*> getClipsInRect(const juce::Rectangle<int>& rect) const;
    void drawMarqueeSelection(juce::Graphics& g);

    // Background animation
    struct Particle {
        float x, y;
        float vx, vy;
        float size;
        float alpha;
    };
    std::vector<Particle> particles;
    float animationTime = 0.0f;
    std::mt19937 animRng{std::random_device{}()};
    void initParticles();
    void updateParticles();
    void drawParticles(juce::Graphics& g);
    void drawWaveform(juce::Graphics& g);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelinePanel)
};
