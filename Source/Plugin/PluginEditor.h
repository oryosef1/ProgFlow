#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "../UI/TransportBar.h"
#include "../UI/Timeline/TimelinePanel.h"
#include "../UI/Tracks/TrackHeaderPanel.h"
#include "../UI/Mixer/MixerPanel.h"
#include "../UI/PianoRoll/PianoRollEditor.h"
#include "../UI/Effects/EffectChainPanel.h"
#include "../UI/LookAndFeel.h"

/**
 * ProgFlowPluginEditor - Plugin UI for ProgFlow
 *
 * Contains the full ProgFlow DAW interface in a plugin window.
 */
class ProgFlowPluginEditor : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit ProgFlowPluginEditor(ProgFlowPluginProcessor& processor);
    ~ProgFlowPluginEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    ProgFlowPluginProcessor& processorRef;
    ProgFlowLookAndFeel lookAndFeel;

    // UI Components
    std::unique_ptr<TransportBar> transportBar;
    std::unique_ptr<TrackHeaderPanel> trackHeaderPanel;
    std::unique_ptr<TimelinePanel> timelinePanel;
    std::unique_ptr<MixerPanel> mixerPanel;
    std::unique_ptr<PianoRollEditor> pianoRollEditor;
    std::unique_ptr<EffectChainPanel> effectChainPanel;

    // View state
    enum class ViewMode { Arrange, Mixer };
    ViewMode viewMode = ViewMode::Arrange;

    // Layout
    static constexpr int TRANSPORT_HEIGHT = 50;
    static constexpr int TRACK_HEADER_WIDTH = 200;
    static constexpr int BOTTOM_PANEL_HEIGHT = 250;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProgFlowPluginEditor)
};
