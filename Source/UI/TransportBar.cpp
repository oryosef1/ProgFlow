#include "TransportBar.h"

TransportBar::TransportBar(AudioEngine& engine)
    : audioEngine(engine)
{
    // Play button - completely transparent, icons drawn in paint()
    playButton.setButtonText("");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x00000000));
    playButton.setClickingTogglesState(true);
    playButton.onClick = [this] { playClicked(); };
    addAndMakeVisible(playButton);

    // Stop button - completely transparent
    stopButton.setButtonText("");
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    stopButton.onClick = [this] { stopClicked(); };
    addAndMakeVisible(stopButton);

    // Record button - completely transparent
    recordButton.setButtonText("");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x00000000));
    recordButton.setClickingTogglesState(true);
    addAndMakeVisible(recordButton);

    // BPM label
    bpmLabel.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    addAndMakeVisible(bpmLabel);

    // BPM slider
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmSlider.setRange(20.0, 300.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setColour(juce::Slider::textBoxTextColourId, ProgFlowColours::textPrimary());
    bpmSlider.setColour(juce::Slider::textBoxBackgroundColourId, ProgFlowColours::bgTertiary());
    bpmSlider.setColour(juce::Slider::textBoxOutlineColourId, ProgFlowColours::border());
    bpmSlider.onValueChange = [this] { bpmChanged(); };
    addAndMakeVisible(bpmSlider);

    // Tap tempo button
    tapTempoButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    tapTempoButton.setColour(juce::TextButton::textColourOnId, ProgFlowColours::textPrimary());
    tapTempoButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    tapTempoButton.onClick = [this] { tapTempoClicked(); };
    tapTempoButton.setTooltip("Tap to set tempo (T key)");
    addAndMakeVisible(tapTempoButton);

    // Time signature selector
    timeSigSelector.addItem("4/4", 1);
    timeSigSelector.addItem("3/4", 2);
    timeSigSelector.addItem("6/8", 3);
    timeSigSelector.addItem("2/4", 4);
    timeSigSelector.addItem("5/4", 5);
    timeSigSelector.addItem("7/8", 6);
    timeSigSelector.setSelectedId(1, juce::dontSendNotification);
    timeSigSelector.setColour(juce::ComboBox::backgroundColourId, ProgFlowColours::bgTertiary());
    timeSigSelector.setColour(juce::ComboBox::textColourId, ProgFlowColours::textPrimary());
    timeSigSelector.setColour(juce::ComboBox::outlineColourId, ProgFlowColours::border());
    timeSigSelector.setTooltip("Time Signature");
    timeSigSelector.onChange = [this] { timeSigChanged(); };
    addAndMakeVisible(timeSigSelector);

    // Position display
    positionLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    positionLabel.setColour(juce::Label::backgroundColourId, ProgFlowColours::bgTertiary());
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 16.0f, juce::Font::plain));
    addAndMakeVisible(positionLabel);

    // Metronome toggle
    metronomeButton.setColour(juce::ToggleButton::textColourId, ProgFlowColours::textSecondary());
    metronomeButton.setColour(juce::ToggleButton::tickColourId, ProgFlowColours::accentBlue());
    metronomeButton.onClick = [this] {
        audioEngine.setMetronomeEnabled(metronomeButton.getToggleState());
    };
    addAndMakeVisible(metronomeButton);

    // Count-in toggle
    countInButton.setColour(juce::ToggleButton::textColourId, ProgFlowColours::textSecondary());
    countInButton.setColour(juce::ToggleButton::tickColourId, ProgFlowColours::accentBlue());
    countInButton.onClick = [this] {
        audioEngine.setCountInBars(countInButton.getToggleState() ? 1 : 0);
    };
    addAndMakeVisible(countInButton);

    // Loop toggle
    loopButton.setColour(juce::ToggleButton::textColourId, ProgFlowColours::textSecondary());
    loopButton.setColour(juce::ToggleButton::tickColourId, ProgFlowColours::accentBlue());
    addAndMakeVisible(loopButton);

    // CPU meter label
    cpuLabel.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    cpuLabel.setJustificationType(juce::Justification::centredRight);
    cpuLabel.setFont(juce::Font(11.0f));
    addAndMakeVisible(cpuLabel);

    // Start timer for UI updates (60fps)
    startTimerHz(60);
}

TransportBar::~TransportBar()
{
    stopTimer();
}

