#include "TimelinePanel.h"
#include "../LookAndFeel.h"
#include "../../Audio/AudioFileLoader.h"

//==============================================================================
// TimelineViewport implementation
//==============================================================================

void TimelineViewport::visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea)
{
    juce::Viewport::visibleAreaChanged(newVisibleArea);
    owner.onViewportScrolled();
}

TimelinePanel::TimelinePanel(AudioEngine& engine)
    : audioEngine(engine)
{
    // Create time ruler
    timeRuler = std::make_unique<TimeRuler>();
    timeRuler->setBarWidth(getBarWidth());
    timeRuler->onSeek = [this](double bar) { handleSeek(bar); };
    timeRuler->setMarkerTrack(&audioEngine.getMarkerTrack());
    timeRuler->onMarkerAdd = [this](double beatPosition) {
        audioEngine.getMarkerTrack().addMarker(beatPosition);
        timeRuler->repaint();
    };
    addAndMakeVisible(*timeRuler);

    // Create track container and viewport
    trackContainer = std::make_unique<juce::Component>();

    trackViewport = std::make_unique<TimelineViewport>(*this);
    trackViewport->setViewedComponent(trackContainer.get(), false);
    trackViewport->setScrollBarsShown(true, true);
    addAndMakeVisible(*trackViewport);

    // Create playhead overlay
    playhead = std::make_unique<PlayheadComponent>();
    addAndMakeVisible(*playhead);

    // Build initial track lanes
    updateTracks();

    // Start timer for playhead updates (60fps)
    startTimerHz(60);

    // Add keyboard listener
    addKeyListener(this);
    setWantsKeyboardFocus(true);
}

TimelinePanel::~TimelinePanel()
{
    stopTimer();
    removeKeyListener(this);
}

bool TimelinePanel::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    int keyCode = key.getKeyCode();
    bool cmd = key.getModifiers().isCommandDown();
    bool shift = key.getModifiers().isShiftDown();
    // bool alt = key.getModifiers().isAltDown();  // Reserved for future use

    // Delete/Backspace - delete selected clip
    if (keyCode == juce::KeyPress::deleteKey || keyCode == juce::KeyPress::backspaceKey)
    {
        if (selectedClip != nullptr)
        {
            deleteSelectedClip();
            return true;
        }
    }

    // Cmd+C - Copy
    if (cmd && keyCode == 'C' && !shift)
    {
        copySelectedClip();
        return true;
    }

    // Cmd+X - Cut
    if (cmd && keyCode == 'X' && !shift)
    {
        cutSelectedClip();
        return true;
    }

    // Cmd+V - Paste
    if (cmd && keyCode == 'V' && !shift)
    {
        pasteClip();
        return true;
    }

    // Cmd+D - Duplicate
    if (cmd && keyCode == 'D' && !shift)
    {
        duplicateSelectedClip();
        return true;
    }

    // Cmd+E - Split clip at playhead
    if (cmd && keyCode == 'E' && !shift)
    {
        splitSelectedClip();
        return true;
    }

    // Home - Go to start
    if (keyCode == juce::KeyPress::homeKey)
    {
        goToStart();
        return true;
    }

    // End - Go to end
    if (keyCode == juce::KeyPress::endKey)
    {
        goToEnd();
        return true;
    }

    // Arrow keys - nudge selected clip
    if (selectedClip != nullptr)
    {
        double nudgeAmount = shift ? 1.0 : 0.25;  // 1 bar or 1/4 bar

        if (keyCode == juce::KeyPress::leftKey && !cmd)
        {
            nudgeClip(-nudgeAmount);
            return true;
        }
        if (keyCode == juce::KeyPress::rightKey && !cmd)
        {
            nudgeClip(nudgeAmount);
            return true;
        }
    }

    // Zoom shortcuts
    // Cmd++ / Cmd+= - Zoom in
    if (cmd && (keyCode == '+' || keyCode == '='))
    {
        zoomIn();
        return true;
    }

    // Cmd+- - Zoom out
    if (cmd && keyCode == '-')
    {
        zoomOut();
        return true;
    }

    // Cmd+0 - Zoom to fit
    if (cmd && keyCode == '0')
    {
        zoomToFit();
        return true;
    }

    // [ and ] - Navigate between clips
    if (keyCode == '[')
    {
        selectPreviousClip();
        return true;
    }
    if (keyCode == ']')
    {
        selectNextClip();
        return true;
    }

    // M - Add marker at current playhead position
    if (keyCode == 'M' && !cmd)
    {
        double beatPosition = audioEngine.getPositionInBeats();
        audioEngine.getMarkerTrack().addMarker(beatPosition);
        timeRuler->repaint();
        return true;
    }

    // S - Toggle snap
    if (keyCode == 'S' && !cmd)
    {
        toggleSnap();
        return true;
    }

    return false;
}

