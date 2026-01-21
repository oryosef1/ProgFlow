#include "TransportBar.h"

TransportBar::TransportBar(AudioEngine& engine)
    : audioEngine(engine)
{
    // Play button - completely transparent, icons drawn in paint()
    playButton.setButtonText("");
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    playButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x00000000));
    playButton.setClickingTogglesState(true);
    playButton.setTooltip("Play/Pause (Space)");
    playButton.onClick = [this] { playClicked(); };
    addAndMakeVisible(playButton);

    // Stop button - completely transparent
    stopButton.setButtonText("");
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    stopButton.setTooltip("Stop");
    stopButton.onClick = [this] { stopClicked(); };
    addAndMakeVisible(stopButton);

    // Record button - completely transparent
    recordButton.setButtonText("");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x00000000));
    recordButton.setClickingTogglesState(true);
    recordButton.setTooltip("Record (R)");
    recordButton.onClick = [this] {
        bool isRecording = recordButton.getToggleState();

        // Arm/disarm all tracks for recording
        for (int i = 0; i < audioEngine.getNumTracks(); ++i)
        {
            if (auto* track = audioEngine.getTrack(i))
                track->setArmed(isRecording);
        }

        if (isRecording)
        {
            // Starting recording - also start playback with count-in if enabled
            if (!audioEngine.isPlaying())
            {
                if (countInButton.getToggleState())
                {
                    audioEngine.playWithCountIn();
                }
                else
                {
                    audioEngine.play();
                }
                playButton.setToggleState(true, juce::dontSendNotification);
            }
        }
        else
        {
            // Stopping recording - also stop playback
            audioEngine.stop();
            playButton.setToggleState(false, juce::dontSendNotification);
        }

        repaint();
    };
    addAndMakeVisible(recordButton);

    // BPM label (Saturn: muted color, uppercase)
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    bpmLabel.setFont(juce::Font(10.0f, juce::Font::bold));
    addAndMakeVisible(bpmLabel);

    // BPM slider
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 45, 20);
    bpmSlider.setRange(20.0, 300.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setColour(juce::Slider::textBoxTextColourId, ProgFlowColours::textPrimary());
    bpmSlider.setColour(juce::Slider::textBoxBackgroundColourId, ProgFlowColours::bgTertiary());
    bpmSlider.setColour(juce::Slider::textBoxOutlineColourId, ProgFlowColours::border());
    bpmSlider.setColour(juce::Slider::trackColourId, ProgFlowColours::accentBlue());
    bpmSlider.setColour(juce::Slider::backgroundColourId, ProgFlowColours::bgTertiary());
    bpmSlider.onValueChange = [this] { bpmChanged(); };
    addAndMakeVisible(bpmSlider);

    // Tap tempo button (Saturn styling)
    tapTempoButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
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
    timeSigSelector.setTooltip("Time Signature");
    timeSigSelector.onChange = [this] { timeSigChanged(); };
    addAndMakeVisible(timeSigSelector);

    // Position display (Saturn: monospace, card background)
    positionLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    positionLabel.setColour(juce::Label::backgroundColourId, ProgFlowColours::bgTertiary());
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 14.0f, juce::Font::bold));
    addAndMakeVisible(positionLabel);

    // Metronome toggle (Saturn style)
    metronomeButton.setColour(juce::ToggleButton::textColourId, ProgFlowColours::textSecondary());
    metronomeButton.setColour(juce::ToggleButton::tickColourId, ProgFlowColours::accentBlue());
    metronomeButton.onClick = [this] {
        audioEngine.setMetronomeEnabled(metronomeButton.getToggleState());
    };
    metronomeButton.setTooltip("Enable metronome");
    addAndMakeVisible(metronomeButton);

    // Count-in toggle
    countInButton.setColour(juce::ToggleButton::textColourId, ProgFlowColours::textSecondary());
    countInButton.setColour(juce::ToggleButton::tickColourId, ProgFlowColours::accentBlue());
    countInButton.onClick = [this] {
        audioEngine.setCountInBars(countInButton.getToggleState() ? 1 : 0);
    };
    countInButton.setTooltip("Count-in before recording");
    addAndMakeVisible(countInButton);

    // Loop toggle
    loopButton.setColour(juce::ToggleButton::textColourId, ProgFlowColours::textSecondary());
    loopButton.setColour(juce::ToggleButton::tickColourId, ProgFlowColours::accentBlue());
    loopButton.setTooltip("Enable loop playback (L)");
    addAndMakeVisible(loopButton);

    // CPU meter label (Saturn: right aligned, color coded)
    cpuLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    cpuLabel.setJustificationType(juce::Justification::centredRight);
    cpuLabel.setFont(juce::Font(10.0f));
    addAndMakeVisible(cpuLabel);

    // Home button (back to project selection)
    homeButton.setButtonText(juce::String::fromUTF8("⌂"));
    homeButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::surfaceBg());
    homeButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textSecondary());
    homeButton.setTooltip("Back to project selection");
    homeButton.onClick = [this]() {
        if (onBackToProjectSelection)
            onBackToProjectSelection();
    };
    addAndMakeVisible(homeButton);

    // Project name label (clickable to rename)
    projectNameLabel.setText("Untitled", juce::dontSendNotification);
    projectNameLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    projectNameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    projectNameLabel.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    projectNameLabel.setJustificationType(juce::Justification::centredLeft);
    projectNameLabel.setTooltip("Click to rename project");
    projectNameLabel.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    projectNameLabel.addMouseListener(this, false);
    addAndMakeVisible(projectNameLabel);

    // Start timer for UI updates (60fps)
    startTimerHz(60);
}