void TransportBar::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(ProgFlowColours::bgSecondary());

    // Separator line at bottom
    g.setColour(ProgFlowColours::border());
    g.drawLine(0, static_cast<float>(getHeight()) - 1, static_cast<float>(getWidth()), static_cast<float>(getHeight()) - 1);

    // Draw project name (after buttons, before position)
    auto bounds = getLocalBounds().reduced(8);
    int buttonsWidth = 3 * (bounds.getHeight() - 4 + 8) + 20;  // 3 buttons + spacer
    int bpmWidth = 35 + 120 + 10;  // label + slider + spacer
    int nameX = buttonsWidth + bpmWidth + 50;  // After time sig

    g.setColour(ProgFlowColours::textPrimary());
    g.setFont(juce::FontOptions(13.0f));
    juce::String displayName = projectName;
    if (projectDirty) displayName += " *";
    g.drawText(displayName, nameX, 0, 150, getHeight(), juce::Justification::centredLeft);

    // Draw meters (wider, darker background, with peak hold)
    auto meterX = getWidth() - 70;
    auto meterY = 6;
    auto meterWidth = 14;
    auto meterGap = 4;
    auto meterHeight = getHeight() - 12;

    // Left meter background (very dark)
    g.setColour(ProgFlowColours::meterBg());
    g.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(meterY),
                           static_cast<float>(meterWidth), static_cast<float>(meterHeight), 2.0f);

    // Left meter value
    auto levelHeightL = static_cast<int>(meterLevelL * meterHeight);
    auto meterColourL = meterLevelL > 0.8f ? ProgFlowColours::meterRed() :
                        meterLevelL > 0.5f ? ProgFlowColours::meterYellow() : ProgFlowColours::meterGreen();
    g.setColour(meterColourL);
    if (levelHeightL > 0)
        g.fillRoundedRectangle(static_cast<float>(meterX), static_cast<float>(meterY + meterHeight - levelHeightL),
                               static_cast<float>(meterWidth), static_cast<float>(levelHeightL), 2.0f);

    // Left peak hold indicator
    if (peakLevelL > 0.01f)
    {
        auto peakY = meterY + meterHeight - static_cast<int>(peakLevelL * meterHeight);
        auto peakColour = peakLevelL > 0.8f ? ProgFlowColours::meterRed() :
                          peakLevelL > 0.5f ? ProgFlowColours::meterYellow() : ProgFlowColours::meterGreen();
        g.setColour(peakColour);
        g.fillRect(meterX, peakY, meterWidth, 2);
    }

    // Right meter background
    g.setColour(ProgFlowColours::meterBg());
    g.fillRoundedRectangle(static_cast<float>(meterX + meterWidth + meterGap), static_cast<float>(meterY),
                           static_cast<float>(meterWidth), static_cast<float>(meterHeight), 2.0f);

    // Right meter value
    auto levelHeightR = static_cast<int>(meterLevelR * meterHeight);
    auto meterColourR = meterLevelR > 0.8f ? ProgFlowColours::meterRed() :
                        meterLevelR > 0.5f ? ProgFlowColours::meterYellow() : ProgFlowColours::meterGreen();
    g.setColour(meterColourR);
    if (levelHeightR > 0)
        g.fillRoundedRectangle(static_cast<float>(meterX + meterWidth + meterGap), static_cast<float>(meterY + meterHeight - levelHeightR),
                               static_cast<float>(meterWidth), static_cast<float>(levelHeightR), 2.0f);

    // Right peak hold indicator
    if (peakLevelR > 0.01f)
    {
        auto peakY = meterY + meterHeight - static_cast<int>(peakLevelR * meterHeight);
        auto peakColour = peakLevelR > 0.8f ? ProgFlowColours::meterRed() :
                          peakLevelR > 0.5f ? ProgFlowColours::meterYellow() : ProgFlowColours::meterGreen();
        g.setColour(peakColour);
        g.fillRect(meterX + meterWidth + meterGap, peakY, meterWidth, 2);
    }
}

