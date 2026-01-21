#include "TrackLane.h"
#include "../LookAndFeel.h"
#include "../../Audio/AudioFileLoader.h"

TrackLane::TrackLane(Track& t, int bw, int th, int idx)
    : track(t), barWidth(bw), trackHeight(th), trackIndex(idx)
{
    updateClips();
}

void TrackLane::setBarWidth(int width)
{
    barWidth = std::max(20, width);

    for (auto& comp : clipComponents)
    {
        comp->setBarWidth(barWidth);
    }

    // Update audio clips with pixels per beat (4 beats per bar)
    int pixelsPerBeat = barWidth / 4;
    for (auto& comp : audioClipComponents)
    {
        comp->setPixelsPerBeat(pixelsPerBeat);
    }

    for (auto& comp : automationLaneComponents)
    {
        comp->setBarWidth(barWidth);
    }
}

void TrackLane::setBpm(double newBpm)
{
    if (std::abs(bpm - newBpm) < 0.01)
        return;

    bpm = newBpm;

    for (auto& comp : audioClipComponents)
    {
        comp->setBpm(bpm);
    }
}

void TrackLane::setTrackHeight(int height)
{
    trackHeight = std::max(20, height);

    for (auto& comp : clipComponents)
    {
        comp->setTrackHeight(trackHeight);
    }

    for (auto& comp : audioClipComponents)
    {
        comp->setTrackHeight(trackHeight);
    }
}

void TrackLane::updateClips()
{
    // Update MIDI clips
    clipComponents.clear();

    const auto& clips = track.getClips();

    for (const auto& clip : clips)
    {
        auto comp = std::make_unique<ClipComponent>(*clip, barWidth, trackHeight);

        comp->onSelected = [this](ClipComponent* c) { handleClipSelected(c); };
        comp->onDoubleClicked = [this](ClipComponent* c) { handleClipDoubleClicked(c); };

        addAndMakeVisible(*comp);
        clipComponents.push_back(std::move(comp));
    }

    // Update Audio clips
    audioClipComponents.clear();

    const auto& audioClips = track.getAudioClips();
    int pixelsPerBeat = barWidth / 4;

    for (const auto& clip : audioClips)
    {
        auto comp = std::make_unique<AudioClipComponent>(*clip, bpm, pixelsPerBeat, trackHeight);

        comp->onSelected = [this](AudioClipComponent* c) { handleAudioClipSelected(c); };

        addAndMakeVisible(*comp);
        audioClipComponents.push_back(std::move(comp));
    }

    resized();
}

void TrackLane::setSelectedClip(MidiClip* clip)
{
    selectedClip = clip;

    for (auto& comp : clipComponents)
    {
        comp->setSelected(&comp->getClip() == clip);
    }
}

void TrackLane::setSelectedClips(const std::set<MidiClip*>& clips)
{
    selectedClip = clips.empty() ? nullptr : *clips.begin();

    for (auto& comp : clipComponents)
    {
        comp->setSelected(clips.count(&comp->getClip()) > 0);
    }
}

std::set<MidiClip*> TrackLane::getClipsInRect(const juce::Rectangle<int>& rect) const
{
    std::set<MidiClip*> result;

    for (auto& comp : clipComponents)
    {
        // Get clip bounds in track coordinates
        auto clipBounds = comp->getBounds();

        // Check if selection rectangle intersects clip bounds
        if (rect.intersects(clipBounds))
        {
            result.insert(&comp->getClip());
        }
    }

    return result;
}

void TrackLane::handleClipSelected(ClipComponent* comp)
{
    selectedClip = &comp->getClip();

    // Deselect audio clips
    for (auto& c : audioClipComponents)
    {
        c->setSelected(false);
    }
    selectedAudioClip = nullptr;

    // Update selection state of MIDI clips
    for (auto& c : clipComponents)
    {
        c->setSelected(c.get() == comp);
    }

    if (onClipSelected)
        onClipSelected(selectedClip);
}

void TrackLane::handleClipDoubleClicked(ClipComponent* comp)
{
    if (onClipDoubleClicked)
        onClipDoubleClicked(&comp->getClip());
}

void TrackLane::handleAudioClipSelected(AudioClipComponent* comp)
{
    selectedAudioClip = &comp->getClip();

    // Deselect MIDI clips
    for (auto& c : clipComponents)
    {
        c->setSelected(false);
    }
    selectedClip = nullptr;

    // Update selection state of audio clips
    for (auto& c : audioClipComponents)
    {
        c->setSelected(c.get() == comp);
    }

    if (onAudioClipSelected)
        onAudioClipSelected(selectedAudioClip);
}

