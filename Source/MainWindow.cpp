#include "MainWindow.h"
#include "Core/UndoManager.h"
#include <vector>
#include <cmath>

//==============================================================================
// MainContentComponent
//==============================================================================

MainContentComponent::MainContentComponent()
{
    // Set custom look and feel
    setLookAndFeel(&lookAndFeel);

    // Listen for theme changes
    ThemeManager::getInstance().addListener(this);

    // Initialize audio device manager
    auto result = deviceManager.initialiseWithDefaultDevices(0, 2); // 0 inputs, 2 outputs (stereo)
    if (result.isNotEmpty())
    {
        // Audio device initialization failed
        juce::ignoreUnused(result);
    }

    // Set up audio playback
    audioSourcePlayer.setSource(&audioEngine);
    deviceManager.addAudioCallback(&audioSourcePlayer);

    // Create project manager
    projectManager = std::make_unique<ProjectManager>(audioEngine);
    projectManager->addListener(this);

    // Create transport bar
    transportBar = std::make_unique<TransportBar>(audioEngine);
    transportBar->setAudioDeviceManager(&deviceManager);
    transportBar->onBackToProjectSelection = [this] { showWelcomeScreen(); };
    transportBar->onProjectRename = [this](const juce::String& newName) {
        if (projectManager)
        {
            projectManager->setProjectName(newName);
            transportBar->setProjectName(newName);
            if (parentWindow)
                parentWindow->updateTitle();
        }
    };
    addAndMakeVisible(*transportBar);

    // Create timeline panel
    timelinePanel = std::make_unique<TimelinePanel>(audioEngine);
    timelinePanel->onClipDoubleClicked = [this](MidiClip* clip) {
        openPianoRoll(clip);
    };
    addAndMakeVisible(*timelinePanel);

    // Create piano roll editor (initially hidden)
    pianoRollEditor = std::make_unique<PianoRollEditor>(audioEngine);
    addChildComponent(*pianoRollEditor);

    // Create synth editor (initially for the global synth, will update when track selected)
    synthEditor = std::make_unique<AnalogSynthEditor>(audioEngine.getSynth());
    addAndMakeVisible(*synthEditor);

    // Create track header panel
    trackHeaderPanel = std::make_unique<TrackHeaderPanel>(audioEngine);
    trackHeaderPanel->onTrackSelected = [this](Track* track) {
        selectTrack(track);
    };
    trackHeaderPanel->onTracksChanged = [this]() {
        // Update timeline when tracks are added/removed
        if (timelinePanel)
            timelinePanel->updateTracks();
        if (mixerPanel)
            mixerPanel->refreshTracks();
    };
    trackHeaderPanel->onBackToProjectSelection = [this]() {
        showWelcomeScreen();
    };
    addAndMakeVisible(*trackHeaderPanel);

    // Create mixer panel (initially hidden)
    mixerPanel = std::make_unique<MixerPanel>(audioEngine);
    mixerPanel->onTrackSelected = [this](Track* track) {
        selectTrack(track);
    };
    addChildComponent(*mixerPanel);

    // Initialize plugin host
    pluginHost = std::make_unique<PluginHost>();
    pluginEditorManager = std::make_unique<PluginEditorManager>();

    // Create plugin browser (initially hidden)
    pluginBrowser = std::make_unique<PluginBrowserPanel>(*pluginHost);
    pluginBrowser->onPluginDoubleClicked = [this](const juce::PluginDescription& desc) {
        loadPluginOnSelectedTrack(desc);
    };
    addChildComponent(*pluginBrowser);

    // Add keyboard listener for synth testing
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // Create welcome screen
    welcomeScreen = std::make_unique<WelcomeScreen>();
    welcomeScreen->onNewProject = [this] { newProject(); };
    welcomeScreen->onOpenProject = [this] { openProject(); };
    welcomeScreen->onOpenRecentProject = [this](const juce::String& path) {
        juce::File file(path);
        if (file.existsAsFile())
        {
            projectManager->openProject(file);
        }
    };
    welcomeScreen->setRecentProjects(projectManager->getRecentProjects());
    addAndMakeVisible(*welcomeScreen);

    // Create virtual keyboard (initially hidden)
    virtualKeyboard = std::make_unique<VirtualKeyboardPanel>();
    virtualKeyboard->onNoteOn = [this](int note, float velocity) {
        audioEngine.synthNoteOn(note, velocity);
    };
    virtualKeyboard->onNoteOff = [this](int note) {
        audioEngine.synthNoteOff(note);
    };
    addChildComponent(*virtualKeyboard);

    // Create toast manager (overlay for notifications)
    toastManager = std::make_unique<ToastManager>();
    addAndMakeVisible(*toastManager);

    // Setup resize handle for bottom panel
    resizeHandle.onResize = [this](int deltaY) {
        int newHeight = bottomPanelHeight + deltaY;
        newHeight = juce::jlimit(MIN_BOTTOM_PANEL_HEIGHT, MAX_BOTTOM_PANEL_HEIGHT, newHeight);
        if (newHeight != bottomPanelHeight)
        {
            bottomPanelHeight = newHeight;
            resized();
        }
    };
    addAndMakeVisible(resizeHandle);

    // Initially hide all other UI until welcome screen is dismissed
    transportBar->setVisible(false);
    trackHeaderPanel->setVisible(false);
    timelinePanel->setVisible(false);
    pianoRollEditor->setVisible(false);
    if (synthEditor) synthEditor->setVisible(false);

    // Initialize background animation
    initBackgroundAnimation();
    startTimerHz(30);  // 30fps for smooth animations

    // Set minimum size
    setSize(1400, 900);
}

