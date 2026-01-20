#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Audio/Effects/EffectBase.h"
#include "../Common/RotaryKnob.h"
#include "../LookAndFeel.h"

/**
 * EffectSlot - UI component for a single effect in the chain
 *
 * Layout:
 * ┌──────────────────────────────────────┐
 * │ [Effect Name]             [X] [Byp]  │
 * │ ┌──────────────────────────────────┐ │
 * │ │ [Param1] [Param2] [Param3] ...   │ │
 * │ └──────────────────────────────────┘ │
 * │ [Wet/Dry]                            │
 * └──────────────────────────────────────┘
 */
class EffectSlot : public juce::Component,
                   public juce::DragAndDropTarget
{
public:
    EffectSlot(int slotIndex);
    ~EffectSlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Set the effect for this slot (can be nullptr for empty slot)
    void setEffect(EffectBase* effect);
    EffectBase* getEffect() const { return currentEffect; }

    int getSlotIndex() const { return slotIndex; }

    // Bypass state
    void setBypass(bool bypassed);
    bool isBypassed() const { return bypassed; }

    // Callbacks
    std::function<void(int)> onRemoveClicked;
    std::function<void(int, bool)> onBypassToggled;
    std::function<void(int, int)> onEffectDropped;  // fromSlot, toSlot

    // Drag and drop
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    int slotIndex = 0;
    EffectBase* currentEffect = nullptr;
    bool bypassed = false;
    bool dragHovered = false;

    // UI Components
    juce::Label nameLabel;
    juce::TextButton removeButton;
    juce::TextButton bypassButton;
    RotaryKnob wetDryKnob;

    // Parameter knobs (up to 4 main params shown)
    static constexpr int MAX_VISIBLE_PARAMS = 4;
    std::array<std::unique_ptr<RotaryKnob>, MAX_VISIBLE_PARAMS> paramKnobs;

    void updateFromEffect();
    void setupParamKnob(int index, const juce::String& paramId, const EffectParameter& param);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectSlot)
};