void TransportBar::paintOverChildren(juce::Graphics& g)
{
    // Draw button backgrounds and icons AFTER buttons have painted
    const float cornerRadius = 6.0f;

    // Play button background and icon
    {
        auto bounds = playButton.getBounds().toFloat();
        bool isPlaying = playButton.getToggleState();
        g.setColour(isPlaying ? ProgFlowColours::accentGreen() : ProgFlowColours::bgTertiary());
        g.fillRoundedRectangle(bounds, cornerRadius);

        auto iconBounds = bounds.reduced(12);
        g.setColour(isPlaying ? juce::Colours::white : ProgFlowColours::accentGreen());

        if (isPlaying)
        {
            // Draw pause bars
            float barWidth = iconBounds.getWidth() * 0.25f;
            float gap = iconBounds.getWidth() * 0.2f;
            g.fillRect(iconBounds.getX() + gap, iconBounds.getY(),
                       barWidth, iconBounds.getHeight());
            g.fillRect(iconBounds.getRight() - gap - barWidth, iconBounds.getY(),
                       barWidth, iconBounds.getHeight());
        }
        else
        {
            // Draw play triangle
            juce::Path playPath;
            playPath.addTriangle(iconBounds.getX(), iconBounds.getY(),
                                 iconBounds.getX(), iconBounds.getBottom(),
                                 iconBounds.getRight(), iconBounds.getCentreY());
            g.fillPath(playPath);
        }
    }

    // Stop button background and icon
    {
        auto bounds = stopButton.getBounds().toFloat();
        g.setColour(ProgFlowColours::bgTertiary());
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Draw stop square
        auto iconBounds = bounds.reduced(14);
        g.setColour(ProgFlowColours::textPrimary());
        g.fillRect(iconBounds);
    }

    // Record button background and icon
    {
        auto bounds = recordButton.getBounds().toFloat();
        g.setColour(recordButton.getToggleState() ? ProgFlowColours::accentRed() : ProgFlowColours::bgTertiary());
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Draw record circle
        auto iconBounds = bounds.reduced(12);
        float size = juce::jmin(iconBounds.getWidth(), iconBounds.getHeight());
        g.setColour(recordButton.getToggleState() ? juce::Colours::white : ProgFlowColours::accentRed());
        g.fillEllipse(iconBounds.getCentreX() - size/2, iconBounds.getCentreY() - size/2, size, size);
    }
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds().reduced(8);
    int buttonSize = bounds.getHeight() - 4;

    // Transport buttons (left side) - make them square
    playButton.setBounds(bounds.removeFromLeft(buttonSize + 8).reduced(2));
    stopButton.setBounds(bounds.removeFromLeft(buttonSize + 8).reduced(2));
    recordButton.setBounds(bounds.removeFromLeft(buttonSize + 8).reduced(2));

    bounds.removeFromLeft(20); // Spacer

    // BPM control
    bpmLabel.setBounds(bounds.removeFromLeft(35));
    bpmSlider.setBounds(bounds.removeFromLeft(100));
    tapTempoButton.setBounds(bounds.removeFromLeft(40).reduced(2, 6));

    bounds.removeFromLeft(15); // Spacer

    // Time signature
    timeSigSelector.setBounds(bounds.removeFromLeft(55).reduced(0, 6));

    // Position display
    positionLabel.setBounds(bounds.removeFromLeft(100));

    bounds.removeFromLeft(20); // Spacer

    // Toggles
    metronomeButton.setBounds(bounds.removeFromLeft(70));
    countInButton.setBounds(bounds.removeFromLeft(65));
    loopButton.setBounds(bounds.removeFromLeft(60));

    // CPU label (before meters)
    bounds.removeFromRight(45);  // Space for wider meters
    cpuLabel.setBounds(bounds.removeFromRight(70));
}

void TransportBar::timerCallback()
{
    updatePositionDisplay();

    // Update meter values
    meterLevelL = audioEngine.getMasterLevelL() * 3.0f; // Scale for visibility
    meterLevelR = audioEngine.getMasterLevelR() * 3.0f;
    meterLevelL = juce::jlimit(0.0f, 1.0f, meterLevelL);
    meterLevelR = juce::jlimit(0.0f, 1.0f, meterLevelR);

    // Update peak hold
    if (meterLevelL > peakLevelL)
    {
        peakLevelL = meterLevelL;
        peakHoldCounter = PEAK_HOLD_FRAMES;
    }
    if (meterLevelR > peakLevelR)
    {
        peakLevelR = meterLevelR;
        peakHoldCounter = PEAK_HOLD_FRAMES;
    }

    // Decay peak after hold period
    if (peakHoldCounter > 0)
    {
        peakHoldCounter--;
    }
    else
    {
        peakLevelL *= 0.95f;
        peakLevelR *= 0.95f;
    }

    // Update CPU usage
    if (deviceManager != nullptr)
    {
        cpuUsage = static_cast<float>(deviceManager->getCpuUsage());
        int cpuPercent = static_cast<int>(cpuUsage * 100.0f);

        // Update label with color coding
        juce::String cpuText = "CPU: " + juce::String(cpuPercent) + "%";
        cpuLabel.setText(cpuText, juce::dontSendNotification);

        // Color code based on usage
        juce::Colour cpuColour = ProgFlowColours::textSecondary();
        if (cpuUsage > 0.8f)
            cpuColour = ProgFlowColours::meterRed();
        else if (cpuUsage > 0.5f)
            cpuColour = ProgFlowColours::meterYellow();
        else if (cpuUsage > 0.3f)
            cpuColour = ProgFlowColours::accentGreen();

        cpuLabel.setColour(juce::Label::textColourId, cpuColour);
    }

    // Update play button state to sync with engine
    bool isPlaying = audioEngine.isPlaying();
    if (playButton.getToggleState() != isPlaying)
    {
        playButton.setToggleState(isPlaying, juce::dontSendNotification);
    }

    repaint();
}