void TimelinePanel::setHorizontalZoom(float zoom)
{
    horizontalZoom = juce::jlimit(0.25f, 4.0f, zoom);

    int barWidth = getBarWidth();
    timeRuler->setBarWidth(barWidth);

    for (auto& lane : trackLanes)
    {
        lane->setBarWidth(barWidth);
    }

    // Update track container width
    int contentWidth = DEFAULT_BARS * barWidth;
    trackContainer->setSize(contentWidth, trackContainer->getHeight());

    resized();
    repaint();
}

void TimelinePanel::setScrollPosition(double bars)
{
    scrollPosition = std::max(0.0, bars);
    timeRuler->setScrollOffset(scrollPosition);

    // Sync viewport
    int xPos = static_cast<int>(scrollPosition * getBarWidth());
    trackViewport->setViewPosition(xPos, trackViewport->getViewPositionY());
}

void TimelinePanel::setSnapEnabled(bool enabled)
{
    snapEnabled = enabled;

    // Update all track lanes with new snap setting
    for (auto& lane : trackLanes)
    {
        lane->setSnapEnabled(enabled);
    }

    // Visual feedback could be added here (e.g., status indicator)
    DBG("Snap " << (enabled ? "enabled" : "disabled"));
}

void TimelinePanel::selectClip(MidiClip* clip)
{
    selectedClip = clip;
    selectedClips.clear();
    if (clip)
        selectedClips.insert(clip);

    for (auto& lane : trackLanes)
    {
        lane->setSelectedClips(selectedClips);
    }

    if (onClipSelected)
        onClipSelected(clip);
}

void TimelinePanel::selectClips(const std::set<MidiClip*>& clips)
{
    selectedClips = clips;
    selectedClip = clips.empty() ? nullptr : *clips.begin();

    for (auto& lane : trackLanes)
    {
        lane->setSelectedClips(selectedClips);
    }

    if (onClipSelected && selectedClip)
        onClipSelected(selectedClip);
}

void TimelinePanel::selectAllClips()
{
    std::set<MidiClip*> allClips;

    // Gather all clips from all tracks
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (track)
        {
            for (const auto& clip : track->getClips())
            {
                if (clip)
                    allClips.insert(clip.get());
            }
        }
    }

    selectClips(allClips);
}

void TimelinePanel::selectNextClip()
{
    // Gather all clips sorted by start position
    std::vector<MidiClip*> allClips;
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (track)
        {
            for (const auto& clip : track->getClips())
            {
                if (clip)
                    allClips.push_back(clip.get());
            }
        }
    }

    if (allClips.empty())
        return;

    // Sort by start position
    std::sort(allClips.begin(), allClips.end(),
        [](const MidiClip* a, const MidiClip* b) {
            return a->getStartBar() < b->getStartBar();
        });

    // If no selection, select first clip
    if (!selectedClip)
    {
        selectClip(allClips.front());
        return;
    }

    // Find current clip and select next one
    for (size_t i = 0; i < allClips.size(); ++i)
    {
        if (allClips[i] == selectedClip && i + 1 < allClips.size())
        {
            selectClip(allClips[i + 1]);
            return;
        }
    }

    // If at end, wrap to beginning
    selectClip(allClips.front());
}

