#include "PluginEditor.h"

ProgFlowPluginEditor::ProgFlowPluginEditor(ProgFlowPluginProcessor& processor)
    : AudioProcessorEditor(&processor),
      processorRef(processor)
{
    // Set look and feel
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);

    auto& engine = processorRef.getAudioEngine();

    // Create transport bar
    transportBar = std::make_unique<TransportBar>(engine);
    addAndMakeVisible(*transportBar);

    // Create track header panel
    trackHeaderPanel = std::make_unique<TrackHeaderPanel>(engine);
    addAndMakeVisible(*trackHeaderPanel);

    // Create timeline panel
    timelinePanel = std::make_unique<TimelinePanel>(engine);
    addAndMakeVisible(*timelinePanel);

    // Create mixer panel (initially hidden)
    mixerPanel = std::make_unique<MixerPanel>(engine);
    addChildComponent(*mixerPanel);

    // Create piano roll editor
    pianoRollEditor = std::make_unique<PianoRollEditor>(engine);
    addAndMakeVisible(*pianoRollEditor);

    // Create effect chain panel
    effectChainPanel = std::make_unique<EffectChainPanel>(engine.getEffectChain());
    addAndMakeVisible(*effectChainPanel);

    // Wire up callbacks
    timelinePanel->onClipDoubleClicked = [this](MidiClip* clip) {
        if (clip && pianoRollEditor)
        {
            pianoRollEditor->setClip(clip);
        }
    };

    trackHeaderPanel->onTrackSelected = [this](Track* track) {
        juce::ignoreUnused(track);
        // Track selection handled by track header panel
    };

    // Set initial size
    setSize(1200, 800);
    setResizable(true, true);
    setResizeLimits(800, 600, 2560, 1600);

    // Start timer for UI updates
    startTimerHz(30);
}

ProgFlowPluginEditor::~ProgFlowPluginEditor()
{
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void ProgFlowPluginEditor::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());
}

void ProgFlowPluginEditor::resized()
{
    auto bounds = getLocalBounds();

    // Transport bar at top
    transportBar->setBounds(bounds.removeFromTop(TRANSPORT_HEIGHT));

    if (viewMode == ViewMode::Arrange)
    {
        // Bottom panel (piano roll + effects)
        auto bottomArea = bounds.removeFromBottom(BOTTOM_PANEL_HEIGHT);

        // Piano roll takes 70%, effect chain takes 30%
        int pianoRollWidth = static_cast<int>(bottomArea.getWidth() * 0.7f);
        pianoRollEditor->setBounds(bottomArea.removeFromLeft(pianoRollWidth));
        effectChainPanel->setBounds(bottomArea);

        pianoRollEditor->setVisible(true);
        effectChainPanel->setVisible(true);
        mixerPanel->setVisible(false);

        // Track headers on left
        trackHeaderPanel->setBounds(bounds.removeFromLeft(TRACK_HEADER_WIDTH));
        trackHeaderPanel->setVisible(true);

        // Timeline takes remaining space
        timelinePanel->setBounds(bounds);
        timelinePanel->setVisible(true);
    }
    else // Mixer view
    {
        trackHeaderPanel->setVisible(false);
        timelinePanel->setVisible(false);
        pianoRollEditor->setVisible(false);
        effectChainPanel->setVisible(false);

        mixerPanel->setBounds(bounds);
        mixerPanel->setVisible(true);
    }
}

void ProgFlowPluginEditor::timerCallback()
{
    // Refresh UI components
    if (trackHeaderPanel)
        trackHeaderPanel->repaint();
    if (timelinePanel)
        timelinePanel->repaint();
}
