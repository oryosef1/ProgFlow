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
    auto radius = KNOB_DIAMETER * 0.46f;  // ~44px diameter for 48px area

    // Background circle - darker, no border by default
    g.setColour(ProgFlowColours::sectionBg());
    g.fillEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2);

    // Hover glow ring
    if (isHovering && !isDragging && !midiLearnActive)
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.3f));
        g.drawEllipse(centreX - radius - 1, centreY - radius - 1,
                      radius * 2 + 2, radius * 2 + 2, 1.5f);
    }

    // Special state borders (MIDI learn, mapped, dragging)
    if (midiLearnActive)
    {
        g.setColour(ProgFlowColours::accentOrange());
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, 2.0f);
    }
    else if (isDragging)
    {
        g.setColour(ProgFlowColours::accentBlue());
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, 2.0f);
    }
    else if (hasMidiMapping)
    {
        g.setColour(ProgFlowColours::accentGreen());
        g.drawEllipse(centreX - radius, centreY - radius, radius * 2, radius * 2, 1.5f);
    }

    // Arc (value indicator)
    float startAngle = juce::MathConstants<float>::pi * 1.25f;
    float endAngle = juce::MathConstants<float>::pi * 2.75f;
    float angle = valueToAngle(currentValue);

    juce::Path arcPath;
    arcPath.addCentredArc(centreX, centreY,
                          radius * 0.82f, radius * 0.82f,
                          0.0f,
                          startAngle, angle,
                          true);

    // Arc color - brighter when dragging
    auto arcColour = isDragging ? ProgFlowColours::accentBlue().brighter(0.2f)
                                : ProgFlowColours::accentBlue();
    g.setColour(arcColour);
    g.strokePath(arcPath, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded));

    // Pointer line - clean, no dot
    float pointerLength = radius * 0.55f;
    float pointerEndX = centreX + std::sin(angle) * pointerLength;
    float pointerEndY = centreY - std::cos(angle) * pointerLength;

    g.setColour(ProgFlowColours::textPrimary());
    g.drawLine(centreX, centreY, pointerEndX, pointerEndY, 2.0f);

    // Label - always visible
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(11.0f);
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
