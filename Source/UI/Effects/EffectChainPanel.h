#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "EffectSlot.h"
#include "../../Audio/Effects/EffectChain.h"
#include "../../Audio/Effects/ReverbEffect.h"
#include "../../Audio/Effects/DelayEffect.h"
#include "../../Audio/Effects/ChorusEffect.h"
#include "../../Audio/Effects/DistortionEffect.h"
#include "../../Audio/Effects/CompressorEffect.h"
#include "../../Audio/Effects/EQEffect.h"
#include "../../Audio/Effects/PhaserEffect.h"
#include "../../Audio/Effects/FlangerEffect.h"
#include "../../Audio/Effects/TremoloEffect.h"
#include "../../Audio/Effects/BitcrusherEffect.h"
#include "../../Audio/Effects/LimiterEffect.h"
#include "../../Audio/Effects/GateEffect.h"
#include "../../Audio/Effects/FilterEffect.h"
#include "../../Audio/Effects/AmpSimulatorEffect.h"
#include "../../Audio/Effects/CabinetEffect.h"
#include "../../Audio/Effects/SidechainCompressorEffect.h"
#include "../LookAndFeel.h"

/**
 * EffectChainPanel - Full UI panel for the effect chain
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │ EFFECT CHAIN                                [Add Effect ▼] [Clear] │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐                    │
 * │ │ Slot 1  │ │ Slot 2  │ │ Slot 3  │ │ Slot 4  │  (scrollable)      │
 * │ │ Reverb  │ │ Delay   │ │ Empty   │ │ Empty   │                    │
 * │ └─────────┘ └─────────┘ └─────────┘ └─────────┘                    │
 * └─────────────────────────────────────────────────────────────────────┘
 */
class EffectChainPanel : public juce::Component,
                          public juce::DragAndDropContainer
{
public:
    EffectChainPanel(EffectChain& chain);
    ~EffectChainPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshFromChain();

private:
    EffectChain& effectChain;

    // UI Components
    juce::Label titleLabel;
    juce::ComboBox addEffectSelector;
    juce::TextButton clearButton;

    // Effect slots (8 max)
    std::array<std::unique_ptr<EffectSlot>, EffectChain::MAX_EFFECTS> slots;

    // Scrollable viewport for slots
    juce::Viewport viewport;
    juce::Component slotsContainer;

    // Helpers
    void onAddEffectSelected();
    void onRemoveEffect(int slot);
    void onBypassToggled(int slot, bool bypassed);
    void onEffectDropped(int fromSlot, int toSlot);
    void populateEffectSelector();

    std::unique_ptr<EffectBase> createEffect(const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectChainPanel)
};
