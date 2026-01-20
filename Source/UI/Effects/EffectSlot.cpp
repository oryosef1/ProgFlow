#include "EffectSlot.h"

EffectSlot::EffectSlot(int index)
    : slotIndex(index)
{
    // Name label
    nameLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    nameLabel.setColour(juce::Label::textColourId, ProgFlowColours::textPrimary());
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setText("Empty Slot", juce::dontSendNotification);
    addAndMakeVisible(nameLabel);

    // Remove button
    removeButton.setButtonText("X");
    removeButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    removeButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::accentRed());
    removeButton.onClick = [this]()
    {
        if (onRemoveClicked)
            onRemoveClicked(slotIndex);
    };
    addAndMakeVisible(removeButton);

    // Bypass button
    bypassButton.setButtonText("BYP");
    bypassButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this]()
    {
        bypassed = bypassButton.getToggleState();
        bypassButton.setColour(juce::TextButton::textColourOffId,
                               bypassed ? ProgFlowColours::accentRed() : ProgFlowColours::textSecondary());
        if (currentEffect)
            currentEffect->setBypass(bypassed);
        if (onBypassToggled)
            onBypassToggled(slotIndex, bypassed);
        repaint();
    };
    addAndMakeVisible(bypassButton);

    // Wet/dry knob
    wetDryKnob.setLabel("Mix");
    wetDryKnob.setRange(0.0f, 1.0f, 0.01f);
    wetDryKnob.setDefaultValue(1.0f);
    wetDryKnob.setValue(1.0f, juce::dontSendNotification);
    wetDryKnob.onValueChange = [this](float value)
    {
        if (currentEffect)
            currentEffect->setWetDry(value);
    };
    addAndMakeVisible(wetDryKnob);

    // Create parameter knobs (initially hidden)
    for (int i = 0; i < MAX_VISIBLE_PARAMS; ++i)
    {
        paramKnobs[i] = std::make_unique<RotaryKnob>();
        paramKnobs[i]->setVisible(false);
        addAndMakeVisible(*paramKnobs[i]);
    }

    updateFromEffect();
}

EffectSlot::~EffectSlot() = default;

void EffectSlot::setEffect(EffectBase* effect)
{
    currentEffect = effect;
    updateFromEffect();
    repaint();
}

void EffectSlot::setBypass(bool shouldBypass)
{
    bypassed = shouldBypass;
    bypassButton.setToggleState(bypassed, juce::dontSendNotification);
    bypassButton.setColour(juce::TextButton::textColourOffId,
                           bypassed ? ProgFlowColours::accentRed() : ProgFlowColours::textSecondary());
    if (currentEffect)
        currentEffect->setBypass(bypassed);
    repaint();
}

void EffectSlot::updateFromEffect()
{
    if (currentEffect)
    {
        nameLabel.setText(currentEffect->getName(), juce::dontSendNotification);
        wetDryKnob.setValue(currentEffect->getWetDry(), juce::dontSendNotification);
        wetDryKnob.setVisible(true);
        removeButton.setVisible(true);
        bypassButton.setVisible(true);

        // Setup parameter knobs
        auto paramNames = currentEffect->getParameterNames();
        int numParams = std::min((int)paramNames.size(), MAX_VISIBLE_PARAMS);

        for (int i = 0; i < MAX_VISIBLE_PARAMS; ++i)
        {
            if (i < numParams)
            {
                const auto& paramId = paramNames[i];
                if (auto* param = currentEffect->getParameterInfo(paramId))
                {
                    setupParamKnob(i, paramId, *param);
                    paramKnobs[i]->setVisible(true);
                }
            }
            else
            {
                paramKnobs[i]->setVisible(false);
            }
        }
    }
    else
    {
        nameLabel.setText("Empty Slot", juce::dontSendNotification);
        wetDryKnob.setVisible(false);
        removeButton.setVisible(false);
        bypassButton.setVisible(false);

        for (auto& knob : paramKnobs)
            knob->setVisible(false);
    }

    resized();
}