void TimelinePanel::selectPreviousClip()
{
    // Gather all clips sorted by start position
    std::vector<MidiClip*> allClips;
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (track)
        {
            for (const auto& clip : track->getClips())
            {
                if (clip)
                    allClips.push_back(clip.get());
            }
        }
    }

    if (allClips.empty())
        return;

    // Sort by start position
    std::sort(allClips.begin(), allClips.end(),
        [](const MidiClip* a, const MidiClip* b) {
            return a->getStartBar() < b->getStartBar();
        });

    // If no selection, select last clip
    if (!selectedClip)
    {
        selectClip(allClips.back());
        return;
    }

    // Find current clip and select previous one
    for (size_t i = 0; i < allClips.size(); ++i)
    {
        if (allClips[i] == selectedClip && i > 0)
        {
            selectClip(allClips[i - 1]);
            return;
        }
    }

    // If at beginning, wrap to end
    selectClip(allClips.back());
}

void TimelinePanel::addToSelection(MidiClip* clip)
{
    if (!clip) return;

    selectedClips.insert(clip);
    if (!selectedClip)
        selectedClip = clip;

    for (auto& lane : trackLanes)
    {
        lane->setSelectedClips(selectedClips);
    }
}

void TimelinePanel::clearSelection()
{
    selectedClip = nullptr;
    selectedClips.clear();

    for (auto& lane : trackLanes)
    {
        lane->setSelectedClips({});
    }
}

void TimelinePanel::updateTracks()
{
    trackLanes.clear();
    trackContainer->removeAllChildren();

    int numTracks = audioEngine.getNumTracks();
    int barWidth = getBarWidth();
    int contentWidth = DEFAULT_BARS * barWidth;

    for (int i = 0; i < numTracks; ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (!track) continue;

        auto lane = std::make_unique<TrackLane>(*track, barWidth, TRACK_HEIGHT, i);
        lane->setBounds(0, i * TRACK_HEIGHT, contentWidth, TRACK_HEIGHT);

        lane->onClipSelected = [this](MidiClip* clip) { handleClipSelected(clip); };
        lane->onClipDoubleClicked = [this](MidiClip* clip) { handleClipDoubleClicked(clip); };
        lane->onCreateClip = [this, track](double bar) { handleCreateClip(track, bar); };

        // Audio file drop callback
        lane->onAudioFileDropped = [this, track](const juce::File& file, double beatPosition) {
            handleAudioFileDropped(track, file, beatPosition);
        };

        // Set BPM for audio clip duration calculation
        lane->setBpm(audioEngine.getBpm());

        trackContainer->addAndMakeVisible(*lane);
        trackLanes.push_back(std::move(lane));
    }

    // Update container size
    int contentHeight = std::max(TRACK_HEIGHT, numTracks * TRACK_HEIGHT);
    trackContainer->setSize(contentWidth, contentHeight);
}

void TimelinePanel::createClipOnTrack(Track* track, double barPosition)
{
    if (!track) return;

    MidiClip* newClip = track->addClip(barPosition, 4.0);  // 4 bar default

    // Update UI
    updateTracks();

    // Select the new clip
    selectClip(newClip);
}

void TimelinePanel::deleteSelectedClip()
{
    if (!selectedClip) return;

    // Find the track containing this clip
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (!track) continue;

        if (track->getClip(selectedClip->getId()))
        {
            track->removeClip(selectedClip->getId());
            break;
        }
    }

    selectedClip = nullptr;
    selectedTrack = nullptr;
    updateTracks();
}

