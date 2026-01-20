#include "AudioClipComponent.h"

AudioClipComponent::AudioClipComponent(AudioClip& audioClip, double initialBpm,
                                       int initialPixelsPerBeat, int height)
    : clip(audioClip), bpm(initialBpm), pixelsPerBeat(initialPixelsPerBeat),
      trackHeight(height)
{
    addAndMakeVisible(waveformDisplay);
    waveformDisplay.setAudioClip(&clip);
    waveformDisplay.setShowName(false);  // We draw name ourselves
    waveformDisplay.setInterceptsMouseClicks(false, false);

    updateFromClip();
}

void AudioClipComponent::setBpm(double newBpm)
{
    if (std::abs(bpm - newBpm) < 0.01)
        return;
    bpm = newBpm;
    updateFromClip();
}

void AudioClipComponent::setPixelsPerBeat(int ppb)
{
    if (pixelsPerBeat == ppb)
        return;
    pixelsPerBeat = ppb;
    updateFromClip();
}

void AudioClipComponent::setTrackHeight(int height)
{
    if (trackHeight == height)
        return;
    trackHeight = height;
    updateFromClip();
}

void AudioClipComponent::setSelected(bool newSelected)
{
    if (selected == newSelected)
        return;
    selected = newSelected;
    repaint();
}

void AudioClipComponent::updateFromClip()
{
    // Calculate position and size based on clip data and zoom
    double startBeat = clip.getStartBeat();
    double durationBeats = clip.getDurationInBeats(bpm);

    int x = static_cast<int>(startBeat * pixelsPerBeat);
    int width = std::max(20, static_cast<int>(durationBeats * pixelsPerBeat));

    setBounds(x, 0, width, trackHeight);

    // Update waveform display
    double pixelsPerSecond = (bpm / 60.0) * pixelsPerBeat;
    waveformDisplay.setPixelsPerSecond(pixelsPerSecond);

    repaint();
}

void AudioClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Background with gradient
    juce::Colour baseColour(0xff4ade80);  // Green for audio clips
    if (selected)
        baseColour = baseColour.brighter(0.3f);

    juce::ColourGradient gradient(
        baseColour.brighter(0.1f), 0, 0,
        baseColour.darker(0.2f), 0, static_cast<float>(bounds.getHeight()),
        false
    );
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.toFloat(), 4.0f);

    // Border
    g.setColour(selected ? juce::Colours::white : baseColour.darker(0.4f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 4.0f, selected ? 2.0f : 1.0f);

    // Selection glow
    if (selected)
    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.5f), 4.0f, 3.0f);
    }

    // Header bar
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
    g.setColour(baseColour.darker(0.3f).withAlpha(0.5f));
    g.fillRoundedRectangle(headerBounds.toFloat(), 4.0f);

    // Clip name
    g.setColour(juce::Colours::white);
    g.setFont(11.0f);
    g.drawText(clip.getName(), headerBounds.reduced(4, 0),
               juce::Justification::centredLeft, true);

    // Draw fade overlays
    drawFadeOverlays(g);

    // Trim handles visual indicator
    if (isMouseOver())
    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        // Left handle
        g.fillRect(0, HEADER_HEIGHT, TRIM_HANDLE_WIDTH, getHeight() - HEADER_HEIGHT);
        // Right handle
        g.fillRect(getWidth() - TRIM_HANDLE_WIDTH, HEADER_HEIGHT,
                   TRIM_HANDLE_WIDTH, getHeight() - HEADER_HEIGHT);
    }
}

void AudioClipComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(HEADER_HEIGHT);
    bounds.reduce(2, 2);
    waveformDisplay.setBounds(bounds);
}

void AudioClipComponent::drawFadeOverlays(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(HEADER_HEIGHT);

    double sampleRate = clip.getSampleRate();
    if (sampleRate <= 0) return;

    // Fade in
    juce::int64 fadeInSamples = clip.getFadeInSamples();
    if (fadeInSamples > 0)
    {
        double fadeInSeconds = fadeInSamples / sampleRate;
        double fadeInBeats = fadeInSeconds * (bpm / 60.0);
        int fadeInWidth = static_cast<int>(fadeInBeats * pixelsPerBeat);

        juce::Path fadeInPath;
        fadeInPath.startNewSubPath(0, static_cast<float>(bounds.getY()));
        fadeInPath.lineTo(static_cast<float>(fadeInWidth), static_cast<float>(bounds.getY()));
        fadeInPath.lineTo(0, static_cast<float>(bounds.getBottom()));
        fadeInPath.closeSubPath();

        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillPath(fadeInPath);
    }

    // Fade out
    juce::int64 fadeOutSamples = clip.getFadeOutSamples();
    if (fadeOutSamples > 0)
    {
        double fadeOutSeconds = fadeOutSamples / sampleRate;
        double fadeOutBeats = fadeOutSeconds * (bpm / 60.0);
        int fadeOutWidth = static_cast<int>(fadeOutBeats * pixelsPerBeat);
        int fadeOutStart = getWidth() - fadeOutWidth;

        juce::Path fadeOutPath;
        fadeOutPath.startNewSubPath(static_cast<float>(fadeOutStart), static_cast<float>(bounds.getBottom()));
        fadeOutPath.lineTo(static_cast<float>(getWidth()), static_cast<float>(bounds.getY()));
        fadeOutPath.lineTo(static_cast<float>(getWidth()), static_cast<float>(bounds.getBottom()));
        fadeOutPath.closeSubPath();

        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.fillPath(fadeOutPath);
    }
}

void AudioClipComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        // Context menu could go here
        return;
    }

    dragMode = getDragModeForPosition(e.x, e.y);
    dragStartBeat = clip.getStartBeat();
    dragStartTrimStart = clip.getTrimStartSample();
    dragStartTrimEnd = clip.getTrimEndSample();

    if (onSelected)
        onSelected(this);
}

void AudioClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (dragMode == DragMode::None)
        return;

    double beatsPerPixel = 1.0 / pixelsPerBeat;
    auto offset = e.getOffsetFromDragStart();

    if (dragMode == DragMode::Move)
    {
        double beatOffset = offset.x * beatsPerPixel;
        double newStartBeat = std::max(0.0, dragStartBeat + beatOffset);

        clip.setStartBeat(newStartBeat);
        updateFromClip();

        if (onMoved)
            onMoved(newStartBeat);
    }
    else if (dragMode == DragMode::TrimLeft)
    {
        // Trimming from left: adjust trim start
        double sampleRate = clip.getSampleRate();
        double samplesPerBeat = (sampleRate * 60.0) / bpm;
        double beatOffset = offset.x * beatsPerPixel;
        juce::int64 sampleOffset = static_cast<juce::int64>(beatOffset * samplesPerBeat);

        juce::int64 newTrimStart = juce::jlimit(
            (juce::int64)0,
            dragStartTrimEnd - 100,
            dragStartTrimStart + sampleOffset
        );

        clip.setTrimStartSample(newTrimStart);
        updateFromClip();

        if (onTrimmed)
            onTrimmed(static_cast<double>(clip.getTrimStartSample()),
                      static_cast<double>(clip.getTrimEndSample()));
    }
    else if (dragMode == DragMode::TrimRight)
    {
        // Trimming from right: adjust trim end
        double sampleRate = clip.getSampleRate();
        double samplesPerBeat = (sampleRate * 60.0) / bpm;
        double beatOffset = offset.x * beatsPerPixel;
        juce::int64 sampleOffset = static_cast<juce::int64>(beatOffset * samplesPerBeat);

        juce::int64 maxSamples = clip.getAudioBuffer().getNumSamples();
        juce::int64 newTrimEnd = juce::jlimit(
            dragStartTrimStart + 100,
            maxSamples,
            dragStartTrimEnd + sampleOffset
        );

        clip.setTrimEndSample(newTrimEnd);
        updateFromClip();

        if (onTrimmed)
            onTrimmed(static_cast<double>(clip.getTrimStartSample()),
                      static_cast<double>(clip.getTrimEndSample()));
    }
}

void AudioClipComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    dragMode = DragMode::None;
}

void AudioClipComponent::mouseDoubleClick(const juce::MouseEvent& /*e*/)
{
    if (onDoubleClicked)
        onDoubleClicked(this);
}

void AudioClipComponent::mouseMove(const juce::MouseEvent& e)
{
    updateCursor(e.x, e.y);
}

AudioClipComponent::DragMode AudioClipComponent::getDragModeForPosition(int x, int y) const
{
    // Check trim handles (only in waveform area)
    if (y > HEADER_HEIGHT)
    {
        if (x < TRIM_HANDLE_WIDTH)
            return DragMode::TrimLeft;
        if (x > getWidth() - TRIM_HANDLE_WIDTH)
            return DragMode::TrimRight;
    }

    return DragMode::Move;
}

void AudioClipComponent::updateCursor(int x, int y)
{
    auto mode = getDragModeForPosition(x, y);

    switch (mode)
    {
        case DragMode::TrimLeft:
        case DragMode::TrimRight:
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            break;
        default:
            setMouseCursor(juce::MouseCursor::NormalCursor);
            break;
    }
}
