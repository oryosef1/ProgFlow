#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * RotaryKnob - Custom knob control with vertical drag
 *
 * Features:
 * - Vertical drag to change value (drag up = increase)
 * - SHIFT for fine control (10x finer)
 * - Double-click to reset to default
 * - Right-click context menu with:
 *   - MIDI Learn - assign a CC to control this knob
 *   - Remove MIDI Mapping - clear the CC assignment
 *   - Enter Value - type an exact value
 * - Tooltip on hover showing parameter name and current value
 */
class RotaryKnob : public juce::Component,
                   public juce::TooltipClient
{
public:
    static constexpr int KNOB_DIAMETER = 48;
    static constexpr int LABEL_HEIGHT = 16;
    static constexpr int LABEL_GAP = 4;
    static constexpr int TOTAL_HEIGHT = KNOB_DIAMETER + LABEL_GAP + LABEL_HEIGHT; // 68px

    RotaryKnob(const juce::String& name = "");
    ~RotaryKnob() override;

    //==========================================================================
    // Value control
    void setValue(float value, juce::NotificationType notification = juce::sendNotification);
    float getValue() const { return currentValue; }

    void setRange(float min, float max, float interval = 0.0f);
    void setDefaultValue(float defaultVal) { defaultValue = defaultVal; }

    // Callback when value changes
    std::function<void(float)> onValueChange;

    //==========================================================================
    // Appearance
    void setLabel(const juce::String& newLabel) { label = newLabel; repaint(); }
    void setValueSuffix(const juce::String& suffix) { valueSuffix = suffix; }
    void setShowValue(bool show) { showValue = show; repaint(); }
    void setTooltipText(const juce::String& tip) { tooltipText = tip; }

    // TooltipClient
    juce::String getTooltip() override;

    //==========================================================================
    // MIDI Learn
    /**
     * Set unique parameter ID for MIDI mapping
     * Format suggestion: "synth.filter.cutoff" or "track.0.volume"
     */
    void setParameterId(const juce::String& id);
    juce::String getParameterId() const { return parameterId; }

    /**
     * Check if this knob is currently in MIDI learn mode
     */
    bool isInMidiLearnMode() const { return midiLearnActive; }

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    juce::String name;
    juce::String label;
    juce::String valueSuffix;
    juce::String tooltipText;

    float currentValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
    float interval = 0.0f;
    float defaultValue = 0.0f;

    bool showValue = true;
    bool isDragging = false;
    bool isHovering = false;
    float dragStartValue = 0.0f;
    int dragStartY = 0;

    // Sensitivity
    static constexpr float normalSensitivity = 0.005f;
    static constexpr float fineSensitivity = 0.0005f;

    // MIDI Learn
    juce::String parameterId;
    bool midiLearnActive = false;
    bool hasMidiMapping = false;

    // Convert between value and angle
    float valueToAngle(float value) const;
    float angleToValue(float angle) const;

    // Snap value to interval
    float snapValue(float value) const;

    // Context menu
    void showContextMenu();
    void startMidiLearn();
    void removeMidiMapping();
    void showEnterValueDialog();

    // Called by MidiLearnManager when CC value changes
    void onMidiCcValueChange(float normalizedValue);

    // Layout areas (calculated in resized())
    juce::Rectangle<int> knobArea;
    juce::Rectangle<int> labelArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RotaryKnob)
};
