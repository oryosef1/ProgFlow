#include "CardPanel.h"

CardPanel::CardPanel(const juce::String& titleText)
    : title(titleText)
{
    showHeader = title.isNotEmpty();
}

void CardPanel::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    showHeader = title.isNotEmpty();
    repaint();
}

void CardPanel::setShowHeader(bool show)
{
    showHeader = show;
    repaint();
}

void CardPanel::setCornerRadius(float radius)
{
    cornerRadius = radius;
    repaint();
}

void CardPanel::setPadding(int newPadding)
{
    padding = newPadding;
    repaint();
}

juce::Rectangle<int> CardPanel::getContentArea() const
{
    auto bounds = getLocalBounds();

    if (showHeader)
    {
        bounds.removeFromTop(HEADER_HEIGHT);
        bounds.removeFromTop(HEADER_BOTTOM_MARGIN);
    }

    return bounds.reduced(padding);
}

void CardPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Draw soft shadow first (offset down)
    auto shadowBounds = bounds.translated(0.0f, 4.0f);
    g.setColour(juce::Colour(0x33000000));  // ~20% black
    g.fillRoundedRectangle(shadowBounds.reduced(2.0f), cornerRadius + 2.0f);

    // Draw card background with gradient
    drawCardBackground(g, bounds);

    // Draw header if enabled
    if (showHeader)
    {
        auto headerBounds = bounds.removeFromTop(static_cast<float>(HEADER_HEIGHT));
        drawHeader(g, headerBounds);
    }

    // Subtle border
    g.setColour(juce::Colour(0x0dffffff));  // ~5% white
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);
}

void CardPanel::drawCardBackground(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Gradient from bgSurface (top) to bgSecondary (bottom)
    // This creates subtle depth - lighter at top, darker at bottom
    juce::ColourGradient gradient(
        ProgFlowColours::bgTertiary(), bounds.getX(), bounds.getY(),
        ProgFlowColours::bgSecondary(), bounds.getX(), bounds.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);
}

void CardPanel::drawHeader(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Draw title - 11px uppercase, textMuted color, letter-spaced
    g.setColour(ProgFlowColours::textMuted());

    // Create font with letter spacing (using character spacing)
    juce::Font headerFont(juce::FontOptions(11.0f));
    headerFont.setExtraKerningFactor(0.1f);  // Add letter spacing
    g.setFont(headerFont);

    auto textBounds = bounds.reduced(static_cast<float>(padding), 0);
    g.drawText(title.toUpperCase(), textBounds, juce::Justification::centredLeft);
}

void CardPanel::resized()
{
    // Nothing special needed - content area is calculated dynamically
}