void TimelinePanel::splitSelectedClip()
{
    if (!selectedClip) return;

    // Get playhead position in beats
    double playheadBeat = audioEngine.getPositionInBeats();
    double clipStartBeat = selectedClip->getStartBeat();
    double clipEndBeat = selectedClip->getEndBeat();

    // Check if playhead is within the clip
    if (playheadBeat <= clipStartBeat || playheadBeat >= clipEndBeat)
        return;  // Playhead not within clip

    // Calculate split position relative to clip start
    double splitBeat = playheadBeat - clipStartBeat;

    // Find the track containing this clip
    Track* containingTrack = nullptr;
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (!track) continue;

        if (track->getClip(selectedClip->getId()))
        {
            containingTrack = track;
            break;
        }
    }

    if (!containingTrack) return;

    // Get beats per bar at the split position for proper time signature support
    double barPosition = audioEngine.getTimeSignatureTrack().beatsToBar(playheadBeat);
    auto timeSig = audioEngine.getTimeSignatureTrack().getTimeSignatureAtBar(barPosition);
    double beatsPerBar = static_cast<double>(timeSig.numerator);

    // Perform the split
    auto newClip = selectedClip->splitAt(splitBeat, beatsPerBar);
    if (!newClip) return;

    // Add the new clip to the same track
    MidiClip* newClipPtr = newClip.get();
    containingTrack->addClip(std::move(newClip));

    // Update the UI
    updateTracks();

    // Select the new clip (second half)
    selectClip(newClipPtr);
}

void TimelinePanel::copySelectedClip()
{
    if (!selectedClip) return;

    // Create clipboard copy
    clipboardClip = std::make_unique<ClipboardClip>();
    clipboardClip->name = selectedClip->getName();
    clipboardClip->colour = selectedClip->getColour();
    clipboardClip->durationBars = selectedClip->getDurationBars();

    // Copy all notes
    clipboardClip->notes.clear();
    for (const auto& note : selectedClip->getNotes())
    {
        clipboardClip->notes.push_back(note);
    }
}

void TimelinePanel::cutSelectedClip()
{
    copySelectedClip();
    deleteSelectedClip();
}

void TimelinePanel::pasteClip()
{
    if (!clipboardClip) return;

    // Find target track - use selectedTrack or first track
    Track* targetTrack = selectedTrack;
    if (!targetTrack && audioEngine.getNumTracks() > 0)
    {
        targetTrack = audioEngine.getTrack(0);
    }
    if (!targetTrack) return;

    // Paste at playhead position
    double pastePosition = audioEngine.getPositionInBeats() / 4.0;

    // Create new clip
    MidiClip* newClip = targetTrack->addClip(pastePosition, clipboardClip->durationBars);
    if (!newClip) return;

    // Set clip properties
    newClip->setName(clipboardClip->name + " (copy)");
    newClip->setColour(clipboardClip->colour);

    // Copy notes
    for (const auto& note : clipboardClip->notes)
    {
        newClip->addNote(note.midiNote, note.startBeat, note.durationBeats, note.velocity);
    }

    // Update UI and select new clip
    updateTracks();
    selectClip(newClip);
}

void TimelinePanel::duplicateSelectedClip()
{
    if (!selectedClip) return;

    // Find the track containing this clip
    Track* sourceTrack = nullptr;
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (!track) continue;

        if (track->getClip(selectedClip->getId()))
        {
            sourceTrack = track;
            break;
        }
    }

    if (!sourceTrack) return;

    // Create duplicate right after the original
    double newPosition = selectedClip->getEndBar();

    MidiClip* newClip = sourceTrack->addClip(newPosition, selectedClip->getDurationBars());
    if (!newClip) return;

    // Copy properties
    newClip->setName(selectedClip->getName());
    newClip->setColour(selectedClip->getColour());

    // Copy notes
    for (const auto& note : selectedClip->getNotes())
    {
        newClip->addNote(note.midiNote, note.startBeat, note.durationBeats, note.velocity);
    }

    // Update UI and select new clip
    updateTracks();
    selectClip(newClip);
}

void TimelinePanel::goToStart()
{
    audioEngine.setPositionInBeats(0.0);
    setScrollPosition(0.0);
    updatePlayheadPosition();
}

void TimelinePanel::goToEnd()
{
    // Find the end of the last clip
    double maxEnd = 0.0;
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (!track) continue;

        for (const auto& clip : track->getClips())
        {
            maxEnd = std::max(maxEnd, clip->getEndBar());
        }
    }

    audioEngine.setPositionInBars(maxEnd);
    updatePlayheadPosition();
}

