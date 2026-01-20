#include "ChannelStrip.h"

// Track channel strip constructor
ChannelStrip::ChannelStrip(Track& track)
    : track(&track), isMaster(false)
{
    setupComponents();

    // Set track-specific values
    nameLabel.setText(track.getName(), juce::dontSendNotification);
    panKnob.setValue(track.getPan(), juce::dontSendNotification);
    volumeFader.setValue(track.getVolume(), juce::dontSendNotification);

    // Track-specific callbacks
    panKnob.onValueChange = [this](float value) {
        if (this->track)
            this->track->setPan(value);
    };

    volumeFader.onValueChange = [this] {
        if (this->track)
            this->track->setVolume(static_cast<float>(volumeFader.getValue()));
    };

    muteButton.onClick = [this]() {
        if (this->track)
        {
            this->track->setMuted(!this->track->isMuted());
            updateMuteButtonAppearance();
        }
    };

    soloButton.onClick = [this]() {
        if (this->track)
        {
            this->track->setSoloed(!this->track->isSoloed());
            updateSoloButtonAppearance();
        }
    };

    updateMuteButtonAppearance();
    updateSoloButtonAppearance();

    startTimerHz(30);
}

// Master channel strip constructor
ChannelStrip::ChannelStrip(AudioEngine& engine)
    : audioEngine(&engine), isMaster(true)
{
    setupComponents();

    nameLabel.setText("Master", juce::dontSendNotification);

    // Master doesn't have pan, mute, solo - hide them
    panKnob.setVisible(false);
    muteButton.setVisible(false);
    soloButton.setVisible(false);

    // Master volume control
    volumeFader.setValue(engine.getMasterVolume(), juce::dontSendNotification);
    volumeFader.onValueChange = [this] {
        if (this->audioEngine)
            this->audioEngine->setMasterVolume(static_cast<float>(volumeFader.getValue()));
    };

    startTimerHz(30);
}

void ChannelStrip::setupComponents()
{
    // Name label
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    addAndMakeVisible(nameLabel);

    // Pan knob
    panKnob.setLabel("Pan");
    panKnob.setRange(-1.0f, 1.0f, 0.01f);
    panKnob.setDefaultValue(0.0f);
    addAndMakeVisible(panKnob);

    // Mute button
    muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    addAndMakeVisible(soloButton);

    // Volume fader (vertical slider)
    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeFader.setRange(0.0, 2.0, 0.01);
    volumeFader.setValue(1.0);
    volumeFader.setDoubleClickReturnValue(true, 1.0);
    volumeFader.setColour(juce::Slider::thumbColourId, ProgFlowColours::accentBlue());
    volumeFader.setColour(juce::Slider::trackColourId, ProgFlowColours::bgTertiary());
    volumeFader.setColour(juce::Slider::backgroundColourId, ProgFlowColours::bgPrimary());
    addAndMakeVisible(volumeFader);

    // Meters
    addAndMakeVisible(meterL);
    addAndMakeVisible(meterR);
}

ChannelStrip::~ChannelStrip()
{
    stopTimer();
}

void ChannelStrip::timerCallback()
{
    if (track)
    {
        float level = track->getMeterLevel();
        meterL.setLevel(level);
        meterR.setLevel(level);  // Mono for now
    }
    else if (audioEngine)
    {
        meterL.setLevel(audioEngine->getMasterLevelL());
        meterR.setLevel(audioEngine->getMasterLevelR());
    }
}

void ChannelStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(selected ? ProgFlowColours::bgTertiary() : ProgFlowColours::bgSecondary());
    g.fillRoundedRectangle(bounds, 4.0f);

    // Track color indicator at top
    if (track)
    {
        g.setColour(track->getColour());
        g.fillRoundedRectangle(bounds.getX() + 4.0f, bounds.getY() + 4.0f,
                               bounds.getWidth() - 8.0f, 4.0f, 2.0f);
    }
    else if (isMaster)
    {
        g.setColour(ProgFlowColours::accentBlue());
        g.fillRoundedRectangle(bounds.getX() + 4.0f, bounds.getY() + 4.0f,
                               bounds.getWidth() - 8.0f, 4.0f, 2.0f);
    }

    // Border
    g.setColour(ProgFlowColours::border());
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void ChannelStrip::resized()
{
    auto bounds = getLocalBounds().reduced(4);

    // Color bar + name at top (28px)
    auto topBounds = bounds.removeFromTop(28);
    topBounds.removeFromTop(8);  // Space for color bar
    nameLabel.setBounds(topBounds);

    bounds.removeFromTop(4);

    // Pan knob (40px)
    if (!isMaster)
    {
        panKnob.setBounds(bounds.removeFromTop(40).reduced(8, 0));
        bounds.removeFromTop(4);

        // Mute/Solo buttons (28px row)
        auto buttonRow = bounds.removeFromTop(28);
        int buttonWidth = (buttonRow.getWidth() - 4) / 2;
        muteButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(4);
        soloButton.setBounds(buttonRow);
        bounds.removeFromTop(4);
    }
    else
    {
        // Extra space for master (no pan/mute/solo)
        bounds.removeFromTop(80);
    }

    // Meters at bottom (rest of height after fader)
    auto meterBounds = bounds.removeFromBottom(80);
    int meterWidth = 12;
    int totalMeterWidth = meterWidth * 2 + 4;
    int meterX = (meterBounds.getWidth() - totalMeterWidth) / 2;
    meterL.setBounds(meterX, meterBounds.getY(), meterWidth, meterBounds.getHeight());
    meterR.setBounds(meterX + meterWidth + 4, meterBounds.getY(), meterWidth, meterBounds.getHeight());

    bounds.removeFromBottom(4);

    // Volume fader fills the middle
    volumeFader.setBounds(bounds.reduced(8, 0));
}

void ChannelStrip::setSelected(bool isSelected)
{
    if (selected != isSelected)
    {
        selected = isSelected;
        repaint();
    }
}

void ChannelStrip::updateMuteButtonAppearance()
{
    if (!track) return;

    if (track->isMuted())
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

void ChannelStrip::updateSoloButtonAppearance()
{
    if (!track) return;

    if (track->isSoloed())
    {
        soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentOrange());
        soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::bgPrimary());
    }
    else
    {
        soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
        soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    soloButton.repaint();
}
