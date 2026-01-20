#include "RotaryKnob.h"
#include "../../MIDI/MidiLearnManager.h"

RotaryKnob::RotaryKnob(const juce::String& name)
    : name(name)
{
}

RotaryKnob::~RotaryKnob()
{
    // Unregister from MIDI learn manager if we have a parameter ID
    if (parameterId.isNotEmpty())
        MidiLearnManager::getInstance().unregisterParameterCallback(parameterId);
}

void RotaryKnob::setValue(float value, juce::NotificationType notification)
{
    value = juce::jlimit(minValue, maxValue, value);
    value = snapValue(value);

    if (value != currentValue)
    {
        currentValue = value;
        repaint();

        if (notification != juce::dontSendNotification && onValueChange)
            onValueChange(currentValue);
    }
}

void RotaryKnob::setRange(float min, float max, float newInterval)
{
    minValue = min;
    maxValue = max;
    interval = newInterval;
    currentValue = juce::jlimit(minValue, maxValue, currentValue);
    repaint();
}

void RotaryKnob::resized()
{
    auto bounds = getLocalBounds();

    // Label at bottom
    labelArea = bounds.removeFromBottom(LABEL_HEIGHT);

    // Gap
    bounds.removeFromBottom(LABEL_GAP);

    // Knob centered in remaining space
    knobArea = bounds.withSizeKeepingCentre(KNOB_DIAMETER, KNOB_DIAMETER);
}