MainContentComponent::~MainContentComponent()
{
    stopTimer();
    ThemeManager::getInstance().removeListener(this);
    if (projectManager)
        projectManager->removeListener(this);
    removeKeyListener(this);
    deviceManager.removeAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(nullptr);
    setLookAndFeel(nullptr);
}

void MainContentComponent::mouseDown(const juce::MouseEvent&)
{
    // Grab keyboard focus when clicking anywhere in the component
    grabKeyboardFocus();
}

void MainContentComponent::mouseDrag(const juce::MouseEvent&) {}
void MainContentComponent::mouseUp(const juce::MouseEvent&) {}
void MainContentComponent::mouseMove(const juce::MouseEvent&) {}

void MainContentComponent::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());

    // Don't draw borders when showing welcome screen
    if (showingWelcomeScreen)
        return;

    // Draw subtle background animation
    drawBackgroundAnimation(g);

    // Draw borders between panels
    g.setColour(ProgFlowColours::border());

    // Border below transport bar
    g.drawLine(0, 50, static_cast<float>(getWidth()), 50);

    if (mainViewMode == MainViewMode::Arrange)
    {
        int keyboardOffset = showingVirtualKeyboard ? 110 : 0;
        int bottomY = getHeight() - bottomPanelHeight - keyboardOffset - RESIZE_HANDLE_HEIGHT;

        // Border between track list and timeline
        g.drawLine(200, 50, 200, static_cast<float>(bottomY));
    }
    // Mixer view has no internal borders (handled by MixerPanel)
}

void MainContentComponent::resized()
{
    auto bounds = getLocalBounds();

    // Toast manager always fills the component (overlay)
    if (toastManager)
        toastManager->setBounds(bounds);

    // Show welcome screen full size when active
    if (showingWelcomeScreen && welcomeScreen)
    {
        welcomeScreen->setBounds(bounds);
        return;
    }

    // Transport bar at top (50px height)
    transportBar->setBounds(bounds.removeFromTop(50));

    // Virtual keyboard at bottom (if visible)
    if (showingVirtualKeyboard && virtualKeyboard)
    {
        virtualKeyboard->setBounds(bounds.removeFromBottom(110));
        virtualKeyboard->setVisible(true);
    }
    else if (virtualKeyboard)
    {
        virtualKeyboard->setVisible(false);
    }

    if (mainViewMode == MainViewMode::Arrange)
    {
        // Arrange view: Track list | Timeline | Bottom Panel
        mixerPanel->setVisible(false);

        // Plugin browser on right (if showing)
        if (showingPluginBrowser && pluginBrowser)
        {
            pluginBrowser->setBounds(bounds.removeFromRight(300));
            pluginBrowser->setVisible(true);
        }
        else if (pluginBrowser)
        {
            pluginBrowser->setVisible(false);
        }

        // Bottom panel (resizable height) - either synth editor or piano roll
        auto bottomBounds = bounds.removeFromBottom(bottomPanelHeight);

        // Resize handle between timeline and bottom panel
        resizeHandle.setBounds(bounds.removeFromBottom(RESIZE_HANDLE_HEIGHT));
        resizeHandle.setVisible(true);

        if (bottomPanelMode == BottomPanelMode::SynthEditor)
        {
            synthEditor->setBounds(bottomBounds);
            synthEditor->setVisible(true);
            pianoRollEditor->setVisible(false);
        }
        else
        {
            pianoRollEditor->setBounds(bottomBounds);
            pianoRollEditor->setVisible(true);
            synthEditor->setVisible(false);
        }

        // Track list on left (200px width)
        trackHeaderPanel->setBounds(bounds.removeFromLeft(200));
        trackHeaderPanel->setVisible(true);

        // Timeline fills the rest
        timelinePanel->setBounds(bounds);
        timelinePanel->setVisible(true);
    }
    else // Mixer view
    {
        // Hide arrange view components
        trackHeaderPanel->setVisible(false);
        timelinePanel->setVisible(false);
        synthEditor->setVisible(false);
        pianoRollEditor->setVisible(false);
        resizeHandle.setVisible(false);

        // Mixer fills everything below transport
        mixerPanel->setBounds(bounds);
        mixerPanel->setVisible(true);
    }
}