TransportBar::~TransportBar()
{
    stopTimer();
}

void TransportBar::paint(juce::Graphics& g)
{
    // Saturn design: card-like background with subtle gradient
    auto bounds = getLocalBounds().toFloat();

    // Background gradient
    juce::ColourGradient gradient(
        ProgFlowColours::bgSecondary(),
        0.0f, 0.0f,
        ProgFlowColours::bgPrimary(),
        0.0f, bounds.getHeight(),
        false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Subtle border at bottom
    g.setColour(ProgFlowColours::border());
    g.drawLine(0.0f, bounds.getBottom() - 0.5f, bounds.getRight(), bounds.getBottom() - 0.5f);

    // Draw stereo meters (Saturn style: LED segments)
    auto meterX = getWidth() - 60;
    auto meterY = 8;
    auto meterWidth = 12;
    auto meterGap = 3;
    auto meterHeight = getHeight() - 16;

    // Left meter
    drawMeter(g, meterX, meterY, meterWidth, meterHeight, meterLevelL, peakLevelL);

    // Right meter
    drawMeter(g, meterX + meterWidth + meterGap, meterY, meterWidth, meterHeight, meterLevelR, peakLevelR);
}

void TransportBar::drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level, float peak)
{
    // Background
    g.setColour(ProgFlowColours::bgPrimary());
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(width), static_cast<float>(height), 2.0f);

    // Level bar with gradient coloring
    int levelHeight = static_cast<int>(level * height);
    if (levelHeight > 0)
    {
        auto meterColour = level > 0.8f ? ProgFlowColours::accentRed() :
                          level > 0.5f ? ProgFlowColours::accentOrange() : ProgFlowColours::accentGreen();
        g.setColour(meterColour);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y + height - levelHeight),
                               static_cast<float>(width), static_cast<float>(levelHeight), 2.0f);
    }

    // Peak hold indicator
    if (peak > 0.01f)
    {
        int peakY = y + height - static_cast<int>(peak * height);
        auto peakColour = peak > 0.8f ? ProgFlowColours::accentRed() :
                         peak > 0.5f ? ProgFlowColours::accentOrange() : ProgFlowColours::accentGreen();
        g.setColour(peakColour);
        g.fillRect(x, peakY, width, 2);
    }
}

