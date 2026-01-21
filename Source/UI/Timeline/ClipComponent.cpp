#include "ClipComponent.h"
#include "../LookAndFeel.h"

ClipComponent::ClipComponent(MidiClip& c, int bw, int th)
    : clip(c), barWidth(bw), trackHeight(th)
{
    updateFromClip();
}

void ClipComponent::setBarWidth(int width)
{
    barWidth = std::max(20, width);
    updateFromClip();
}

void ClipComponent::setTrackHeight(int height)
{
    trackHeight = std::max(20, height);
    updateFromClip();
}

void ClipComponent::setSelected(bool sel)
{
    if (selected != sel)
    {
        selected = sel;
        repaint();
    }
}

void ClipComponent::updateFromClip()
{
    // Calculate position and size from clip data
    int x = static_cast<int>(clip.getStartBar() * barWidth);
    int width = static_cast<int>(clip.getDurationBars() * barWidth);

    setBounds(x, 0, width, trackHeight);
    repaint();
}

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Add vertical padding like the Electron version (top-2 bottom-2 = 8px each)
    constexpr int verticalPadding = 4;
    bounds = bounds.reduced(2, verticalPadding);

    juce::Colour clipColour = clip.getColour();

    // Selection glow effect (drawn first, behind the clip) - Saturn purple accent
    if (selected)
    {
        // Draw outer glow with Saturn accent
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.3f));
        g.fillRoundedRectangle(bounds.toFloat().expanded(3.0f), 6.0f);
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.15f));
        g.fillRoundedRectangle(bounds.toFloat().expanded(6.0f), 8.0f);
    }

    // Clip background with subtle gradient
    juce::ColourGradient gradient(clipColour.brighter(0.1f), 0, static_cast<float>(bounds.getY()),
                                   clipColour.darker(0.1f), 0, static_cast<float>(bounds.getBottom()),
                                   false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds.toFloat(), 5.0f);

    // Border - white when selected, semi-transparent when not
    if (selected)
    {
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawRoundedRectangle(bounds.toFloat(), 5.0f, 2.0f);
    }
    else
    {
        g.setColour(juce::Colours::white.withAlpha(isMouseOver() ? 0.5f : 0.2f));
        g.drawRoundedRectangle(bounds.toFloat(), 5.0f, 1.0f);
    }

    // Clip name at top with better styling
    auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
    g.setColour(ProgFlowColours::textPrimary());
    g.setFont(juce::FontOptions(12.0f).withStyle("Bold"));
    g.drawText(clip.getName(), headerBounds.reduced(8, 2), juce::Justification::centredLeft, true);

    // Note preview in remaining area
    if (bounds.getHeight() > 8)
    {
        drawNotePreviews(g, bounds.reduced(4, 2));
    }

    // Resize handles (subtle visual cue on hover)
    if (isMouseOver())
    {
        auto handleBounds = getLocalBounds().reduced(2, verticalPadding);
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        // Left handle
        g.fillRoundedRectangle(static_cast<float>(handleBounds.getX()),
                                static_cast<float>(handleBounds.getY()),
                                static_cast<float>(RESIZE_HANDLE_WIDTH),
                                static_cast<float>(handleBounds.getHeight()), 3.0f);
        // Right handle
        g.fillRoundedRectangle(static_cast<float>(handleBounds.getRight() - RESIZE_HANDLE_WIDTH),
                                static_cast<float>(handleBounds.getY()),
                                static_cast<float>(RESIZE_HANDLE_WIDTH),
                                static_cast<float>(handleBounds.getHeight()), 3.0f);
    }
}