//==============================================================================
// Background Animation
//==============================================================================

void MainContentComponent::initBackgroundAnimation()
{
    bgParticles.clear();
    std::uniform_real_distribution<float> xDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> yDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> vDist(-0.001f, 0.001f);
    std::uniform_real_distribution<float> sizeDist(2.0f, 5.0f);
    std::uniform_real_distribution<float> alphaDist(0.15f, 0.35f);

    // Visible particles with glow
    for (int i = 0; i < 40; ++i)
    {
        Particle p;
        p.x = xDist(rng);
        p.y = yDist(rng);
        p.vx = vDist(rng);
        p.vy = vDist(rng) - 0.0004f; // Slight upward drift
        p.size = sizeDist(rng);
        p.alpha = alphaDist(rng);
        bgParticles.push_back(p);
    }
}

void MainContentComponent::updateBackgroundAnimation()
{
    animationTime += 0.033f; // ~30fps

    for (auto& p : bgParticles)
    {
        p.x += p.vx;
        p.y += p.vy;

        // Wrap around
        if (p.x < 0.0f) p.x += 1.0f;
        if (p.x > 1.0f) p.x -= 1.0f;
        if (p.y < 0.0f) p.y += 1.0f;
        if (p.y > 1.0f) p.y -= 1.0f;

        // Visible alpha pulsing
        p.alpha = 0.15f + 0.12f * std::sin(animationTime * 2.0f + p.x * 10.0f);
    }
}

void MainContentComponent::drawBackgroundAnimation(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    for (const auto& p : bgParticles)
    {
        float x = p.x * bounds.getWidth();
        float y = p.y * bounds.getHeight();

        // Draw particle with subtle glow
        juce::ColourGradient glow(
            ProgFlowColours::accentBlue().withAlpha(p.alpha),
            x, y,
            ProgFlowColours::accentBlue().withAlpha(0.0f),
            x + p.size * 3.0f, y,
            true);

        g.setGradientFill(glow);
        g.fillEllipse(x - p.size, y - p.size, p.size * 2.0f, p.size * 2.0f);
    }

    // Animated waveform visualization at the bottom
    if (mainViewMode == MainViewMode::Arrange)
    {
        float waveY = bounds.getBottom() - 30.0f;
        juce::Path wavePath;

        int numPoints = 80;
        for (int i = 0; i <= numPoints; ++i)
        {
            float nx = static_cast<float>(i) / numPoints;
            float x = nx * bounds.getWidth();

            // Combine sine waves for organic look
            float y = waveY;
            y += std::sin(nx * 8.0f + animationTime * 1.5f) * 12.0f;
            y += std::sin(nx * 12.0f - animationTime * 2.0f) * 6.0f;
            y += std::sin(nx * 20.0f + animationTime * 0.8f) * 3.0f;

            // Fade at edges
            float edgeFade = std::min(nx, 1.0f - nx) * 4.0f;
            edgeFade = juce::jmin(1.0f, edgeFade);
            y = waveY + (y - waveY) * edgeFade;

            if (i == 0)
                wavePath.startNewSubPath(x, y);
            else
                wavePath.lineTo(x, y);
        }

        // Waveform with glow effect
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.08f));
        g.strokePath(wavePath, juce::PathStrokeType(8.0f));
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.2f));
        g.strokePath(wavePath, juce::PathStrokeType(3.0f));
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.4f));
        g.strokePath(wavePath, juce::PathStrokeType(1.5f));
    }
}

void MainContentComponent::timerCallback()
{
    // Only animate when not showing welcome screen (it has its own animation)
    if (!showingWelcomeScreen)
    {
        updateBackgroundAnimation();
        repaint();
    }
}

//==============================================================================
// Keyboard handling for synth testing
//==============================================================================

