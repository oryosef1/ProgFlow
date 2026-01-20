#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Track.h"
#include "ClipComponent.h"
#include "AudioClipComponent.h"
#include "AutomationLaneComponent.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>

/**
 * TrackLane - Horizontal lane for a single track in the timeline
 *
 * Contains ClipComponents for each clip in the track.
 * Supports creating new clips via double-click.
 */
class TrackLane : public juce::Component,
                  public juce::FileDragAndDropTarget
{
public:
    TrackLane(Track& track, int barWidth, int trackHeight, int trackIndex = 0);

    void setBarWidth(int width);
    int getBarWidth() const { return barWidth; }

    void setBpm(double newBpm);
    double getBpm() const { return bpm; }

    void setTrackHeight(int height);
    int getTrackHeight() const { return trackHeight; }

    void setTrackIndex(int index) { trackIndex = index; repaint(); }
    int getTrackIndex() const { return trackIndex; }

    void setSnapEnabled(bool enabled) { snapEnabled = enabled; }
    bool isSnapEnabled() const { return snapEnabled; }

    void updateClips();  // Rebuild clip components from Track data

    Track& getTrack() { return track; }
    const Track& getTrack() const { return track; }

    void setSelectedClip(MidiClip* clip);
    void setSelectedClips(const std::set<MidiClip*>& clips);
    MidiClip* getSelectedClip() const { return selectedClip; }
    std::set<MidiClip*> getClipsInRect(const juce::Rectangle<int>& rect) const;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Create clip on double-click
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    // FileDragAndDropTarget
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;

    // Callbacks
    std::function<void(MidiClip*)> onClipSelected;
    std::function<void(MidiClip*)> onClipDoubleClicked;  // Open piano roll
    std::function<void(double)> onCreateClip;            // Bar position
    std::function<void(AudioClip*)> onAudioClipSelected;
    std::function<void(const juce::File&, double)> onAudioFileDropped;  // File and beat position

    // Automation
    void setAutomationExpanded(bool expanded);
    bool isAutomationExpanded() const { return automationExpanded; }
    void refreshAutomationLanes();
    void addAutomationLane(const juce::String& parameterId);
    int getTotalHeight() const;  // Base height + automation lanes

private:
    Track& track;
    int barWidth;
    double bpm = 120.0;
    int trackHeight;
    int trackIndex = 0;
    bool snapEnabled = true;
    MidiClip* selectedClip = nullptr;
    AudioClip* selectedAudioClip = nullptr;

    std::vector<std::unique_ptr<ClipComponent>> clipComponents;
    std::vector<std::unique_ptr<AudioClipComponent>> audioClipComponents;

    // File drag state
    bool fileDragHover = false;

    // Automation
    std::vector<std::unique_ptr<AutomationLaneComponent>> automationLaneComponents;
    bool automationExpanded = false;

    void handleClipSelected(ClipComponent* comp);
    void handleClipDoubleClicked(ClipComponent* comp);
    void handleAudioClipSelected(AudioClipComponent* comp);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLane)
};
