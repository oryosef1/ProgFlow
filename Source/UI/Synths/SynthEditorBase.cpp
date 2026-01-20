#include "SynthEditorBase.h"

SynthEditorBase::SynthEditorBase()
{
    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setFont(juce::Font(10.0f));
    presetLabel.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    presetLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(presetLabel);

    addAndMakeVisible(presetSelector);

    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(juce::Font(10.0f));
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
    area = area.reduced(ProgFlowSpacing::MD, 4);

    // Left side: Preset controls (vertically centered)
    presetLabel.setBounds(area.getX(), area.getCentreY() - 8, 50, 16);
    presetSelector.setBounds(area.getX() + 54, area.getCentreY() - 12, 180, 24);

    // Right side: Master volume knob - positioned to not overlap label
    int masterX = area.getRight() - KNOB_SIZE - 10;
    masterLabel.setBounds(masterX - 10, 4, KNOB_SIZE + 20, 14);
    masterVolume.setBounds(masterX, 16, KNOB_SIZE, KNOB_SIZE + 20);
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
    label.setFont(juce::Font(10.0f));
    label.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    label.setJustificationType(juce::Justification::centredLeft);
}

void SynthEditorBase::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    juce::ignoreUnused(g, area);
}

void SynthEditorBase::drawSectionBox(juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& title)
{
    // Draw rounded box background
    g.setColour(ProgFlowColours::sectionBg());
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    // Draw border
    g.setColour(ProgFlowColours::border());
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);

    // Draw title
    if (title.isNotEmpty())
    {
        g.setColour(ProgFlowColours::textMuted());
        g.setFont(juce::Font(10.0f));
        g.drawText(title, bounds.getX() + 8, bounds.getY() + 4, bounds.getWidth() - 16, 14,
                   juce::Justification::centredLeft);
    }
}
