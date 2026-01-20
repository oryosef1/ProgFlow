#include "TrackHeaderPanel.h"

TrackHeaderPanel::TrackHeaderPanel(AudioEngine& audioEngine)
    : audioEngine(audioEngine)
{
    // Title label
    titleLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    addAndMakeVisible(titleLabel);

    // Add track button
    addTrackButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
    addTrackButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    addTrackButton.onClick = [this]() { addNewTrack(); };
    addAndMakeVisible(addTrackButton);

    // Viewport for scrollable track list
    trackListContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(trackListContainer.get(), false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(8);
    addAndMakeVisible(viewport);

    // Master meters
    masterLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    masterLabel.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);
    addAndMakeVisible(masterMeterL);
    addAndMakeVisible(masterMeterR);

    // Initial track list
    refreshTracks();

    // Start timer for master meters
    startTimerHz(30);
}

TrackHeaderPanel::~TrackHeaderPanel()
{
    stopTimer();
}

void TrackHeaderPanel::timerCallback()
{
    // Update master meters
    masterMeterL.setLevel(audioEngine.getMasterLevelL());
    masterMeterR.setLevel(audioEngine.getMasterLevelR());
}

void TrackHeaderPanel::refreshTracks()
{
    // Clear existing headers
    trackHeaders.clear();

    // Create headers for each track
    int numTracks = audioEngine.getNumTracks();
    for (int i = 0; i < numTracks; ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (track)
        {
            auto header = std::make_unique<TrackHeader>(*track);
            header->onTrackSelected = [this](Track& t) {
                selectTrack(&t);
            };
            header->onTrackDeleted = [this](Track& t) {
                deleteTrack(&t);
            };
            header->onSynthTypeChanged = [this](Track& t) {
                // Re-select track to update synth editor
                selectTrack(&t);
            };

            // Click to select
            header->addMouseListener(this, false);

            trackListContainer->addAndMakeVisible(*header);
            trackHeaders.push_back(std::move(header));
        }
    }

    // Update container size
    int totalHeight = numTracks * TrackHeader::defaultHeight;
    trackListContainer->setSize(viewport.getWidth(), std::max(totalHeight, viewport.getHeight()));

    resized();
}

void TrackHeaderPanel::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgSecondary());

    // Header background
    g.setColour(ProgFlowColours::bgTertiary());
    g.fillRect(0, 0, getWidth(), headerHeight);

    // Border below header
    g.setColour(ProgFlowColours::border());
    g.drawLine(0.0f, static_cast<float>(headerHeight), static_cast<float>(getWidth()),
               static_cast<float>(headerHeight));

    // Border above master section
    int masterY = getHeight() - masterHeight;
    g.drawLine(0.0f, static_cast<float>(masterY), static_cast<float>(getWidth()),
               static_cast<float>(masterY));
}

void TrackHeaderPanel::resized()
{
    auto bounds = getLocalBounds();

    // Header area
    auto headerBounds = bounds.removeFromTop(headerHeight);
    headerBounds.reduce(8, 4);
    addTrackButton.setBounds(headerBounds.removeFromRight(24));
    headerBounds.removeFromRight(8);
    titleLabel.setBounds(headerBounds);

    // Master section at bottom
    auto masterBounds = bounds.removeFromBottom(masterHeight);
    masterBounds.reduce(8, 8);
    masterLabel.setBounds(masterBounds.removeFromTop(16));
    masterBounds.removeFromTop(4);

    // Master meters (centered)
    int meterWidth = 16;
    int totalMeterWidth = meterWidth * 2 + 4;
    int meterX = (masterBounds.getWidth() - totalMeterWidth) / 2 + masterBounds.getX();
    masterMeterL.setBounds(meterX, masterBounds.getY(), meterWidth, masterBounds.getHeight());
    masterMeterR.setBounds(meterX + meterWidth + 4, masterBounds.getY(), meterWidth, masterBounds.getHeight());

    // Viewport fills the rest
    viewport.setBounds(bounds);

    // Update track list container
    int containerWidth = viewport.getWidth() - viewport.getScrollBarThickness();
    int totalHeight = static_cast<int>(trackHeaders.size()) * TrackHeader::defaultHeight;
    trackListContainer->setSize(containerWidth, std::max(totalHeight, viewport.getHeight()));

    // Position track headers
    int y = 0;
    for (auto& header : trackHeaders)
    {
        header->setBounds(0, y, containerWidth, TrackHeader::defaultHeight);
        y += TrackHeader::defaultHeight;
    }
}

void TrackHeaderPanel::selectTrack(Track* track)
{
    selectedTrack = track;

    // Update selection state
    for (auto& header : trackHeaders)
    {
        header->setSelected(&header->getTrack() == track);
    }

    if (onTrackSelected)
        onTrackSelected(track);
}

void TrackHeaderPanel::addNewTrack()
{
    // Create new track with default name
    int trackNum = audioEngine.getNumTracks() + 1;
    auto track = std::make_unique<Track>("Track " + juce::String(trackNum));

    // Cycle through colors
    static const juce::Colour trackColors[] = {
        juce::Colour(0xff3b82f6),  // Blue
        juce::Colour(0xff10b981),  // Green
        juce::Colour(0xfff59e0b),  // Yellow
        juce::Colour(0xffef4444),  // Red
        juce::Colour(0xff8b5cf6),  // Purple
        juce::Colour(0xffec4899),  // Pink
        juce::Colour(0xff06b6d4),  // Cyan
        juce::Colour(0xfff97316),  // Orange
    };
    int colorIndex = (trackNum - 1) % 8;
    track->setColour(trackColors[colorIndex]);

    audioEngine.addTrack(std::move(track));
    refreshTracks();

    if (onTracksChanged)
        onTracksChanged();
}

void TrackHeaderPanel::deleteTrack(Track* track)
{
    if (!track) return;

    // Don't delete if it's the only track
    if (audioEngine.getNumTracks() <= 1)
        return;

    // Find the track index
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        if (audioEngine.getTrack(i) == track)
        {
            // Clear selection if we're deleting the selected track
            if (selectedTrack == track)
                selectedTrack = nullptr;

            audioEngine.removeTrack(i);
            refreshTracks();

            if (onTracksChanged)
                onTracksChanged();

            break;
        }
    }
}