void ClipComponent::drawNotePreviews(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    const auto& notes = clip.getNotes();
    if (notes.empty()) return;

    // Find note range for scaling
    int minNote = 127, maxNote = 0;
    for (const auto& note : notes)
    {
        minNote = std::min(minNote, note.midiNote);
        maxNote = std::max(maxNote, note.midiNote);
    }

    // Ensure minimum range for better visualization
    if (maxNote - minNote < 12)
    {
        int mid = (maxNote + minNote) / 2;
        minNote = mid - 6;
        maxNote = mid + 6;
    }

    int noteRange = maxNote - minNote + 1;
    double noteHeight = static_cast<double>(bounds.getHeight()) / noteRange;
    double beatWidth = static_cast<double>(bounds.getWidth()) / clip.getDurationBeats();

    // Use white with good visibility
    g.setColour(juce::Colours::white.withAlpha(0.8f));

    for (const auto& note : notes)
    {
        float x = bounds.getX() + static_cast<float>(note.startBeat * beatWidth);
        float y = bounds.getY() + static_cast<float>((maxNote - note.midiNote) * noteHeight);
        float w = std::max(3.0f, static_cast<float>(note.durationBeats * beatWidth));
        float h = std::max(2.0f, static_cast<float>(noteHeight) - 1.0f);

        // Draw note with rounded corners for a cleaner look
        g.fillRoundedRectangle(x, y, w, h, 1.0f);
    }
}

ClipComponent::DragMode ClipComponent::getDragModeForPosition(int x) const
{
    // Account for horizontal padding (2px on each side)
    constexpr int horizontalPadding = 2;
    int effectiveX = x - horizontalPadding;
    int effectiveWidth = getWidth() - (horizontalPadding * 2);

    if (effectiveX < RESIZE_HANDLE_WIDTH)
        return DragMode::ResizeLeft;
    if (effectiveX > effectiveWidth - RESIZE_HANDLE_WIDTH)
        return DragMode::ResizeRight;
    return DragMode::Move;
}

void ClipComponent::updateCursor(int x)
{
    auto mode = getDragModeForPosition(x);

    if (mode == DragMode::ResizeLeft || mode == DragMode::ResizeRight)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else
        setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void ClipComponent::mouseMove(const juce::MouseEvent& e)
{
    updateCursor(e.x);
}

void ClipComponent::mouseDown(const juce::MouseEvent& e)
{
    // Select on click
    if (onSelected)
        onSelected(this);

    // Start drag - store initial clip state
    dragMode = getDragModeForPosition(e.x);
    dragStartBar = clip.getStartBar();
    dragStartDuration = clip.getDurationBars();

    // Bring to front during drag
    toFront(true);

    updateCursor(e.x);
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (dragMode == DragMode::None) return;

    // Use getOffsetFromDragStart for accurate drag tracking
    // (not affected by component bounds changes during drag)
    auto dragOffset = e.getOffsetFromDragStart();
    double deltaBars = static_cast<double>(dragOffset.getX()) / static_cast<double>(barWidth);

    if (dragMode == DragMode::Move)
    {
        double newStartBar = std::max(0.0, dragStartBar + deltaBars);

        // Snap to quarter-bar boundaries
        newStartBar = std::round(newStartBar * 4.0) / 4.0;

        clip.setStartBar(newStartBar);
        updateFromClip();

        if (onMoved)
            onMoved(newStartBar, clip.getDurationBars());
    }
    else if (dragMode == DragMode::ResizeRight)
    {
        double newDuration = std::max(0.25, dragStartDuration + deltaBars);

        // Snap to quarter-bar
        newDuration = std::round(newDuration * 4.0) / 4.0;

        clip.setDurationBars(newDuration);
        updateFromClip();

        if (onResized)
            onResized(newDuration);
    }
    else if (dragMode == DragMode::ResizeLeft)
    {
        double newStartBar = dragStartBar + deltaBars;
        double endBar = dragStartBar + dragStartDuration;

        // Don't let start go past end
        newStartBar = std::min(newStartBar, endBar - 0.25);
        newStartBar = std::max(0.0, newStartBar);

        // Snap to quarter-bar
        newStartBar = std::round(newStartBar * 4.0) / 4.0;

        double newDuration = endBar - newStartBar;

        clip.setStartBar(newStartBar);
        clip.setDurationBars(newDuration);
        updateFromClip();

        if (onMoved)
            onMoved(newStartBar, newDuration);
    }
}

void ClipComponent::mouseUp(const juce::MouseEvent&)
{
    dragMode = DragMode::None;
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ClipComponent::mouseDoubleClick(const juce::MouseEvent&)
{
    if (onDoubleClicked)
        onDoubleClicked(this);
}
