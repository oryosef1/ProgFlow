#include "TrackHeaderPanel.h"

TrackHeaderPanel::TrackHeaderPanel(AudioEngine& audioEngine)
    : audioEngine(audioEngine)
{
    // Title label (Saturn design: uppercase, muted color)
    titleLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    titleLabel.setText("TRACKS", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    addAndMakeVisible(titleLabel);

    // Add track button (Saturn design: accent color, rounded)
    addTrackButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
    addTrackButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    addTrackButton.setTooltip("Add new track");
    addTrackButton.onClick = [this]() { addNewTrack(); };
    addAndMakeVisible(addTrackButton);

    // Viewport for scrollable track list
    trackListContainer = std::make_unique<juce::Component>();
    viewport.setViewedComponent(trackListContainer.get(), false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(6);
    viewport.setColour(juce::ScrollBar::thumbColourId, ProgFlowColours::textMuted());
    addAndMakeVisible(viewport);

    // Master meters (Saturn design)
    masterLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    masterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(masterLabel);
    addAndMakeVisible(masterMeterL);
    addAndMakeVisible(masterMeterR);

    // Home button to go back to project selection
    homeButton.setButtonText(juce::String::fromUTF8("⌂")); // House icon
    homeButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
    homeButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    homeButton.setTooltip("Back to project selection");
    homeButton.onClick = [this]() {
        if (onBackToProjectSelection)
            onBackToProjectSelection();
    };
    addAndMakeVisible(homeButton);

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
    // Main background
    g.fillAll(ProgFlowColours::bgPrimary());

    // Header area with subtle card styling
    auto headerRect = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(getWidth()),
                                              static_cast<float>(headerHeight));
    g.setColour(ProgFlowColours::bgSecondary());
    g.fillRect(headerRect);

    // Subtle border below header
    g.setColour(ProgFlowColours::border());
    g.drawLine(0.0f, static_cast<float>(headerHeight),
               static_cast<float>(getWidth()), static_cast<float>(headerHeight));

    // Master section background
    int masterY = getHeight() - masterHeight;
    g.setColour(ProgFlowColours::bgSecondary());
    g.fillRect(0, masterY, getWidth(), masterHeight);

    // Border above master section
    g.setColour(ProgFlowColours::border());
    g.drawLine(0.0f, static_cast<float>(masterY),
               static_cast<float>(getWidth()), static_cast<float>(masterY));
}

void TrackHeaderPanel::resized()
{
    auto bounds = getLocalBounds();

    // Header area
    auto headerBounds = bounds.removeFromTop(headerHeight);
    headerBounds.reduce(8, 6);
    addTrackButton.setBounds(headerBounds.removeFromRight(24));
    headerBounds.removeFromRight(8);
    titleLabel.setBounds(headerBounds);

    // Master section at bottom
    auto masterBounds = bounds.removeFromBottom(masterHeight);
    masterBounds.reduce(8, 4);

    // Home button on top
    homeButton.setBounds(masterBounds.removeFromTop(22).reduced(2, 0));
    masterBounds.removeFromTop(2);

    // Master label
    masterLabel.setBounds(masterBounds.removeFromTop(12));
    masterBounds.removeFromTop(2);

    // Master meters (centered)
    int meterWidth = 14;
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

    // Cycle through Saturn-themed colors
    static const juce::Colour trackColors[] = {
        juce::Colour(0xff9d7cd8),  // Purple (Saturn accent)
        juce::Colour(0xff7dcfff),  // Cyan
        juce::Colour(0xffe0af68),  // Gold
        juce::Colour(0xfff7768e),  // Coral
        juce::Colour(0xff7aa2f7),  // Blue
        juce::Colour(0xff9ece6a),  // Green
        juce::Colour(0xffbb9af7),  // Light purple
        juce::Colour(0xffff9e64),  // Orange
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