int MainContentComponent::keyToMidiNote(int keyCode) const
{
    // QWERTY keyboard piano layout
    // Bottom row: Z-M = C3-B3 (white keys)
    // Top row: A-K = C4-B4 (white keys)
    // Numbers: 1-8 = C5-C6
    // Black keys interleaved on S, D, G, H, J and W, E, T, Y, U

    // Base octave for bottom row is C3 (MIDI 48)
    // Base octave for middle row is C4 (MIDI 60)

    switch (keyCode)
    {
        // Bottom row - C3 to B3
        case 'Z': return 48;  // C3
        case 'S': return 49;  // C#3
        case 'X': return 50;  // D3
        case 'D': return 51;  // D#3
        case 'C': return 52;  // E3
        case 'V': return 53;  // F3
        case 'G': return 54;  // F#3
        case 'B': return 55;  // G3
        case 'H': return 56;  // G#3
        case 'N': return 57;  // A3
        case 'J': return 58;  // A#3
        case 'M': return 59;  // B3

        // Middle row - C4 to B4
        case 'Q': return 60;  // C4
        case '2': return 61;  // C#4
        case 'W': return 62;  // D4
        case '3': return 63;  // D#4
        case 'E': return 64;  // E4
        case 'R': return 65;  // F4
        case '5': return 66;  // F#4
        case 'T': return 67;  // G4
        case '6': return 68;  // G#4
        case 'Y': return 69;  // A4 (440Hz)
        case '7': return 70;  // A#4
        case 'U': return 71;  // B4
        case 'I': return 72;  // C5
        case '9': return 73;  // C#5
        case 'O': return 74;  // D5
        case '0': return 75;  // D#5
        case 'P': return 76;  // E5

        default: return -1;
    }
}

bool MainContentComponent::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    int keyCode = key.getKeyCode();
    bool cmd = key.getModifiers().isCommandDown();
    bool shift = key.getModifiers().isShiftDown();

    // File operations (Cmd+N, Cmd+O, Cmd+S, Cmd+Shift+S, Cmd+E)
    if (cmd)
    {
        if (keyCode == 'N' && !shift)
        {
            newProject();
            return true;
        }
        if (keyCode == 'O' && !shift)
        {
            openProject();
            return true;
        }
        if (keyCode == 'S' && !shift)
        {
            saveProject();
            return true;
        }
        if (keyCode == 'S' && shift)
        {
            saveProjectAs();
            return true;
        }
        if (keyCode == 'E' && !shift)
        {
            exportAudio();
            return true;
        }
        if (keyCode == 'T' && !shift)
        {
            addNewTrack();
            return true;
        }
        if (keyCode == 'Z' && !shift)
        {
            auto& undoManager = UndoManager::getInstance();
            if (undoManager.canUndo())
            {
                undoManager.undo();
                repaint();
                if (timelinePanel) timelinePanel->repaint();
                if (pianoRollEditor) pianoRollEditor->repaint();
            }
            return true;
        }
        if (keyCode == 'Z' && shift)
        {
            auto& undoManager = UndoManager::getInstance();
            if (undoManager.canRedo())
            {
                undoManager.redo();
                repaint();
                if (timelinePanel) timelinePanel->repaint();
                if (pianoRollEditor) pianoRollEditor->repaint();
            }
            return true;
        }
        if (keyCode == 'A' && !shift)
        {
            // Select all clips in timeline
            if (timelinePanel)
                timelinePanel->selectAllClips();
            return true;
        }
        if (keyCode == 'P' && !shift)
        {
            togglePluginBrowser();
            return true;
        }
        if (keyCode == ',' && !shift)
        {
            PreferencesDialog::show(deviceManager, this);
            return true;
        }
    }

    // Transport shortcuts (no modifiers)
    if (!cmd && !shift)
    {
        // Space - Play/Stop
        if (keyCode == juce::KeyPress::spaceKey)
        {
            if (audioEngine.isPlaying())
                audioEngine.stop();
            else
                audioEngine.play();
            return true;
        }

        // Enter - Stop and return to start
        if (keyCode == juce::KeyPress::returnKey)
        {
            audioEngine.stop();
            audioEngine.setPositionInBeats(0.0);
            return true;
        }

        // L - Toggle loop
        if (keyCode == 'L')
        {
            audioEngine.toggleLoop();
            if (toastManager)
                toastManager->showToast(audioEngine.isLoopEnabled() ? "Loop enabled" : "Loop disabled",
                                        ToastManager::ToastType::Info, 1500);
            return true;
        }

        // Comma - Rewind (move playhead left by 1 bar)
        if (keyCode == ',')
        {
            double newPos = std::max(0.0, audioEngine.getPositionInBeats() - 4.0);
            audioEngine.setPositionInBeats(newPos);
            return true;
        }

        // Period - Forward (move playhead right by 1 bar)
        if (keyCode == '.')
        {
            audioEngine.setPositionInBeats(audioEngine.getPositionInBeats() + 4.0);
            return true;
        }

        // K - Toggle virtual keyboard
        if (keyCode == 'K')
        {
            toggleVirtualKeyboard();
            return true;
        }
    }

    // Tab key toggles mixer view
    if (keyCode == juce::KeyPress::tabKey)
    {
        toggleMixerView();
        return true;
    }

    // Escape returns to arrange view or closes piano roll or plugin browser
    if (keyCode == juce::KeyPress::escapeKey)
    {
        if (showingPluginBrowser)
        {
            hidePluginBrowser();
            return true;
        }
        else if (mainViewMode == MainViewMode::Mixer)
        {
            showArrangeView();
            return true;
        }
        else if (bottomPanelMode == BottomPanelMode::PianoRoll)
        {
            showSynthEditor();
            return true;
        }
    }

    int midiNote = keyToMidiNote(keyCode);

    if (midiNote >= 0)
    {
        // Only trigger if not already held (prevent repeats)
        if (keysDown.find(keyCode) == keysDown.end())
        {
            keysDown.insert(keyCode);
            audioEngine.synthNoteOn(midiNote, 0.8f);
        }
        return true;
    }

    return false;
}

