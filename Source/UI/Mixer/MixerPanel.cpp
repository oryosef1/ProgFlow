#include "MixerPanel.h"

MixerPanel::MixerPanel(AudioEngine& audioEngine)
    : audioEngine(audioEngine)
{
    // Viewport for scrollable track strips (Saturn styling)
    stripContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(stripContainer.get(), false);
    viewport.setScrollBarsShown(false, true);  // Horizontal scroll only
    viewport.setScrollBarThickness(6);
    viewport.setColour(juce::ScrollBar::thumbColourId, ProgFlowColours::textMuted());
    addAndMakeVisible(viewport);

    // Master channel strip
    masterStrip = std::make_unique<ChannelStrip>(audioEngine);
    addAndMakeVisible(*masterStrip);

    // Initial channel strip creation
    refreshTracks();

    // Start timer
    startTimerHz(10);  // Slower refresh for track list changes
}

MixerPanel::~MixerPanel()
{
    stopTimer();
}

void MixerPanel::timerCallback()
{
    // Check if track count changed
    int numTracks = audioEngine.getNumTracks();
    if (static_cast<int>(channelStrips.size()) != numTracks)
    {
        refreshTracks();
    }
}

void MixerPanel::refreshTracks()
{
    // Clear existing strips
    channelStrips.clear();

    // Create strips for each track
    int numTracks = audioEngine.getNumTracks();
    for (int i = 0; i < numTracks; ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (track)
        {
            auto strip = std::make_unique<ChannelStrip>(*track);
            strip->onTrackSelected = [this](Track* t) {
                selectTrack(t);
            };

            // Click to select
            strip->addMouseListener(this, true);

            stripContainer->addAndMakeVisible(*strip);
            channelStrips.push_back(std::move(strip));
        }
    }

    // Update container size
    int totalWidth = numTracks * (ChannelStrip::defaultWidth + stripSpacing);
    stripContainer->setSize(std::max(totalWidth, viewport.getWidth()),
                            viewport.getHeight());

    resized();
}

void MixerPanel::paint(juce::Graphics& g)
{
    // Saturn design: dark background
    g.fillAll(ProgFlowColours::bgPrimary());

    // Subtle separator before master strip
    int masterX = getWidth() - masterStripWidth - 12;
    g.setColour(ProgFlowColours::border());
    g.drawLine(static_cast<float>(masterX), 4.0f,
               static_cast<float>(masterX), static_cast<float>(getHeight() - 4));
}

void MixerPanel::resized()
{
    auto bounds = getLocalBounds();
    bounds.reduce(4, 4);

    // Master strip on right
    auto masterBounds = bounds.removeFromRight(masterStripWidth);
    bounds.removeFromRight(8);  // Separator gap
    masterStrip->setBounds(masterBounds);

    // Viewport for track strips
    viewport.setBounds(bounds);

    // Update container and position strips
    int containerHeight = viewport.getHeight();
    int numStrips = static_cast<int>(channelStrips.size());
    int totalWidth = numStrips * (ChannelStrip::defaultWidth + stripSpacing);
    stripContainer->setSize(std::max(totalWidth, viewport.getWidth()), containerHeight);

    int x = 0;
    for (auto& strip : channelStrips)
    {
        strip->setBounds(x, 0, ChannelStrip::defaultWidth, containerHeight);
        x += ChannelStrip::defaultWidth + stripSpacing;
    }
}

void MixerPanel::selectTrack(Track* track)
{
    selectedTrack = track;

    // Update selection state
    for (auto& strip : channelStrips)
    {
        strip->setSelected(strip->getTrack() == track);
    }

    if (onTrackSelected)
        onTrackSelected(track);
}