void TransportBar::playClicked()
{
    if (audioEngine.isPlaying())
    {
        audioEngine.stop();
    }
    else
    {
        // Use count-in if enabled
        if (audioEngine.getCountInBars() > 0)
            audioEngine.playWithCountIn();
        else
            audioEngine.play();
    }
}

void TransportBar::stopClicked()
{
    audioEngine.stop();
}

void TransportBar::bpmChanged()
{
    audioEngine.setBpm(bpmSlider.getValue());
}

void TransportBar::tapTempoClicked()
{
    juce::int64 now = juce::Time::currentTimeMillis();

    // Reset if too much time has passed since last tap
    if (!tapTimes.empty() && (now - tapTimes.back()) > TAP_RESET_MS)
    {
        tapTimes.clear();
    }

    // Add this tap
    tapTimes.push_back(now);

    // Keep only the last MAX_TAPS taps
    if (tapTimes.size() > MAX_TAPS)
    {
        tapTimes.erase(tapTimes.begin());
    }

    // Need at least 2 taps to calculate tempo
    if (tapTimes.size() >= 2)
    {
        // Calculate average interval
        juce::int64 totalInterval = 0;
        for (size_t i = 1; i < tapTimes.size(); ++i)
        {
            totalInterval += tapTimes[i] - tapTimes[i - 1];
        }

        double avgIntervalMs = static_cast<double>(totalInterval) / static_cast<double>(tapTimes.size() - 1);

        // Convert to BPM (60000ms / interval = BPM)
        double bpm = 60000.0 / avgIntervalMs;

        // Clamp to valid range and update
        bpm = juce::jlimit(20.0, 300.0, bpm);

        bpmSlider.setValue(bpm, juce::sendNotification);
        DBG("Tap tempo: " << bpm << " BPM (from " << tapTimes.size() << " taps)");
    }
}

void TransportBar::updatePositionDisplay()
{
    double positionInBeats = audioEngine.getPositionInBeats();

    // Get current time signature from TimeSignatureTrack
    auto timeSig = audioEngine.getCurrentTimeSignature();
    int beatsPerBar = timeSig.numerator;

    int totalBeats = static_cast<int>(positionInBeats);
    int bars = (totalBeats / beatsPerBar) + 1;
    int beats = (totalBeats % beatsPerBar) + 1;
    int ticks = static_cast<int>((positionInBeats - totalBeats) * 960);

    juce::String posText = juce::String::formatted("%d:%d:%03d", bars, beats, ticks);
    positionLabel.setText(posText, juce::dontSendNotification);

    // Update time signature selector to match current time sig
    juce::String timeSigText = juce::String(timeSig.numerator) + "/" + juce::String(timeSig.denominator);
    for (int i = 0; i < timeSigSelector.getNumItems(); ++i)
    {
        if (timeSigSelector.getItemText(i) == timeSigText)
        {
            if (timeSigSelector.getSelectedId() != i + 1)
                timeSigSelector.setSelectedId(i + 1, juce::dontSendNotification);
            break;
        }
    }

    // Update BPM slider to reflect current tempo (which may be automated)
    double currentTempo = audioEngine.getCurrentTempo();
    if (std::abs(bpmSlider.getValue() - currentTempo) > 0.5)
    {
        bpmSlider.setValue(currentTempo, juce::dontSendNotification);
    }
}

void TransportBar::timeSigChanged()
{
    juce::String selected = timeSigSelector.getText();

    // Parse the time signature string (e.g., "4/4" -> numerator=4, denominator=4)
    int slashPos = selected.indexOf("/");
    if (slashPos > 0)
    {
        int numerator = selected.substring(0, slashPos).getIntValue();
        int denominator = selected.substring(slashPos + 1).getIntValue();

        if (numerator > 0 && denominator > 0)
        {
            audioEngine.getTimeSignatureTrack().setInitialTimeSignature(numerator, denominator);
        }
    }
}
