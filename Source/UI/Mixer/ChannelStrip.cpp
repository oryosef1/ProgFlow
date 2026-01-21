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

    nameLabel.setText("MASTER", juce::dontSendNotification);
    nameLabel.setFont(juce::Font(10.0f, juce::Font::bold));

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
    // Name label (Saturn: centered, bold)
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    nameLabel.setFont(juce::Font(11.0f, juce::Font::bold));
    addAndMakeVisible(nameLabel);

    // Pan knob (Saturn style)
    panKnob.setLabel("Pan");
    panKnob.setRange(-1.0f, 1.0f, 0.01f);
    panKnob.setDefaultValue(0.0f);
    panKnob.setTooltipText("Stereo pan position");
    addAndMakeVisible(panKnob);

    // Mute button (Saturn: gold when active)
    muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
    muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    muteButton.setTooltip("Mute track");
    addAndMakeVisible(muteButton);

    // Solo button (Saturn: cyan when active)
    soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
    soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    soloButton.setTooltip("Solo track");
    addAndMakeVisible(soloButton);

    // Volume fader (vertical slider, Saturn style)
    volumeFader.setSliderStyle(juce::Slider::LinearVertical);
    volumeFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    volumeFader.setRange(0.0, 2.0, 0.01);
    volumeFader.setValue(1.0);
    volumeFader.setDoubleClickReturnValue(true, 1.0);
    volumeFader.setColour(juce::Slider::thumbColourId, ProgFlowColours::textPrimary());
    volumeFader.setColour(juce::Slider::trackColourId, ProgFlowColours::accentBlue());
    volumeFader.setColour(juce::Slider::backgroundColourId, ProgFlowColours::bgPrimary());
    volumeFader.setColour(juce::Slider::textBoxTextColourId, ProgFlowColours::textSecondary());
    volumeFader.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    volumeFader.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    volumeFader.setTextValueSuffix(" dB");
    volumeFader.setSkewFactorFromMidPoint(1.0);  // Center at unity gain
    volumeFader.setTooltip("Track volume");
    addAndMakeVisible(volumeFader);

    // Meters (Saturn: LED style)
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
    auto bounds = getLocalBounds().reduced(2).toFloat();

    // Saturn design: card background with gradient
    juce::ColourGradient gradient(
        ProgFlowColours::surfaceBg(),
        0.0f, 0.0f,
        ProgFlowColours::bgSecondary(),
        0.0f, bounds.getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, 6.0f);

    // Subtle border
    g.setColour(juce::Colour(0x0dffffff));  // 5% white
    g.drawRoundedRectangle(bounds, 6.0f, 1.0f);

    // Track color indicator at top (inside card)
    if (track)
    {
        g.setColour(track->getColour());
        g.fillRoundedRectangle(bounds.getX() + 6.0f, bounds.getY() + 6.0f,
                               bounds.getWidth() - 12.0f, 4.0f, 2.0f);
    }
    else if (isMaster)
    {
        // Master uses accent color
        g.setColour(ProgFlowColours::accentBlue());
        g.fillRoundedRectangle(bounds.getX() + 6.0f, bounds.getY() + 6.0f,
                               bounds.getWidth() - 12.0f, 4.0f, 2.0f);
    }

    // Selection highlight
    if (selected)
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.15f));
        g.fillRoundedRectangle(bounds, 6.0f);

        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, 6.0f, 1.5f);
    }
}

void ChannelStrip::resized()
{
    auto bounds = getLocalBounds().reduced(6);

    // Color bar + name at top (26px)
    auto topBounds = bounds.removeFromTop(26);
    topBounds.removeFromTop(8);  // Space for color bar
    nameLabel.setBounds(topBounds);

    bounds.removeFromTop(2);

    // Meters at bottom (70px)
    auto meterBounds = bounds.removeFromBottom(70);
    int meterWidth = 10;
    int totalMeterWidth = meterWidth * 2 + 3;
    int meterX = (meterBounds.getWidth() - totalMeterWidth) / 2 + meterBounds.getX();
    meterL.setBounds(meterX, meterBounds.getY(), meterWidth, meterBounds.getHeight());
    meterR.setBounds(meterX + meterWidth + 3, meterBounds.getY(), meterWidth, meterBounds.getHeight());

    bounds.removeFromBottom(4);

    if (!isMaster)
    {
        // Pan knob (36px)
        panKnob.setBounds(bounds.removeFromTop(36).reduced(4, 0));
        bounds.removeFromTop(2);

        // Mute/Solo buttons (24px row)
        auto buttonRow = bounds.removeFromTop(24);
        buttonRow.reduce(4, 0);
        int buttonWidth = (buttonRow.getWidth() - 4) / 2;
        soloButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(4);
        muteButton.setBounds(buttonRow);
        bounds.removeFromTop(4);
    }
    else
    {
        // Extra space for master (no pan/mute/solo)
        bounds.removeFromTop(66);
    }

    // Volume fader fills the rest
    volumeFader.setBounds(bounds.reduced(4, 0));
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
        // Saturn design: Mute = gold (warning)
        muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentOrange());
        muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::bgPrimary());
    }
    else
    {
        muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
        muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    muteButton.repaint();
}

void ChannelStrip::updateSoloButtonAppearance()
{
    if (!track) return;

    if (track->isSoloed())
    {
        // Saturn design: Solo = cyan (positive)
        soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentGreen());
        soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::bgPrimary());
    }
    else
    {
        soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
        soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    }
    soloButton.repaint();
}