bool MainContentComponent::keyStateChanged(bool isKeyDown, juce::Component*)
{
    // Check all tracked keys to see if any were released
    std::vector<int> keysToRemove;

    for (int keyCode : keysDown)
    {
        if (!juce::KeyPress::isKeyCurrentlyDown(keyCode))
        {
            int midiNote = keyToMidiNote(keyCode);
            if (midiNote >= 0)
            {
                audioEngine.synthNoteOff(midiNote);
            }
            keysToRemove.push_back(keyCode);
        }
    }

    for (int keyCode : keysToRemove)
    {
        keysDown.erase(keyCode);
    }

    return !keysToRemove.empty();
}

//==============================================================================
// MainWindow
//==============================================================================

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     ProgFlowColours::bgPrimary(),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);

    // Create content component and store reference
    mainComponent = new MainContentComponent();
    mainComponent->setParentWindow(this);
    setContentOwned(mainComponent, true);

    // Set up menu bar on macOS
#if JUCE_MAC
    juce::MenuBarModel::setMacMainMenu(mainComponent);
#endif

#if JUCE_IOS || JUCE_ANDROID
    setFullScreen(true);
#else
    setResizable(true, true);
    centreWithSize(getWidth(), getHeight());
#endif

    // Update window title
    updateTitle();

    setVisible(true);

    // Grab keyboard focus for the content component
    mainComponent->grabKeyboardFocus();
}

//==============================================================================
// MainContentComponent helpers
//==============================================================================

void MainContentComponent::hideWelcomeScreen()
{
    if (!showingWelcomeScreen)
        return;

    showingWelcomeScreen = false;

    // Hide welcome screen
    if (welcomeScreen)
        welcomeScreen->setVisible(false);

    // Show main UI
    transportBar->setVisible(true);
    trackHeaderPanel->setVisible(true);
    timelinePanel->setVisible(true);

    // Show synth editor (piano roll shown when clip selected)
    if (synthEditor)
        synthEditor->setVisible(bottomPanelMode == BottomPanelMode::SynthEditor);
    pianoRollEditor->setVisible(bottomPanelMode == BottomPanelMode::PianoRoll);

    // Refresh layout
    resized();
    repaint();
}

void MainContentComponent::showWelcomeScreen()
{
    if (showingWelcomeScreen)
        return;

    showingWelcomeScreen = true;

    // Show welcome screen
    if (welcomeScreen)
    {
        welcomeScreen->setRecentProjects(projectManager->getRecentProjects());
        welcomeScreen->setVisible(true);
    }

    // Hide main UI
    transportBar->setVisible(false);
    trackHeaderPanel->setVisible(false);
    timelinePanel->setVisible(false);
    if (synthEditor)
        synthEditor->setVisible(false);
    pianoRollEditor->setVisible(false);
    mixerPanel->setVisible(false);
    resizeHandle.setVisible(false);

    // Refresh layout
    resized();
    repaint();
}

void MainContentComponent::addNewTrack()
{
    // Generate track name
    int trackNum = audioEngine.getNumTracks() + 1;
    juce::String trackName = "Track " + juce::String(trackNum);

    // Create new track with a random color
    auto track = std::make_unique<Track>(trackName);

    // Generate a nice color (cycle through a palette)
    static const juce::uint32 colors[] = {
        0xff3b82f6, // Blue
        0xff10b981, // Green
        0xfff59e0b, // Amber
        0xffef4444, // Red
        0xff8b5cf6, // Purple
        0xffec4899, // Pink
        0xff06b6d4, // Cyan
        0xfff97316  // Orange
    };
    track->setColour(juce::Colour(colors[(trackNum - 1) % 8]));

    audioEngine.addTrack(std::move(track));

    // Mark project as dirty
    if (projectManager)
        projectManager->markDirty();

    // Refresh UI
    if (timelinePanel)
        timelinePanel->updateTracks();  // Rebuild track lanes
    if (trackHeaderPanel)
        trackHeaderPanel->refreshTracks();
    if (mixerPanel)
        mixerPanel->refreshTracks();

    // Show toast notification
    if (toastManager)
        toastManager->showToast("Track added", ToastManager::ToastType::Info, 2000);
}