void TimelinePanel::nudgeClip(double deltaBars)
{
    if (!selectedClip) return;

    double newPosition = std::max(0.0, selectedClip->getStartBar() + deltaBars);
    selectedClip->setStartBar(newPosition);

    // Update the track lane display
    updateTracks();

    // Re-select the clip after update
    selectClip(selectedClip);
}

void TimelinePanel::zoomIn()
{
    setHorizontalZoom(horizontalZoom * 1.25f);
}

void TimelinePanel::zoomOut()
{
    setHorizontalZoom(horizontalZoom / 1.25f);
}

void TimelinePanel::zoomToFit()
{
    // Find the extent of all clips
    double maxEnd = 4.0;  // Minimum 4 bars
    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        Track* track = audioEngine.getTrack(i);
        if (!track) continue;

        for (const auto& clip : track->getClips())
        {
            maxEnd = std::max(maxEnd, clip->getEndBar());
        }
    }

    // Calculate zoom to fit
    int viewWidth = trackViewport->getWidth();
    if (viewWidth > 0)
    {
        float targetZoom = static_cast<float>(viewWidth) / (static_cast<float>(maxEnd) * BASE_BAR_WIDTH);
        setHorizontalZoom(juce::jlimit(0.25f, 4.0f, targetZoom * 0.9f));  // Leave some margin
    }

    setScrollPosition(0.0);
}

void TimelinePanel::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());

    // Draw marquee selection overlay
    if (isDraggingMarquee)
    {
        drawMarqueeSelection(g);
    }
}

void TimelinePanel::resized()
{
    auto bounds = getLocalBounds();

    // Time ruler at top
    timeRuler->setBounds(bounds.removeFromTop(RULER_HEIGHT));

    // Track viewport takes remaining space
    trackViewport->setBounds(bounds);

    // Playhead overlays both ruler and tracks
    playhead->setBounds(getLocalBounds());

    updatePlayheadPosition();
}

void TimelinePanel::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCommandDown())
    {
        // Zoom with cmd+wheel
        float zoomDelta = wheel.deltaY > 0 ? 0.1f : -0.1f;
        setHorizontalZoom(horizontalZoom + zoomDelta);
    }
    else
    {
        // Pass to viewport for scrolling
        trackViewport->mouseWheelMove(e, wheel);
    }
}

void TimelinePanel::mouseDown(const juce::MouseEvent& e)
{
    // Only start marquee if clicking in the track area (below ruler)
    if (e.y < RULER_HEIGHT)
        return;

    // Start marquee selection
    isDraggingMarquee = true;
    marqueeStart = e.getPosition();
    marqueeRect = juce::Rectangle<int>(marqueeStart, marqueeStart);

    // Clear selection unless Shift is held (for adding to selection)
    if (!e.mods.isShiftDown())
    {
        clearSelection();
    }

    repaint();
}

void TimelinePanel::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDraggingMarquee)
        return;

    // Update marquee rectangle
    auto currentPoint = e.getPosition();
    int x1 = std::min(marqueeStart.x, currentPoint.x);
    int y1 = std::min(marqueeStart.y, currentPoint.y);
    int x2 = std::max(marqueeStart.x, currentPoint.x);
    int y2 = std::max(marqueeStart.y, currentPoint.y);

    marqueeRect = juce::Rectangle<int>(x1, y1, x2 - x1, y2 - y1);

    // Find clips within the marquee and update selection
    auto clipsInRect = getClipsInRect(marqueeRect);
    selectClips(clipsInRect);

    repaint();
}

void TimelinePanel::mouseUp(const juce::MouseEvent& e)
{
    if (isDraggingMarquee)
    {
        // Final selection update
        auto clipsInRect = getClipsInRect(marqueeRect);

        if (e.mods.isShiftDown())
        {
            // Add to existing selection
            for (auto* clip : clipsInRect)
            {
                addToSelection(clip);
            }
        }
        else
        {
            selectClips(clipsInRect);
        }

        isDraggingMarquee = false;
        marqueeRect = juce::Rectangle<int>();
        repaint();
    }
}

