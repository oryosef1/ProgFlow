#include "SectionPanel.h"

SectionPanel::SectionPanel(const juce::String& title)
    : title(title)
{
}

void SectionPanel::setTitle(const juce::String& newTitle)
{
    title = newTitle;
    repaint();
}

juce::Rectangle<int> SectionPanel::getContentBounds() const
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(HEADER_HEIGHT);
    return bounds.reduced(PADDING);
}

void SectionPanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.setColour(ProgFlowColours::sectionBg());
    g.fillRoundedRectangle(bounds, static_cast<float>(CORNER_RADIUS));

    // Title
    if (title.isNotEmpty())
    {
        auto titleBounds = bounds.removeFromTop(static_cast<float>(HEADER_HEIGHT))
                                 .reduced(static_cast<float>(PADDING), 0.0f);
        titleBounds.removeFromTop(6.0f);  // Top padding for title

        g.setColour(ProgFlowColours::textMuted());

        // Uppercase, small font with letter spacing
        juce::Font titleFont(10.0f);
        g.setFont(titleFont);

        // Draw with letter spacing effect (draw each char manually)
        juce::String upperTitle = title.toUpperCase();
        float letterSpacing = 1.0f;
        float totalWidth = 0.0f;

        // Calculate total width
        for (int i = 0; i < upperTitle.length(); ++i)
        {
            totalWidth += titleFont.getStringWidthFloat(upperTitle.substring(i, i + 1));
            if (i < upperTitle.length() - 1)
                totalWidth += letterSpacing;
        }

        // Draw centered or left-aligned
        float x = titleBounds.getX();
        float y = titleBounds.getY();

        for (int i = 0; i < upperTitle.length(); ++i)
        {
            juce::String ch = upperTitle.substring(i, i + 1);
            g.drawText(ch, static_cast<int>(x), static_cast<int>(y),
                       static_cast<int>(titleFont.getStringWidthFloat(ch) + letterSpacing),
                       static_cast<int>(titleBounds.getHeight()),
                       juce::Justification::left);
            x += titleFont.getStringWidthFloat(ch) + letterSpacing;
        }
    }
}

void SectionPanel::resized()
{
    // Child components should be positioned relative to getContentBounds()
}