void MainContentComponent::openPianoRoll(MidiClip* clip)
{
    if (!clip) return;

    // Set the clip in the piano roll editor
    pianoRollEditor->setClip(clip);

    // Find the track colour for this clip
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (track)
        {
            for (auto& trackClip : track->getClips())
            {
                if (trackClip.get() == clip)
                {
                    pianoRollEditor->setTrackColour(track->getColour());
                    break;
                }
            }
        }
    }

    // Switch to piano roll view
    bottomPanelMode = BottomPanelMode::PianoRoll;
    resized();
}

void MainContentComponent::showSynthEditor()
{
    bottomPanelMode = BottomPanelMode::SynthEditor;
    resized();
}

void MainContentComponent::toggleMixerView()
{
    if (mainViewMode == MainViewMode::Arrange)
        showMixerView();
    else
        showArrangeView();
}

void MainContentComponent::showArrangeView()
{
    mainViewMode = MainViewMode::Arrange;
    resized();
    repaint();
}

void MainContentComponent::showMixerView()
{
    mainViewMode = MainViewMode::Mixer;
    mixerPanel->refreshTracks();  // Ensure mixer is up to date
    resized();
    repaint();
}

void MainContentComponent::selectTrack(Track* track)
{
    selectedTrack = track;

    // Update keyboard MIDI routing to selected track
    if (track)
    {
        for (int i = 0; i < audioEngine.getNumTracks(); ++i)
        {
            if (audioEngine.getTrack(i) == track)
            {
                audioEngine.setKeyboardTrackIndex(i);
                break;
            }
        }
    }

    updateSynthEditorForTrack(track);
    showSynthEditor();
}

void MainContentComponent::updateSynthEditorForTrack(Track* track)
{
    if (!track || !track->getSynth())
    {
        // No track selected, show global synth
        if (currentSynthEditorType != SynthType::Analog)
        {
            synthEditor.reset();
            synthEditor = std::make_unique<AnalogSynthEditor>(audioEngine.getSynth());
            addAndMakeVisible(*synthEditor);
            currentSynthEditorType = SynthType::Analog;
            resized();
        }
        return;
    }

    SynthType trackSynthType = track->getSynthType();

    // Only recreate editor if synth type changed
    if (trackSynthType == currentSynthEditorType && synthEditor)
    {
        // Same type, just refresh if possible
        // For now, we recreate to ensure correct synth reference
    }

    // Remove old editor
    synthEditor.reset();

    // Create appropriate editor based on synth type
    switch (trackSynthType)
    {
        case SynthType::Analog:
            if (auto* analogSynth = dynamic_cast<AnalogSynth*>(track->getSynth()))
            {
                synthEditor = std::make_unique<AnalogSynthEditor>(*analogSynth);
            }
            break;

        case SynthType::FM:
            if (auto* fmSynth = dynamic_cast<FMSynth*>(track->getSynth()))
            {
                synthEditor = std::make_unique<FMSynthEditor>(*fmSynth);
            }
            break;

        case SynthType::Pro:
            if (auto* proSynth = dynamic_cast<ProSynth*>(track->getSynth()))
            {
                synthEditor = std::make_unique<ProSynthEditor>(*proSynth);
            }
            break;

        case SynthType::Sampler:
            if (auto* sampler = dynamic_cast<Sampler*>(track->getSynth()))
            {
                synthEditor = std::make_unique<SamplerEditor>(*sampler);
            }
            break;

        case SynthType::SoundFont:
            if (auto* sfPlayer = dynamic_cast<SoundFontPlayer*>(track->getSynth()))
            {
                synthEditor = std::make_unique<SoundFontPlayerEditor>(*sfPlayer);
            }
            break;

        case SynthType::Drums:
            if (auto* drumSynth = dynamic_cast<DrumSynth*>(track->getSynth()))
            {
                synthEditor = std::make_unique<DrumSynthEditor>(*drumSynth);
            }
            break;

        case SynthType::COUNT:
        {
            // Fallback placeholder
            auto placeholder = std::make_unique<juce::Component>();
            synthEditor = std::move(placeholder);
            break;
        }
    }

    if (synthEditor)
    {
        addAndMakeVisible(*synthEditor);
        currentSynthEditorType = trackSynthType;
        resized();
    }
}

//==============================================================================
// Plugin helpers
//==============================================================================

void MainContentComponent::togglePluginBrowser()
{
    if (showingPluginBrowser)
        hidePluginBrowser();
    else
        showPluginBrowser();
}

void MainContentComponent::showPluginBrowser()
{
    showingPluginBrowser = true;
    pluginBrowser->setVisible(true);
    resized();
}

void MainContentComponent::hidePluginBrowser()
{
    showingPluginBrowser = false;
    pluginBrowser->setVisible(false);
    resized();
}