std::set<MidiClip*> TimelinePanel::getClipsInRect(const juce::Rectangle<int>& rect) const
{
    std::set<MidiClip*> result;

    // Convert screen coordinates to track container coordinates
    auto viewPos = trackViewport->getViewPosition();
    auto trackRect = rect.translated(viewPos.x, viewPos.y - RULER_HEIGHT);

    // Query each track lane for clips in the rectangle
    for (size_t i = 0; i < trackLanes.size(); ++i)
    {
        auto& lane = trackLanes[i];
        int laneY = static_cast<int>(i) * TRACK_HEIGHT;

        // Create a rectangle relative to this track lane
        auto laneRelativeRect = trackRect.translated(0, -laneY);

        // Only check if the rect overlaps this lane vertically
        if (trackRect.getY() < laneY + TRACK_HEIGHT && trackRect.getBottom() > laneY)
        {
            auto clipsInLane = lane->getClipsInRect(laneRelativeRect);
            result.insert(clipsInLane.begin(), clipsInLane.end());
        }
    }

    return result;
}

void TimelinePanel::drawMarqueeSelection(juce::Graphics& g)
{
    // Draw semi-transparent fill
    g.setColour(juce::Colour(0x203b82f6));  // Light blue fill
    g.fillRect(marqueeRect);

    // Draw border
    g.setColour(juce::Colour(0xff3b82f6));  // Blue border
    g.drawRect(marqueeRect, 1);
}

void TimelinePanel::timerCallback()
{
    updatePlayheadPosition();
}

int TimelinePanel::getBarWidth() const
{
    return static_cast<int>(BASE_BAR_WIDTH * horizontalZoom);
}

void TimelinePanel::updatePlayheadPosition()
{
    double positionInBars = audioEngine.getPositionInBeats() / 4.0;
    int xPos = static_cast<int>((positionInBars - scrollPosition) * getBarWidth());

    playhead->setPosition(xPos);
    playhead->repaint();
}

void TimelinePanel::handleSeek(double bar)
{
    audioEngine.setPositionInBars(bar);
    updatePlayheadPosition();
}

void TimelinePanel::handleClipSelected(MidiClip* clip)
{
    // Find the track containing this clip
    selectedTrack = nullptr;
    if (clip)
    {
        for (int i = 0; i < audioEngine.getNumTracks(); ++i)
        {
            Track* track = audioEngine.getTrack(i);
            if (track && track->getClip(clip->getId()))
            {
                selectedTrack = track;
                break;
            }
        }
    }

    selectClip(clip);
}

void TimelinePanel::handleClipDoubleClicked(MidiClip* clip)
{
    if (onClipDoubleClicked)
        onClipDoubleClicked(clip);
}

void TimelinePanel::handleCreateClip(Track* track, double barPosition)
{
    createClipOnTrack(track, barPosition);
}

void TimelinePanel::handleAudioFileDropped(Track* track, const juce::File& file, double beatPosition)
{
    if (!track) return;

    // Load the audio file
    auto& loader = getAudioFileLoader();
    auto clip = loader.loadFile(file);

    if (!clip)
    {
        DBG("Failed to load audio file: " + file.getFullPathName());
        return;
    }

    // Set the clip position
    clip->setStartBeat(beatPosition);

    // Add to track
    track->addAudioClip(std::move(clip));

    // Refresh the UI
    updateTracks();

    DBG("Loaded audio file: " + file.getFileName() + " at beat " + juce::String(beatPosition));
}

void TimelinePanel::onViewportScrolled()
{
    // Sync time ruler with viewport horizontal scroll
    if (trackViewport && timeRuler)
    {
        int xPos = trackViewport->getViewPositionX();
        double bars = static_cast<double>(xPos) / getBarWidth();
        scrollPosition = bars;
        timeRuler->setScrollOffset(scrollPosition);

        // Update playhead position relative to new scroll
        updatePlayheadPosition();
    }
}
