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
            if (onSynthTypeChanged)
                onSynthTypeChanged(this->track);
        }
    };
    addAndMakeVisible(synthSelector);

    // Mute button
    muteButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
    muteButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    muteButton.setTooltip("Mute track");
    muteButton.onClick = [this]() {
        this->track.setMuted(!this->track.isMuted());
        updateMuteButtonAppearance();
    };
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
    soloButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    soloButton.setTooltip("Solo track");
    soloButton.onClick = [this]() {
        this->track.setSoloed(!this->track.isSoloed());
        updateSoloButtonAppearance();
    };
    addAndMakeVisible(soloButton);

    // Arm button (Record enable)
    armButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
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

    // Volume slider (horizontal)
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.setRange(0.0, 2.0, 0.01);
    volumeSlider.setValue(track.getVolume(), juce::dontSendNotification);
    volumeSlider.setColour(juce::Slider::trackColourId, ProgFlowColours::accentBlue());
    volumeSlider.setColour(juce::Slider::backgroundColourId, ProgFlowColours::bgTertiary());
    volumeSlider.setColour(juce::Slider::thumbColourId, ProgFlowColours::textPrimary());
    volumeSlider.onValueChange = [this]() {
        this->track.setVolume(static_cast<float>(volumeSlider.getValue()));
    };
    volumeSlider.setTooltip("Track volume");
    addAndMakeVisible(volumeSlider);

    // Volume label
    volumeLabel.setText("Vol", juce::dontSendNotification);
    volumeLabel.setFont(juce::Font(9.0f));
    volumeLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    addAndMakeVisible(volumeLabel);

    // Pan slider (horizontal, small)
    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setValue(track.getPan(), juce::dontSendNotification);
    panSlider.setColour(juce::Slider::trackColourId, ProgFlowColours::accentBlue());
    panSlider.setColour(juce::Slider::backgroundColourId, ProgFlowColours::bgTertiary());
    panSlider.setColour(juce::Slider::thumbColourId, ProgFlowColours::textPrimary());
    panSlider.onValueChange = [this]() {
        this->track.setPan(static_cast<float>(panSlider.getValue()));
    };
    panSlider.setTooltip("Pan (left/right)");
    addAndMakeVisible(panSlider);

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
    if (onTrackSelected)
        onTrackSelected(track);
}

void TrackHeader::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(2);

    // Card background with gradient (Saturn design)
    juce::ColourGradient gradient(
        ProgFlowColours::surfaceBg(),
        0.0f, 0.0f,
        ProgFlowColours::bgSecondary(),
        0.0f, static_cast<float>(bounds.getHeight()),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Subtle border
    g.setColour(juce::Colour(0x0dffffff));  // 5% white
    g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.0f);

    // Selection highlight
    if (selected)
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.15f));
        g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

        // Accent border when selected
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.5f));
        g.drawRoundedRectangle(bounds.toFloat(), 6.0f, 1.5f);
    }

    // Track color indicator dot (top left inside card)
    g.setColour(track.getColour());
    g.fillEllipse(static_cast<float>(bounds.getX() + 8),
                  static_cast<float>(bounds.getY() + 10),
                  8.0f, 8.0f);
}

void TrackHeader::resized()
{
    auto bounds = getLocalBounds().reduced(2);  // Card margin
    bounds.reduce(8, 6);  // Internal padding

    // Skip color dot area
    bounds.removeFromLeft(14);

    // Row 1: Track name + Delete button (18px)
    auto row1 = bounds.removeFromTop(18);
    deleteButton.setBounds(row1.removeFromRight(18));
    row1.removeFromRight(4);
    nameLabel.setBounds(row1);
    bounds.removeFromTop(4);

    // Row 2: Synth selector (22px)
    auto row2 = bounds.removeFromTop(22);
    synthSelector.setBounds(row2);
    bounds.removeFromTop(4);

    // Row 3: Buttons (S, M, R, A) in a row (20px height)
    auto row3 = bounds.removeFromTop(20);
    int btnSize = 20;
    int btnGap = 3;

    soloButton.setBounds(row3.removeFromLeft(btnSize));
    row3.removeFromLeft(btnGap);
    muteButton.setBounds(row3.removeFromLeft(btnSize));
    row3.removeFromLeft(btnGap);
    armButton.setBounds(row3.removeFromLeft(btnSize));
    row3.removeFromLeft(btnGap);
    autoButton.setBounds(row3.removeFromLeft(btnSize));

    bounds.removeFromTop(4);

    // Row 4: Volume slider + meter
    auto row4 = bounds;

    // Meter on right
    meter.setBounds(row4.removeFromRight(12));
    row4.removeFromRight(6);

    // Pan slider (small, right side before meter)
    panSlider.setBounds(row4.removeFromRight(40).withHeight(14).withY(row4.getY() + 2));
    row4.removeFromRight(4);

    // Volume label + slider
    volumeLabel.setBounds(row4.removeFromLeft(20).withHeight(14));
    volumeSlider.setBounds(row4.withHeight(16).withY(row4.getY()));
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

void TrackHeader::updateSoloButtonAppearance()
{
    if (track.isSoloed())
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

void TrackHeader::updateArmButtonAppearance()
{
    if (track.isArmed())
    {
        // Saturn design: Record/Arm = coral (negative)
        armButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentRed());
        armButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::bgPrimary());
    }
    else
    {
        armButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
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
