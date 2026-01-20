#include "ResizablePanel.h"

ResizablePanel::ResizablePanel(Edge resizeEdge)
    : edge(resizeEdge)
{
    setMouseCursor(juce::MouseCursor::NormalCursor);
}

void ResizablePanel::setMinSize(int size)
{
    minSize = size;
    currentSize = juce::jmax(currentSize, minSize);
}

void ResizablePanel::setMaxSize(int size)
{
    maxSize = size;
    currentSize = juce::jmin(currentSize, maxSize);
}

void ResizablePanel::setDefaultSize(int size)
{
    defaultSize = juce::jlimit(minSize, maxSize, size);
}

void ResizablePanel::setPreferenceKey(const juce::String& key)
{
    preferenceKey = key;
    loadSize();
}

int ResizablePanel::getCurrentSize() const
{
    return currentSize;
}

void ResizablePanel::setCurrentSize(int size)
{
    currentSize = juce::jlimit(minSize, maxSize, size);
    saveSize();
}

juce::Rectangle<int> ResizablePanel::getContentArea() const
{
    auto bounds = getLocalBounds();

    switch (edge)
    {
        case Edge::Top:    bounds.removeFromTop(HANDLE_SIZE); break;
        case Edge::Bottom: bounds.removeFromBottom(HANDLE_SIZE); break;
        case Edge::Left:   bounds.removeFromLeft(HANDLE_SIZE); break;
        case Edge::Right:  bounds.removeFromRight(HANDLE_SIZE); break;
    }

    return bounds;
}

juce::Rectangle<int> ResizablePanel::getDragHandleArea() const
{
    auto bounds = getLocalBounds();

    switch (edge)
    {
        case Edge::Top:    return bounds.removeFromTop(HANDLE_SIZE);
        case Edge::Bottom: return bounds.removeFromBottom(HANDLE_SIZE);
        case Edge::Left:   return bounds.removeFromLeft(HANDLE_SIZE);
        case Edge::Right:  return bounds.removeFromRight(HANDLE_SIZE);
    }

    return {};
}

bool ResizablePanel::isInDragHandle(const juce::Point<int>& pos) const
{
    return getDragHandleArea().contains(pos);
}

void ResizablePanel::paint(juce::Graphics& g)
{
    // Draw the drag handle
    auto handleArea = getDragHandleArea().toFloat();

    // Handle background - slightly different from panel
    auto handleColour = isHoveringHandle || isDragging
                        ? ProgFlowColours::bgHover()
                        : ProgFlowColours::bgSecondary();
    g.setColour(handleColour);
    g.fillRect(handleArea);

    // Draw grip indicator (three lines)
    auto gripColour = isHoveringHandle || isDragging
                      ? ProgFlowColours::textSecondary()
                      : ProgFlowColours::dividerLine();
    g.setColour(gripColour);

    bool isVertical = (edge == Edge::Top || edge == Edge::Bottom);

    if (isVertical)
    {
        // Horizontal grip lines
        float centreX = handleArea.getCentreX();
        float centreY = handleArea.getCentreY();
        float lineLength = 20.0f;

        for (int i = -1; i <= 1; ++i)
        {
            float y = centreY + i * 2.0f;
            g.drawHorizontalLine(static_cast<int>(y),
                                 centreX - lineLength / 2,
                                 centreX + lineLength / 2);
        }
    }
    else
    {
        // Vertical grip lines
        float centreX = handleArea.getCentreX();
        float centreY = handleArea.getCentreY();
        float lineLength = 20.0f;

        for (int i = -1; i <= 1; ++i)
        {
            float x = centreX + i * 2.0f;
            g.drawVerticalLine(static_cast<int>(x),
                               centreY - lineLength / 2,
                               centreY + lineLength / 2);
        }
    }

    // Divider line at edge
    g.setColour(ProgFlowColours::dividerLine());

    switch (edge)
    {
        case Edge::Top:
            g.drawHorizontalLine(static_cast<int>(handleArea.getBottom() - 1),
                                 handleArea.getX(), handleArea.getRight());
            break;
        case Edge::Bottom:
            g.drawHorizontalLine(static_cast<int>(handleArea.getY()),
                                 handleArea.getX(), handleArea.getRight());
            break;
        case Edge::Left:
            g.drawVerticalLine(static_cast<int>(handleArea.getRight() - 1),
                               handleArea.getY(), handleArea.getBottom());
            break;
        case Edge::Right:
            g.drawVerticalLine(static_cast<int>(handleArea.getX()),
                               handleArea.getY(), handleArea.getBottom());
            break;
    }
}

void ResizablePanel::resized()
{
    // Nothing special needed
}

void ResizablePanel::mouseMove(const juce::MouseEvent& e)
{
    bool wasHovering = isHoveringHandle;
    isHoveringHandle = isInDragHandle(e.getPosition());

    if (wasHovering != isHoveringHandle)
    {
        updateMouseCursor();
        repaint();
    }
}

void ResizablePanel::mouseDown(const juce::MouseEvent& e)
{
    if (isInDragHandle(e.getPosition()))
    {
        isDragging = true;
        dragStartSize = currentSize;

        bool isVertical = (edge == Edge::Top || edge == Edge::Bottom);
        dragStartMousePos = isVertical ? e.getScreenY() : e.getScreenX();

        repaint();
    }
}

void ResizablePanel::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging)
        return;

    bool isVertical = (edge == Edge::Top || edge == Edge::Bottom);
    int currentMousePos = isVertical ? e.getScreenY() : e.getScreenX();
    int delta = currentMousePos - dragStartMousePos;

    // Invert delta for top/left edges (dragging up/left increases size)
    if (edge == Edge::Top || edge == Edge::Left)
        delta = -delta;

    int newSize = juce::jlimit(minSize, maxSize, dragStartSize + delta);

    if (newSize != currentSize)
    {
        currentSize = newSize;

        if (onResize)
            onResize(currentSize);
    }
}

void ResizablePanel::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (isDragging)
    {
        isDragging = false;
        saveSize();
        repaint();
    }
}

void ResizablePanel::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (isInDragHandle(e.getPosition()))
    {
        // Toggle between min and default size
        if (currentSize > minSize)
            currentSize = minSize;
        else
            currentSize = defaultSize;

        saveSize();

        if (onResize)
            onResize(currentSize);
    }
}

void ResizablePanel::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (isHoveringHandle && !isDragging)
    {
        isHoveringHandle = false;
        updateMouseCursor();
        repaint();
    }
}

void ResizablePanel::updateMouseCursor()
{
    if (isHoveringHandle || isDragging)
    {
        bool isVertical = (edge == Edge::Top || edge == Edge::Bottom);
        setMouseCursor(isVertical ? juce::MouseCursor::UpDownResizeCursor
                                  : juce::MouseCursor::LeftRightResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void ResizablePanel::saveSize()
{
    // Persistence will be handled by the parent component if needed
    // The preference key is stored but not used directly here
    // Parent can call getCurrentSize() and save it themselves
}

void ResizablePanel::loadSize()
{
    // Persistence will be handled by the parent component if needed
    // Parent should call setCurrentSize() after loading preferences
}