void TrackLane::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background - slightly different shade based on track index
    bool isEvenTrack = (trackIndex % 2 == 0);
    g.setColour(isEvenTrack ? ProgFlowColours::bgPrimary() : ProgFlowColours::bgPrimary().darker(0.03f));
    g.fillRect(bounds);

    int numBars = (bounds.getWidth() / barWidth) + 1;

    // Draw beat lines (lighter, every beat within bar)
    g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.3f));
    int beatWidth = barWidth / 4; // Assuming 4/4
    for (int beat = 0; beat <= numBars * 4; ++beat)
    {
        if (beat % 4 != 0) // Skip bar lines
        {
            int x = beat * beatWidth;
            g.drawVerticalLine(x, 0.0f, static_cast<float>(bounds.getHeight()));
        }
    }

    // Draw bar lines (more prominent)
    g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.7f));
    for (int bar = 0; bar <= numBars; ++bar)
    {
        int x = bar * barWidth;
        g.drawVerticalLine(x, 0.0f, static_cast<float>(bounds.getHeight()));
    }

    // Bottom border
    g.setColour(ProgFlowColours::border());
    g.drawHorizontalLine(bounds.getHeight() - 1, 0.0f, static_cast<float>(bounds.getWidth()));

    // File drag hover effect (Saturn accent)
    if (fileDragHover)
    {
        g.setColour(ProgFlowColours::accentGreen().withAlpha(0.2f));
        g.fillRect(getLocalBounds().withHeight(trackHeight));
        g.setColour(ProgFlowColours::accentGreen());
        g.drawRect(getLocalBounds().withHeight(trackHeight), 2);
    }
}

void TrackLane::resized()
{
    // Update MIDI clip component positions and heights
    for (auto& comp : clipComponents)
    {
        comp->setTrackHeight(trackHeight);  // Clips only in main track area
        comp->updateFromClip();
    }

    // Update audio clip component positions and heights
    for (auto& comp : audioClipComponents)
    {
        comp->setTrackHeight(trackHeight);
        comp->updateFromClip();
    }

    // Position automation lanes below the clip area
    if (automationExpanded)
    {
        int y = trackHeight;
        for (auto& comp : automationLaneComponents)
        {
            comp->setBounds(0, y, getWidth(), AutomationLaneComponent::LANE_HEIGHT);
            comp->setBarWidth(barWidth);
            comp->setVisible(true);
            y += AutomationLaneComponent::LANE_HEIGHT;
        }
    }
}

void TrackLane::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Only handle double-click in the clip area (not automation lanes)
    if (e.y > trackHeight)
        return;

    // Create new clip at this position
    double barPosition = static_cast<double>(e.x) / barWidth;

    // Snap to bar
    barPosition = std::floor(barPosition);

    if (onCreateClip)
        onCreateClip(barPosition);
}

//==============================================================================
// Automation

void TrackLane::setAutomationExpanded(bool expanded)
{
    if (automationExpanded == expanded)
        return;

    automationExpanded = expanded;

    if (expanded)
    {
        refreshAutomationLanes();
    }
    else
    {
        // Hide automation lanes
        for (auto& comp : automationLaneComponents)
        {
            comp->setVisible(false);
        }
    }

    resized();
}

void TrackLane::refreshAutomationLanes()
{
    automationLaneComponents.clear();

    const auto& lanes = track.getAutomationLanes();

    for (const auto& lane : lanes)
    {
        auto comp = std::make_unique<AutomationLaneComponent>(track, *lane, barWidth);
        addAndMakeVisible(*comp);
        automationLaneComponents.push_back(std::move(comp));
    }

    resized();
}

void TrackLane::addAutomationLane(const juce::String& parameterId)
{
    // Create lane in track if it doesn't exist
    auto* lane = track.getOrCreateAutomationLane(parameterId);

    // Create UI component
    auto comp = std::make_unique<AutomationLaneComponent>(track, *lane, barWidth);
    addAndMakeVisible(*comp);
    automationLaneComponents.push_back(std::move(comp));

    // Auto-expand when adding a lane
    automationExpanded = true;

    resized();
}

int TrackLane::getTotalHeight() const
{
    int total = trackHeight;

    if (automationExpanded)
    {
        total += static_cast<int>(automationLaneComponents.size()) * AutomationLaneComponent::LANE_HEIGHT;
    }

    return total;
}

//==============================================================================
// FileDragAndDropTarget

bool TrackLane::isInterestedInFileDrag(const juce::StringArray& files)
{
    // Check if any file is a supported audio format
    auto& loader = getAudioFileLoader();

    for (const auto& path : files)
    {
        juce::File file(path);
        if (loader.isFormatSupported(file.getFileExtension()))
            return true;
    }

    return false;
}

void TrackLane::filesDropped(const juce::StringArray& files, int x, int /*y*/)
{
    fileDragHover = false;
    repaint();

    auto& loader = getAudioFileLoader();

    for (const auto& path : files)
    {
        juce::File file(path);

        if (!loader.isFormatSupported(file.getFileExtension()))
            continue;

        // Calculate beat position from x coordinate
        double beatPosition = (static_cast<double>(x) / barWidth) * 4.0;  // 4 beats per bar
        beatPosition = std::max(0.0, std::floor(beatPosition));  // Snap to beat

        if (onAudioFileDropped)
            onAudioFileDropped(file, beatPosition);
    }
}

void TrackLane::fileDragEnter(const juce::StringArray& /*files*/, int /*x*/, int /*y*/)
{
    fileDragHover = true;
    repaint();
}

void TrackLane::fileDragExit(const juce::StringArray& /*files*/)
{
    fileDragHover = false;
    repaint();
}
