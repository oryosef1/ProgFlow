#include "GlassPanel.h"

GlassPanel::GlassPanel(const juce::String& titleText)
    : title(titleText)
{
    showHeader = title.isNotEmpty();
}

void GlassPanel::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    showHeader = title.isNotEmpty();
    repaint();
}

void GlassPanel::setShowHeader(bool show)
{
    showHeader = show;
    repaint();
}

void GlassPanel::setCornerRadius(float radius)
{
    cornerRadius = radius;
    repaint();
}

void GlassPanel::setGlowOnHover(bool glow)
{
    glowOnHover = glow;
}

void GlassPanel::setCustomBackground(juce::Colour colour)
{
    customBgColour = colour;
    useCustomBg = true;
    repaint();
}

juce::Rectangle<int> GlassPanel::getContentArea() const
{
    auto bounds = getLocalBounds();

    if (showHeader)
        bounds.removeFromTop(HEADER_HEIGHT);

    return bounds.reduced(CONTENT_PADDING);
}

void GlassPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Draw shadow first (offset down and right)
    auto shadowBounds = bounds.translated(2.0f, 3.0f);
    g.setColour(juce::Colour(0x30000000));
    g.fillRoundedRectangle(shadowBounds, cornerRadius);

    // Draw glass background
    drawGlassBackground(g, bounds);

    // Draw header if enabled
    if (showHeader)
    {
        auto headerBounds = bounds.removeFromTop(HEADER_HEIGHT);
        drawHeader(g, headerBounds);
    }

    // Draw border
    drawBorder(g, getLocalBounds().toFloat());
}

void GlassPanel::drawGlassBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Base background colour
    auto bgColour = useCustomBg ? customBgColour : ProgFlowColours::bgSecondary();

    // Fill main background
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Glass overlay (frosted effect simulation)
    // Multiple layers create depth
    auto glassColour = ProgFlowColours::glassOverlay();

    // Gradient from top (lighter) to bottom (darker) for depth
    juce::ColourGradient glassGradient(
        glassColour.brighter(0.1f), bounds.getX(), bounds.getY(),
        juce::Colour(0x00000000), bounds.getX(), bounds.getBottom(),
        false);
    g.setGradientFill(glassGradient);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Subtle highlight at top edge
    juce::Path highlightPath;
    highlightPath.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                       bounds.getWidth(), 1.0f,
                                       cornerRadius, cornerRadius, true, true, false, false);
    g.setColour(juce::Colour(0x15ffffff));
    g.fillPath(highlightPath);
}

void GlassPanel::drawHeader(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Slightly different background for header (only round top corners)
    g.setColour(ProgFlowColours::glassOverlay());
    juce::Path headerPath;
    headerPath.addRoundedRectangle(bounds.getX(), bounds.getY(),
                                    bounds.getWidth(), bounds.getHeight(),
                                    cornerRadius, cornerRadius, true, true, false, false);
    g.fillPath(headerPath);

    // Draw title
    g.setColour(ProgFlowColours::textMuted());
    g.setFont(juce::Font(juce::FontOptions(11.0f)).boldened());

    auto textBounds = bounds.reduced(CONTENT_PADDING, 0);
    g.drawText(title.toUpperCase(), textBounds, juce::Justification::centredLeft);

    // Divider line below header
    g.setColour(ProgFlowColours::dividerLine());
    g.drawHorizontalLine(static_cast<int>(bounds.getBottom() - 1),
                         bounds.getX() + CONTENT_PADDING,
                         bounds.getRight() - CONTENT_PADDING);
}

void GlassPanel::drawBorder(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Border colour - glow if hovering and glow enabled
    auto borderColour = (glowOnHover && isHovering)
                        ? ProgFlowColours::borderGlow()
                        : ProgFlowColours::glassBorder();

    // Draw multiple layers for glow effect
    if (glowOnHover && isHovering)
    {
        // Outer glow
        g.setColour(ProgFlowColours::glowBlue());
        g.drawRoundedRectangle(bounds.expanded(1.0f), cornerRadius + 1.0f, 2.0f);
    }

    // Main border
    g.setColour(borderColour);
    g.drawRoundedRectangle(bounds, cornerRadius, 1.0f);
}

void GlassPanel::resized()
{
    // Nothing special needed - content area is calculated dynamically
}

void GlassPanel::mouseEnter(const juce::MouseEvent& /*e*/)
{
    if (glowOnHover)
    {
        isHovering = true;
        repaint();
    }
}

void GlassPanel::mouseExit(const juce::MouseEvent& /*e*/)
{
    if (glowOnHover)
    {
        isHovering = false;
        repaint();
    }
}