void TransportBar::paintOverChildren(juce::Graphics& g)
{
    // Draw button backgrounds and icons AFTER buttons have painted
    const float cornerRadius = 6.0f;

    // Play button (Saturn: cyan when playing)
    {
        auto bounds = playButton.getBounds().toFloat();
        bool isPlaying = playButton.getToggleState();
        g.setColour(isPlaying ? ProgFlowColours::accentGreen() : ProgFlowColours::surfaceBg());
        g.fillRoundedRectangle(bounds, cornerRadius);

        // Subtle border
        g.setColour(juce::Colour(0x15ffffff));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        auto iconBounds = bounds.reduced(10);
        g.setColour(isPlaying ? ProgFlowColours::bgPrimary() : ProgFlowColours::accentGreen());

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

    // Stop button
    {
        auto bounds = stopButton.getBounds().toFloat();
        g.setColour(ProgFlowColours::surfaceBg());
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(juce::Colour(0x15ffffff));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        auto iconBounds = bounds.reduced(12);
        g.setColour(ProgFlowColours::textPrimary());
        g.fillRect(iconBounds);
    }

    // Record button (Saturn: coral when recording)
    {
        auto bounds = recordButton.getBounds().toFloat();
        bool isRecording = recordButton.getToggleState();
        g.setColour(isRecording ? ProgFlowColours::accentRed() : ProgFlowColours::surfaceBg());
        g.fillRoundedRectangle(bounds, cornerRadius);

        g.setColour(juce::Colour(0x15ffffff));
        g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);

        auto iconBounds = bounds.reduced(10);
        float size = juce::jmin(iconBounds.getWidth(), iconBounds.getHeight());
        g.setColour(isRecording ? ProgFlowColours::bgPrimary() : ProgFlowColours::accentRed());
        g.fillEllipse(iconBounds.getCentreX() - size/2, iconBounds.getCentreY() - size/2, size, size);
    }
}

void TransportBar::resized()
{
    auto bounds = getLocalBounds().reduced(8, 6);
    int buttonSize = bounds.getHeight();

    // Transport buttons (left side) - square buttons
    playButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(1));
    bounds.removeFromLeft(4);
    stopButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(1));
    bounds.removeFromLeft(4);
    recordButton.setBounds(bounds.removeFromLeft(buttonSize).reduced(1));

    bounds.removeFromLeft(16); // Spacer

    // BPM control
    bpmLabel.setBounds(bounds.removeFromLeft(30).withHeight(bounds.getHeight()));
    bpmSlider.setBounds(bounds.removeFromLeft(110));
    bounds.removeFromLeft(4);
    tapTempoButton.setBounds(bounds.removeFromLeft(36).reduced(0, 4));

    bounds.removeFromLeft(12); // Spacer

    // Time signature
    timeSigSelector.setBounds(bounds.removeFromLeft(52).reduced(0, 4));

    bounds.removeFromLeft(12); // Spacer

    // Position display (prominent)
    positionLabel.setBounds(bounds.removeFromLeft(90).reduced(0, 2));

    bounds.removeFromLeft(12); // Spacer

    // Project name (clickable)
    projectNameLabel.setBounds(bounds.removeFromLeft(120).reduced(0, 4));

    bounds.removeFromLeft(8); // Spacer

    // Toggles
    metronomeButton.setBounds(bounds.removeFromLeft(65));
    countInButton.setBounds(bounds.removeFromLeft(60));
    loopButton.setBounds(bounds.removeFromLeft(55));

    // Home button (far right, before meters)
    bounds.removeFromRight(35);  // Space for meters
    homeButton.setBounds(bounds.removeFromRight(32).reduced(2, 4));
    bounds.removeFromRight(4);

    // CPU label
    cpuLabel.setBounds(bounds.removeFromRight(60));
}