void RotaryKnob::paint(juce::Graphics& g)
{
    // Use the fixed knob diameter
    auto centreX = static_cast<float>(knobArea.getCentreX());
    auto centreY = static_cast<float>(knobArea.getCentreY());
    auto radius = KNOB_DIAMETER * 0.46f;  // ~24px radius for 52px diameter

    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle = juce::MathConstants<float>::pi * 2.75f;
    float angle = valueToAngle(currentValue);
    float normalizedValue = (currentValue - minValue) / (maxValue - minValue);

    // Outer glow (bloom effect) when there's value
    if (normalizedValue > 0.01f && !midiLearnActive)
    {
        g.setColour(ProgFlowColours::glowBlue());
        g.fillEllipse(centreX - radius - 3, centreY - radius - 3,
                      (radius + 3) * 2, (radius + 3) * 2);
    }

    // Background circle with gradient
    juce::ColourGradient knobGradient(
        ProgFlowColours::knobBodyLight(), centreX, centreY - radius * 0.5f,
        ProgFlowColours::knobBody(), centreX, centreY + radius,
        false);
    g.setGradientFill(knobGradient);
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);

    // Subtle inner shadow for depth
    juce::ColourGradient innerShadow(
        juce::Colour(0x00000000), centreX, centreY,
        juce::Colour(0x25000000), centreX, centreY + radius,
        true);
    g.setGradientFill(innerShadow);
    g.fillEllipse(centreX - radius + 2, centreY - radius + 2,
                  (radius - 2) * 2, (radius - 2) * 2);

    // Border ring
    g.setColour(ProgFlowColours::glassBorder());
    g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, 1.0f);

    // Special state borders (MIDI learn, mapped, dragging)
    if (midiLearnActive)
    {
        g.setColour(ProgFlowColours::accentOrange());
        g.drawEllipse(centreX - radius - 1, centreY - radius - 1,
                      (radius + 1) * 2, (radius + 1) * 2, 2.0f);
    }
    else if (isDragging)
    {
        g.setColour(ProgFlowColours::accentBlue());
        g.drawEllipse(centreX - radius - 1, centreY - radius - 1,
                      (radius + 1) * 2, (radius + 1) * 2, 2.0f);
    }
    else if (hasMidiMapping)
    {
        g.setColour(ProgFlowColours::accentGreen());
        g.drawEllipse(centreX - radius - 1, centreY - radius - 1,
                      (radius + 1) * 2, (radius + 1) * 2, 1.5f);
    }
    else if (isHovering)
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.4f));
        g.drawEllipse(centreX - radius - 1, centreY - radius - 1,
                      (radius + 1) * 2, (radius + 1) * 2, 1.5f);
    }

    // Arc background (inactive portion)
    juce::Path arcBgPath;
    arcBgPath.addCentredArc(centreX, centreY,
                            radius * 0.78f, radius * 0.78f,
                            0.0f,
                            startAngle, endAngle,
                            true);
    g.setColour(ProgFlowColours::knobArcBg());
    g.strokePath(arcBgPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    // Arc (value indicator with glow)
    if (normalizedValue > 0.01f)
    {
        juce::Path arcPath;
        arcPath.addCentredArc(centreX, centreY,
                              radius * 0.78f, radius * 0.78f,
                              0.0f,
                              startAngle, angle,
                              true);

        // Glow layer
        auto glowColour = midiLearnActive ? ProgFlowColours::glowOrange() : ProgFlowColours::glowBlue();
        g.setColour(glowColour);
        g.strokePath(arcPath, juce::PathStrokeType(6.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

        // Main arc
        auto arcColour = midiLearnActive ? ProgFlowColours::accentOrange()
                       : (isDragging ? ProgFlowColours::accentBlue().brighter(0.2f)
                                     : ProgFlowColours::accentBlue());
        g.setColour(arcColour);
        g.strokePath(arcPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // Position indicator dot
    float indicatorRadius = radius * 0.62f;
    float indicatorX = centreX + std::sin(angle) * indicatorRadius;
    float indicatorY = centreY - std::cos(angle) * indicatorRadius;

    g.setColour(ProgFlowColours::knobIndicator());
    g.fillEllipse(indicatorX - 3.0f, indicatorY - 3.0f, 6.0f, 6.0f);

    // Label
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(12.0f);
    g.drawText(label.isEmpty() ? name : label, labelArea.toFloat(), juce::Justification::centred);
}

void RotaryKnob::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown() || e.mods.isPopupMenu())
    {
        showContextMenu();
        return;
    }

    isDragging = true;
    dragStartValue = currentValue;
    dragStartY = e.y;
    repaint();
}

void RotaryKnob::mouseDrag(const juce::MouseEvent& e)
{
    if (!isDragging)
        return;

    float sensitivity = e.mods.isShiftDown() ? fineSensitivity : normalSensitivity;
    float deltaY = static_cast<float>(dragStartY - e.y); // Inverted: drag up = increase
    float range = maxValue - minValue;
    float valueDelta = deltaY * sensitivity * range;

    setValue(dragStartValue + valueDelta, juce::sendNotification);
}

void RotaryKnob::mouseUp(const juce::MouseEvent& /*e*/)
{
    isDragging = false;
    repaint();
}

void RotaryKnob::mouseDoubleClick(const juce::MouseEvent& /*e*/)
{
    // Reset to default
    setValue(defaultValue, juce::sendNotification);
}

void RotaryKnob::mouseEnter(const juce::MouseEvent& /*e*/)
{
    isHovering = true;
    repaint();
}

void RotaryKnob::mouseExit(const juce::MouseEvent& /*e*/)
{
    isHovering = false;
    repaint();
}

float RotaryKnob::valueToAngle(float value) const
{
    float normalized = (value - minValue) / (maxValue - minValue);
    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle = juce::MathConstants<float>::pi * 2.75f;
    return startAngle + normalized * (endAngle - startAngle);
}

float RotaryKnob::angleToValue(float angle) const
{
    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle = juce::MathConstants<float>::pi * 2.75f;
    float normalized = (angle - startAngle) / (endAngle - startAngle);
    return minValue + normalized * (maxValue - minValue);
}

float RotaryKnob::snapValue(float value) const
{
    if (interval > 0.0f)
    {
        value = std::round((value - minValue) / interval) * interval + minValue;
    }
    return juce::jlimit(minValue, maxValue, value);
}

//==============================================================================
// MIDI Learn

void RotaryKnob::setParameterId(const juce::String& id)
{
    // Unregister old callback
    if (parameterId.isNotEmpty())
        MidiLearnManager::getInstance().unregisterParameterCallback(parameterId);

    parameterId = id;

    if (parameterId.isNotEmpty())
    {
        // Check if we already have a mapping
        hasMidiMapping = MidiLearnManager::getInstance().hasMapping(parameterId);

        // Register callback for this parameter
        MidiLearnManager::getInstance().registerParameterCallback(parameterId,
            [this](float normalizedValue) { onMidiCcValueChange(normalizedValue); });

        repaint();
    }
}

void RotaryKnob::showContextMenu()
{
    juce::PopupMenu menu;

    enum MenuItems { MidiLearn = 1, RemoveMapping, EnterValue, ResetToDefault };

    // MIDI Learn option
    if (parameterId.isNotEmpty())
    {
        if (hasMidiMapping)
        {
            auto* mapping = MidiLearnManager::getInstance().getMapping(parameterId);
            juce::String ccText = mapping ? juce::String("CC ") + juce::String(mapping->ccNumber) : "";
            menu.addItem(MidiLearn, "MIDI Learn (" + ccText + ")");
            menu.addItem(RemoveMapping, "Remove MIDI Mapping");
        }
        else
        {
            menu.addItem(MidiLearn, "MIDI Learn");
        }
        menu.addSeparator();
    }

    menu.addItem(EnterValue, "Enter Value...");
    menu.addItem(ResetToDefault, "Reset to Default");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
        [this](int result)
        {
            switch (result)
            {
                case MidiLearn:
                    startMidiLearn();
                    break;
                case RemoveMapping:
                    removeMidiMapping();
                    break;
                case EnterValue:
                    showEnterValueDialog();
                    break;
                case ResetToDefault:
                    setValue(defaultValue, juce::sendNotification);
                    break;
                default:
                    break;
            }
        });
}

void RotaryKnob::startMidiLearn()
{
    if (parameterId.isEmpty())
        return;

    midiLearnActive = true;
    repaint();

    MidiLearnManager::getInstance().startLearning(parameterId,
        [this](int channel, int cc)
        {
            midiLearnActive = false;

            if (channel >= 0)
            {
                // Learning succeeded
                hasMidiMapping = true;
                DBG("MIDI Learn: " + parameterId + " -> CC " + juce::String(cc));
            }
            // channel < 0 means cancelled

            repaint();
        });
}

void RotaryKnob::removeMidiMapping()
{
    if (parameterId.isEmpty())
        return;

    MidiLearnManager::getInstance().removeMapping(parameterId);
    hasMidiMapping = false;
    repaint();
}

void RotaryKnob::showEnterValueDialog()
{
    auto* editor = new juce::AlertWindow("Enter Value",
                                         "Enter a value between " + juce::String(minValue) +
                                         " and " + juce::String(maxValue),
                                         juce::AlertWindow::NoIcon);

    editor->addTextEditor("value", juce::String(currentValue, 2));
    editor->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    editor->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    editor->enterModalState(true, juce::ModalCallbackFunction::create(
        [this, editor](int result)
        {
            if (result == 1)
            {
                auto text = editor->getTextEditorContents("value");
                float newValue = text.getFloatValue();
                setValue(newValue, juce::sendNotification);
            }
        }), true);
}

void RotaryKnob::onMidiCcValueChange(float normalizedValue)
{
    // Map normalized 0-1 value to our range
    float newValue = minValue + normalizedValue * (maxValue - minValue);
    setValue(newValue, juce::sendNotification);
}

juce::String RotaryKnob::getTooltip()
{
    // If custom tooltip text is set, use that
    if (tooltipText.isNotEmpty())
        return tooltipText;

    // Generate tooltip from label and current value
    juce::String tooltip;

    if (label.isNotEmpty())
        tooltip = label;
    else if (name.isNotEmpty())
        tooltip = name;

    if (tooltip.isNotEmpty())
    {
        tooltip += ": ";

        // Format value
        if (interval >= 1.0f)
            tooltip += juce::String(static_cast<int>(currentValue));
        else
            tooltip += juce::String(currentValue, 2);

        tooltip += valueSuffix;

        // Add range hint
        tooltip += " (";
        if (interval >= 1.0f)
            tooltip += juce::String(static_cast<int>(minValue)) + " - " + juce::String(static_cast<int>(maxValue));
        else
            tooltip += juce::String(minValue, 1) + " - " + juce::String(maxValue, 1);
        tooltip += ")";
    }

    return tooltip;
}
