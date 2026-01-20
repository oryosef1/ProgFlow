#include "SynthEditorBase.h"

SynthEditorBase::SynthEditorBase()
{
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    presetLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    presetLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(presetLabel);

    addAndMakeVisible(presetSelector);

    masterLabel.setText("VOLUME", juce::dontSendNotification);
    masterLabel.setFont(juce::Font(juce::FontOptions(11.0f)));
    masterLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    masterLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(masterLabel);

    masterVolume.setLabel("Vol");
    addAndMakeVisible(masterVolume);
}

SynthEditorBase::~SynthEditorBase() {}

void SynthEditorBase::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());
    drawHorizontalDivider(g, 0, getWidth(), HEADER_HEIGHT);
    drawDividers(g, getContentArea());
}

void SynthEditorBase::resized()
{
    auto bounds = getLocalBounds();
    layoutHeader(bounds.removeFromTop(HEADER_HEIGHT));
    layoutContent(bounds);
}

void SynthEditorBase::layoutHeader(juce::Rectangle<int> area)
{
    area = area.reduced(ProgFlowSpacing::MD, ProgFlowSpacing::SM);

    // Left side: Preset controls (vertically centered)
    presetLabel.setBounds(area.getX(), area.getCentreY() - 20, 55, 18);
    presetSelector.setBounds(area.getX() + 60, area.getCentreY() - 14, 200, ProgFlowSpacing::COMBO_HEIGHT);

    // Right side: Master volume knob
    int masterKnobHeight = RotaryKnob::TOTAL_HEIGHT;
    int masterX = area.getRight() - KNOB_SIZE - ProgFlowSpacing::MD;
    masterLabel.setBounds(masterX - 15, area.getY(), KNOB_SIZE + 30, 16);
    masterVolume.setBounds(masterX, area.getY() + 14, KNOB_SIZE, masterKnobHeight);
}

juce::Rectangle<int> SynthEditorBase::getContentArea()
{
    return getLocalBounds().withTrimmedTop(HEADER_HEIGHT);
}

void SynthEditorBase::drawVerticalDivider(juce::Graphics& g, int x, int yStart, int yEnd)
{
    g.setColour(ProgFlowColours::dividerLine());
    g.fillRect(x, yStart, ProgFlowSpacing::DIVIDER_WIDTH, yEnd - yStart);
}

void SynthEditorBase::drawHorizontalDivider(juce::Graphics& g, int xStart, int xEnd, int y)
{
    g.setColour(ProgFlowColours::dividerLine());
    g.fillRect(xStart, y, xEnd - xStart, ProgFlowSpacing::DIVIDER_WIDTH);
}

void SynthEditorBase::createSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text.toUpperCase(), juce::dontSendNotification);
    label.setFont(juce::Font(juce::FontOptions(11.0f)));
    label.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    label.setJustificationType(juce::Justification::centredLeft);
}

void SynthEditorBase::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    juce::ignoreUnused(g, area);
}

void SynthEditorBase::drawSectionBox(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    auto boundsF = bounds.toFloat();
    float radius = static_cast<float>(ProgFlowSpacing::GLASS_CORNER_RADIUS);

    // Draw glass background
    g.setColour(ProgFlowColours::glassOverlay());
    g.fillRoundedRectangle(boundsF, radius);

    // Draw subtle gradient overlay for depth
    juce::ColourGradient gradient(
        juce::Colour(0x08ffffff), boundsF.getX(), boundsF.getY(),
        juce::Colour(0x00000000), boundsF.getX(), boundsF.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(boundsF, radius);

    // Draw border
    g.setColour(ProgFlowColours::glassBorder());
    g.drawRoundedRectangle(boundsF.reduced(0.5f), radius, 1.0f);

    // Draw title
    if (title.isNotEmpty())
    {
        g.setColour(ProgFlowColours::textMuted());
        g.setFont(juce::Font(juce::FontOptions(11.0f)));
        g.drawText(title.toUpperCase(), bounds.getX() + 10, bounds.getY() + 6,
                   bounds.getWidth() - 20, 16, juce::Justification::centredLeft);
    }
}