void MainContentComponent::loadPluginOnSelectedTrack(const juce::PluginDescription& desc)
{
    if (!selectedTrack)
    {
        // No track selected, use first track
        if (audioEngine.getNumTracks() > 0)
            selectedTrack = audioEngine.getTrack(0);
        else
            return;
    }

    juce::String errorMessage;
    auto plugin = pluginHost->createPluginInstance(
        desc,
        44100.0,  // Will be updated when prepareToPlay is called
        512,
        errorMessage);

    if (plugin)
    {
        // Check if it's an instrument or effect
        if (desc.isInstrument)
        {
            selectedTrack->setPluginInstrument(std::move(plugin));

            // Show the plugin's editor
            if (auto* inst = selectedTrack->getPluginInstrument())
            {
                showPluginEditor(inst, desc.name);
            }
        }
        else
        {
            // Find first empty effect slot
            for (int i = 0; i < Track::MAX_PLUGIN_EFFECTS; ++i)
            {
                if (selectedTrack->getPluginEffect(i) == nullptr)
                {
                    selectedTrack->setPluginEffect(i, std::move(plugin));

                    // Show the plugin's editor
                    if (auto* fx = selectedTrack->getPluginEffect(i))
                    {
                        showPluginEditor(fx, desc.name);
                    }
                    break;
                }
            }
        }

        hidePluginBrowser();
    }
    else
    {
        juce::ignoreUnused(errorMessage);
    }
}

void MainContentComponent::showPluginEditor(juce::AudioPluginInstance* plugin, const juce::String& name)
{
    if (plugin && pluginEditorManager)
    {
        pluginEditorManager->showEditorForPlugin(plugin, name);
    }
}

//==============================================================================
// Virtual keyboard helpers
//==============================================================================

void MainContentComponent::toggleVirtualKeyboard()
{
    showingVirtualKeyboard = !showingVirtualKeyboard;
    resized();
    repaint();
}

//==============================================================================
// MenuBarModel implementation
//==============================================================================

juce::StringArray MainContentComponent::getMenuBarNames()
{
    return { "File", "Edit" };
}

juce::PopupMenu MainContentComponent::getMenuForIndex(int menuIndex, const juce::String& /*menuName*/)
{
    juce::PopupMenu menu;

    if (menuIndex == 0) // File menu
    {
        menu.addItem(MenuItemIDs::NewProject, "New Project\tCmd+N", true, false);
        menu.addItem(MenuItemIDs::OpenProject, "Open Project...\tCmd+O", true, false);

        // Recent projects submenu
        juce::PopupMenu recentMenu;
        auto recentProjects = projectManager->getRecentProjects();
        for (int i = 0; i < recentProjects.size() && i < 10; ++i)
        {
            juce::File file(recentProjects[i]);
            recentMenu.addItem(MenuItemIDs::OpenRecentBase + i, file.getFileName());
        }
        if (recentProjects.isEmpty())
        {
            recentMenu.addItem(0, "(No recent projects)", false);
        }
        menu.addSubMenu("Open Recent", recentMenu);

        menu.addSeparator();

        menu.addItem(MenuItemIDs::Save, "Save\tCmd+S", true, false);
        menu.addItem(MenuItemIDs::SaveAs, "Save As...\tCmd+Shift+S", true, false);

        menu.addSeparator();

        menu.addItem(MenuItemIDs::ExportAudio, "Export Audio...\tCmd+E", true, false);

#if ! JUCE_MAC
        menu.addSeparator();
        menu.addItem(MenuItemIDs::Quit, "Quit", true, false);
#endif
    }
    else if (menuIndex == 1) // Edit menu
    {
        auto& undoManager = UndoManager::getInstance();
        menu.addItem(MenuItemIDs::Undo, "Undo\tCmd+Z", undoManager.canUndo(), false);
        menu.addItem(MenuItemIDs::Redo, "Redo\tCmd+Shift+Z", undoManager.canRedo(), false);
        menu.addSeparator();

        menu.addItem(MenuItemIDs::Preferences, "Preferences...\tCmd+,", true, false);
    }

    return menu;
}

void MainContentComponent::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
    switch (menuItemID)
    {
        case MenuItemIDs::NewProject:
            newProject();
            break;

        case MenuItemIDs::OpenProject:
            openProject();
            break;

        case MenuItemIDs::Save:
            saveProject();
            break;

        case MenuItemIDs::SaveAs:
            saveProjectAs();
            break;

        case MenuItemIDs::ExportAudio:
            exportAudio();
            break;

        case MenuItemIDs::Quit:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;

        case MenuItemIDs::Undo:
            {
                auto& undoManager = UndoManager::getInstance();
                if (undoManager.canUndo())
                    undoManager.undo();
            }
            break;

        case MenuItemIDs::Redo:
            {
                auto& undoManager = UndoManager::getInstance();
                if (undoManager.canRedo())
                    undoManager.redo();
            }
            break;

        case MenuItemIDs::Preferences:
            PreferencesDialog::show(deviceManager, this);
            break;

        default:
            // Check for recent project items
            if (menuItemID >= MenuItemIDs::OpenRecentBase && menuItemID < MenuItemIDs::OpenRecentBase + 10)
            {
                int index = menuItemID - MenuItemIDs::OpenRecentBase;
                auto recentProjects = projectManager->getRecentProjects();
                if (index < recentProjects.size())
                {
                    projectManager->openProject(juce::File(recentProjects[index]));
                }
            }
            break;
    }
}

