#include "TrackHeader.h"

TrackHeader::TrackHeader(Track& track)
    : track(track)
{
    // Track name label
    nameLabel.setText(track.getName(), juce::dontSendNotification);
    nameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    nameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    nameLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    nameLabel.setColour(juce::Label::backgroundWhenEditingColourId, ProgFlowColours::bgTertiary());
    nameLabel.setColour(juce::Label::outlineWhenEditingColourId, ProgFlowColours::accentBlue());
    nameLabel.setColour(juce::Label::textWhenEditingColourId, ProgFlowColours::textPrimary());
    nameLabel.setEditable(true, true, false);
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.onTextChange = [this]() {
        this->track.setName(nameLabel.getText());
    };
    addAndMakeVisible(nameLabel);

    // Synth selector
    synthSelector.addItemList(SynthFactory::getAllSynthNames(), 1);
    synthSelector.setSelectedId(static_cast<int>(track.getSynthType()) + 1, juce::dontSendNotification);
    synthSelector.onChange = [this]() {
        int selectedIndex = synthSelector.getSelectedId() - 1;
        if (selectedIndex >= 0)
        {
            this->track.setSynthType(SynthFactory::getSynthType(selectedIndex));
            // Notify listener to update synth editor
            if (onSynthTypeChanged)
                onSynthTypeChanged(this->track);
        }
    };
    addAndMakeVisible(synthSelector);

    // Mute button
    muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    muteButton.onClick = [this]() {
        this->track.setMuted(!this->track.isMuted());
        updateMuteButtonAppearance();
    };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    soloButton.onClick = [this]() {
        this->track.setSoloed(!this->track.isSoloed());
        updateSoloButtonAppearance();
    };
    addAndMakeVisible(soloButton);

    // Arm button (Record enable)
    armButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    armButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    armButton.setTooltip("Arm track for recording");
    armButton.onClick = [this]() {
        this->track.setArmed(!this->track.isArmed());
        updateArmButtonAppearance();
    };
    addAndMakeVisible(armButton);

    // Automation expand button
    autoButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    autoButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    autoButton.setTooltip("Toggle automation lanes");
    autoButton.onClick = [this]() {
        automationExpanded = !automationExpanded;
        updateAutoButtonAppearance();
        if (onAutomationExpandToggled)
            onAutomationExpandToggled(this->track, automationExpanded);
    };
    addAndMakeVisible(autoButton);

    // Delete button
    deleteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    deleteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    deleteButton.setTooltip("Delete track");
    deleteButton.onClick = [this]() {
        if (onTrackDeleted)
            onTrackDeleted(this->track);
    };
    addAndMakeVisible(deleteButton);

    // Volume knob
    volumeKnob.setLabel("Vol");
    volumeKnob.setRange(0.0f, 2.0f, 0.01f);
    volumeKnob.setDefaultValue(1.0f);
    volumeKnob.setValue(track.getVolume(), juce::dontSendNotification);
    volumeKnob.onValueChange = [this](float value) {
        this->track.setVolume(value);
    };
    addAndMakeVisible(volumeKnob);

    // Pan knob
    panKnob.setLabel("Pan");
    panKnob.setRange(-1.0f, 1.0f, 0.01f);
    panKnob.setDefaultValue(0.0f);
    panKnob.setValue(track.getPan(), juce::dontSendNotification);
    panKnob.onValueChange = [this](float value) {
        this->track.setPan(value);
    };
    addAndMakeVisible(panKnob);

    // Meter
    addAndMakeVisible(meter);

    // Update button states
    updateMuteButtonAppearance();
    updateSoloButtonAppearance();
    updateArmButtonAppearance();
    updateAutoButtonAppearance();

    // Start timer for meter updates
    startTimerHz(30);
}

TrackHeader::~TrackHeader()
{
    stopTimer();
}

void TrackHeader::timerCallback()
{
    meter.setLevel(track.getMeterLevel());
}

void TrackHeader::mouseDown(const juce::MouseEvent& /*event*/)
{
    // Select this track when clicked anywhere on the header
    if (onTrackSelected)
        onTrackSelected(track);
}

void TrackHeader::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(selected ? ProgFlowColours::bgTertiary() : ProgFlowColours::bgSecondary());
    g.fillRect(bounds);

    // Track color indicator (left edge)
    g.setColour(track.getColour());
    g.fillRect(0.0f, 0.0f, 4.0f, bounds.getHeight());

    // Bottom border
    g.setColour(ProgFlowColours::border());
    g.drawLine(0.0f, bounds.getBottom() - 0.5f, bounds.getRight(), bounds.getBottom() - 0.5f);

    // Selection highlight
    if (selected)
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.2f));
        g.fillRect(bounds);
    }
}

void TrackHeader::resized()
{
    auto bounds = getLocalBounds();

    // Skip color indicator area
    bounds.removeFromLeft(8);
    bounds.reduce(6, 6);

    // Row 1: Track name + Delete button (18px)
    auto row1 = bounds.removeFromTop(18);
    deleteButton.setBounds(row1.removeFromRight(18));
    row1.removeFromRight(4);
    nameLabel.setBounds(row1);
    bounds.removeFromTop(4);

    // Row 2: Synth selector (22px)
    auto row2 = bounds.removeFromTop(22);
    synthSelector.setBounds(row2);
    bounds.removeFromTop(6);

    // Row 3: Buttons (M, S, R, A) in a row (20px height)
    auto row3 = bounds.removeFromTop(20);
    int btnSize = 18;
    int btnGap = 2;

    muteButton.setBounds(row3.removeFromLeft(btnSize));
    row3.removeFromLeft(btnGap);
    soloButton.setBounds(row3.removeFromLeft(btnSize));
    row3.removeFromLeft(btnGap);
    armButton.setBounds(row3.removeFromLeft(btnSize));
    row3.removeFromLeft(btnGap);
    autoButton.setBounds(row3.removeFromLeft(btnSize));

    bounds.removeFromTop(4);

    // Row 4: Volume, Pan knobs and Meter (remaining space ~24px)
    auto row4 = bounds;
    int knobSize = 28;  // Smaller knobs for track header

    volumeKnob.setBounds(row4.removeFromLeft(knobSize + 6).withSizeKeepingCentre(knobSize, row4.getHeight()));
    panKnob.setBounds(row4.removeFromLeft(knobSize + 6).withSizeKeepingCentre(knobSize, row4.getHeight()));

    // Meter takes remaining space
    row4.removeFromLeft(4);
    meter.setBounds(row4);
}

void TrackHeader::setSelected(bool isSelected)
{
    if (selected != isSelected)
    {
        selected = isSelected;
        repaint();
    }
}

void TrackHeader::updateMuteButtonAppearance()
{
    if (track.isMuted())
    {
        muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentRed());
        muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    }
    else
    {
        muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    muteButton.repaint();
}

void TrackHeader::updateSoloButtonAppearance()
{
    if (track.isSoloed())
    {
        soloButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xfffbbf24));  // Yellow
        soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::bgPrimary());
    }
    else
    {
        soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    soloButton.repaint();
}

void TrackHeader::updateArmButtonAppearance()
{
    if (track.isArmed())
    {
        armButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentRed());
        armButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    }
    else
    {
        armButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        armButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    armButton.repaint();
}

void TrackHeader::updateAutoButtonAppearance()
{
    if (automationExpanded)
    {
        autoButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
        autoButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    }
    else
    {
        autoButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        autoButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    autoButton.repaint();
}