void TransportBar::timerCallback()
{
    updatePositionDisplay();

    // Update meter values
    meterLevelL = audioEngine.getMasterLevelL() * 3.0f;
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

        juce::String cpuText = "CPU " + juce::String(cpuPercent) + "%";
        cpuLabel.setText(cpuText, juce::dontSendNotification);

        // Color code based on usage (Saturn colors)
        juce::Colour cpuColour = ProgFlowColours::textMuted();
        if (cpuUsage > 0.8f)
            cpuColour = ProgFlowColours::accentRed();
        else if (cpuUsage > 0.5f)
            cpuColour = ProgFlowColours::accentOrange();
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
        if (audioEngine.getCountInBars() > 0)
            audioEngine.playWithCountIn();
        else
            audioEngine.play();
    }
}

void TransportBar::stopClicked()
{
    audioEngine.stop();

    // Disarm all tracks and reset record button
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        if (auto* track = audioEngine.getTrack(i))
            track->setArmed(false);
    }
    recordButton.setToggleState(false, juce::dontSendNotification);
    repaint();
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

    tapTimes.push_back(now);

    if (tapTimes.size() > MAX_TAPS)
    {
        tapTimes.erase(tapTimes.begin());
    }

    if (tapTimes.size() >= 2)
    {
        juce::int64 totalInterval = 0;
        for (size_t i = 1; i < tapTimes.size(); ++i)
        {
            totalInterval += tapTimes[i] - tapTimes[i - 1];
        }

        double avgIntervalMs = static_cast<double>(totalInterval) / static_cast<double>(tapTimes.size() - 1);
        double bpm = juce::jlimit(20.0, 300.0, 60000.0 / avgIntervalMs);

        bpmSlider.setValue(bpm, juce::sendNotification);
    }
}

void TransportBar::updatePositionDisplay()
{
    double positionInBeats = audioEngine.getPositionInBeats();

    auto timeSig = audioEngine.getCurrentTimeSignature();
    int beatsPerBar = timeSig.numerator;

    int totalBeats = static_cast<int>(positionInBeats);
    int bars = (totalBeats / beatsPerBar) + 1;
    int beats = (totalBeats % beatsPerBar) + 1;
    int ticks = static_cast<int>((positionInBeats - totalBeats) * 960);

    juce::String posText = juce::String::formatted("%d:%d:%03d", bars, beats, ticks);
    positionLabel.setText(posText, juce::dontSendNotification);

    // Update time signature selector to match
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

    // Update BPM slider to reflect current tempo
    double currentTempo = audioEngine.getCurrentTempo();
    if (std::abs(bpmSlider.getValue() - currentTempo) > 0.5)
    {
        bpmSlider.setValue(currentTempo, juce::dontSendNotification);
    }
}

void TransportBar::timeSigChanged()
{
    juce::String selected = timeSigSelector.getText();

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

void TransportBar::setProjectName(const juce::String& name)
{
    projectName = name;
    juce::String displayText = projectName;
    if (projectDirty) displayText += " *";
    projectNameLabel.setText(displayText, juce::dontSendNotification);
}

void TransportBar::showRenameDialog()
{
    auto* alertWindow = new juce::AlertWindow("Rename Project",
                                               "Enter a new name for the project:",
                                               juce::MessageBoxIconType::NoIcon);

    alertWindow->addTextEditor("name", projectName, "Project Name:");
    alertWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alertWindow->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, alertWindow](int result)
        {
            if (result == 1)
            {
                juce::String newName = alertWindow->getTextEditorContents("name").trim();
                if (newName.isNotEmpty() && newName != projectName)
                {
                    if (onProjectRename)
                        onProjectRename(newName);
                }
            }
            delete alertWindow;
        }), true);
}

void TransportBar::mouseDown(const juce::MouseEvent& e)
{
    if (e.originalComponent == &projectNameLabel)
    {
        showRenameDialog();
    }
}