//==============================================================================
// ProjectManager::Listener implementation
//==============================================================================

void MainContentComponent::projectStateChanged()
{
    if (parentWindow)
        parentWindow->updateTitle();

    // Update transport bar with project info
    if (transportBar && projectManager)
    {
        transportBar->setProjectName(projectManager->getProjectName());
        transportBar->setProjectDirty(projectManager->isDirty());
    }
}

void MainContentComponent::projectLoaded()
{
    // Hide welcome screen and show main UI
    hideWelcomeScreen();

    // Refresh all UI components
    if (timelinePanel)
        timelinePanel->updateTracks();  // Rebuild track lanes after loading
    if (trackHeaderPanel)
        trackHeaderPanel->refreshTracks();
    if (mixerPanel)
        mixerPanel->refreshTracks();

    // Select first track and update synth editor
    if (audioEngine.getNumTracks() > 0)
    {
        selectTrack(audioEngine.getTrack(0));
    }

    // Update transport bar with project info
    if (transportBar && projectManager)
    {
        transportBar->setProjectName(projectManager->getProjectName());
        transportBar->setProjectDirty(projectManager->isDirty());
    }

    if (parentWindow)
        parentWindow->updateTitle();
}

void MainContentComponent::projectSaved()
{
    // Update transport bar with project info (dirty flag cleared)
    if (transportBar && projectManager)
    {
        transportBar->setProjectName(projectManager->getProjectName());
        transportBar->setProjectDirty(projectManager->isDirty());
    }

    if (parentWindow)
        parentWindow->updateTitle();

    // Show toast notification
    if (toastManager)
        toastManager->showToast("Project saved", ToastManager::ToastType::Success);
}

void MainContentComponent::themeChanged()
{
    // Repaint all components to reflect new theme
    repaint();

    // Force repaint of all child components recursively
    std::function<void(juce::Component*)> repaintRecursive = [&](juce::Component* comp)
    {
        if (comp)
        {
            comp->repaint();
            for (int i = 0; i < comp->getNumChildComponents(); ++i)
                repaintRecursive(comp->getChildComponent(i));
        }
    };

    for (int i = 0; i < getNumChildComponents(); ++i)
        repaintRecursive(getChildComponent(i));

    // Also update the parent window background if available
    if (parentWindow)
        parentWindow->setBackgroundColour(ProgFlowColours::bgPrimary());
}

//==============================================================================
// File operations
//==============================================================================

void MainContentComponent::newProject()
{
    hideWelcomeScreen();
    projectManager->newProject();

    // Add a default track so user has something to work with
    addNewTrack();
}

void MainContentComponent::openProject()
{
    projectManager->openProject();
    // hideWelcomeScreen will be called from projectLoaded() callback
}

void MainContentComponent::saveProject()
{
    projectManager->saveProject();
}

void MainContentComponent::saveProjectAs()
{
    projectManager->saveProjectAs();
}

void MainContentComponent::exportAudio()
{
    ExportDialog::show(audioEngine, this);
}

//==============================================================================
// MainWindow
//==============================================================================

void MainWindow::updateTitle()
{
    if (mainComponent)
    {
        auto& pm = mainComponent->getProjectManager();
        juce::String title = "ProgFlow - " + pm.getProjectName();
        if (pm.isDirty())
            title += " *";
        setName(title);
    }
}

void MainWindow::closeButtonPressed()
{
    // Check for unsaved changes before quitting
    if (mainComponent)
    {
        auto& pm = mainComponent->getProjectManager();
        if (pm.isDirty())
        {
            int result = juce::AlertWindow::showYesNoCancelBox(
                juce::AlertWindow::QuestionIcon,
                "Unsaved Changes",
                "Do you want to save changes to \"" + pm.getProjectName() + "\"?",
                "Save",
                "Don't Save",
                "Cancel",
                nullptr,
                nullptr);

            if (result == 1) // Save
            {
                // Use synchronous save to ensure save completes before quitting
                if (!pm.saveProjectSync())
                    return; // Save cancelled or failed
            }
            else if (result == 0) // Cancel
            {
                return;
            }
            // result == 2 means Don't Save - proceed with quit
        }
    }

    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