void EffectSlot::setupParamKnob(int index, const juce::String& paramId, const EffectParameter& param)
{
    auto& knob = *paramKnobs[index];

    knob.setLabel(param.name);
    knob.setValueSuffix(param.unit);
    knob.setRange(param.minValue, param.maxValue, param.step > 0 ? param.step : 0.01f);
    knob.setDefaultValue(param.defaultValue);
    knob.setValue(param.value, juce::dontSendNotification);

    knob.onValueChange = [this, paramId](float value)
    {
        if (currentEffect)
            currentEffect->setParameter(paramId, value);
    };
}

void EffectSlot::paint(juce::Graphics& g)
{
    // Background
    auto bgColour = currentEffect ? ProgFlowColours::bgSecondary() : ProgFlowColours::bgTertiary();
    if (bypassed && currentEffect)
        bgColour = bgColour.withMultipliedBrightness(0.7f);
    if (dragHovered)
        bgColour = bgColour.brighter(0.1f);

    g.fillAll(bgColour);

    // Border
    g.setColour(ProgFlowColours::border());
    g.drawRect(getLocalBounds(), 1);

    // Slot number
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(juce::Font(10.0f));
    g.drawText(juce::String(slotIndex + 1), 4, 4, 20, 14, juce::Justification::centredLeft);
}

void EffectSlot::resized()
{
    const int margin = 4;
    const int buttonSize = 24;
    const int knobSize = 50;

    auto bounds = getLocalBounds().reduced(margin);

    // Top row: name + buttons
    auto topRow = bounds.removeFromTop(buttonSize);
    topRow.removeFromLeft(20);  // Space for slot number

    auto buttonArea = topRow.removeFromRight(buttonSize * 2 + margin);
    removeButton.setBounds(buttonArea.removeFromRight(buttonSize));
    buttonArea.removeFromRight(margin);
    bypassButton.setBounds(buttonArea.removeFromRight(buttonSize));

    nameLabel.setBounds(topRow);

    bounds.removeFromTop(margin);

    // Parameter knobs row
    if (currentEffect)
    {
        auto knobRow = bounds.removeFromTop(knobSize);
        int visibleKnobs = 0;
        for (const auto& knob : paramKnobs)
            if (knob->isVisible())
                visibleKnobs++;

        for (int i = 0; i < MAX_VISIBLE_PARAMS; ++i)
        {
            if (paramKnobs[i]->isVisible())
            {
                paramKnobs[i]->setBounds(knobRow.removeFromLeft(knobSize));
            }
        }

        bounds.removeFromTop(margin);

        // Wet/dry knob at bottom
        wetDryKnob.setBounds(bounds.removeFromLeft(knobSize));
    }
}

// Drag and drop support
void EffectSlot::mouseDown(const juce::MouseEvent& e)
{
    if (currentEffect && e.mods.isLeftButtonDown())
    {
        // Start drag
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
        {
            container->startDragging("EffectSlot:" + juce::String(slotIndex), this);
        }
    }
}

void EffectSlot::mouseDrag(const juce::MouseEvent& e)
{
    // Handled by DragAndDropContainer
}

bool EffectSlot::isInterestedInDragSource(const SourceDetails& details)
{
    return details.description.toString().startsWith("EffectSlot:");
}

void EffectSlot::itemDropped(const SourceDetails& details)
{
    dragHovered = false;

    auto desc = details.description.toString();
    if (desc.startsWith("EffectSlot:"))
    {
        int fromSlot = desc.fromFirstOccurrenceOf(":", false, false).getIntValue();
        if (fromSlot != slotIndex && onEffectDropped)
        {
            onEffectDropped(fromSlot, slotIndex);
        }
    }

    repaint();
}

void EffectSlot::itemDragEnter(const SourceDetails& details)
{
    dragHovered = true;
    repaint();
}

void EffectSlot::itemDragExit(const SourceDetails& details)
{
    dragHovered = false;
    repaint();
}
