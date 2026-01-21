#include "FMSynthEditor.h"

FMSynthEditor::FMSynthEditor(FMSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Setup preset selector and master volume (from base class)
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "volume", "Volume", "", "Master output volume");

    //==========================================================================
    // CARD PANELS (Saturn design)
    //==========================================================================

    // Add all card panels (no headers, compact padding)
    algorithmCard.setShowHeader(false);
    algorithmCard.setPadding(6);
    addAndMakeVisible(algorithmCard);

    carrierCard.setShowHeader(false);
    carrierCard.setPadding(6);
    addAndMakeVisible(carrierCard);

    mod1Card.setShowHeader(false);
    mod1Card.setPadding(6);
    addAndMakeVisible(mod1Card);

    mod2Card.setShowHeader(false);
    mod2Card.setPadding(6);
    addAndMakeVisible(mod2Card);

    feedbackCard.setShowHeader(false);
    feedbackCard.setPadding(6);
    addAndMakeVisible(feedbackCard);

    envelopesCard.setShowHeader(false);
    envelopesCard.setPadding(6);
    addAndMakeVisible(envelopesCard);

    //==========================================================================
    // ALGORITHM CARD
    //==========================================================================
    setupComboBox(algorithmSelector, "algorithm");
    algorithmCard.addAndMakeVisible(algorithmSelector);

    //==========================================================================
    // CARRIER CARD
    //==========================================================================
    setupKnob(carrierRatio, "carrier_ratio", "Ratio", "", "Carrier frequency ratio (multiplier of base pitch)");
    carrierCard.addAndMakeVisible(carrierRatio);

    //==========================================================================
    // MODULATOR 1 CARD
    //==========================================================================
    setupKnob(mod1Ratio, "mod1_ratio", "Ratio", "", "Modulator 1 frequency ratio");
    setupKnob(mod1Index, "mod1_index", "Index", "", "Modulator 1 depth - higher values add more harmonics");
    mod1Card.addAndMakeVisible(mod1Ratio);
    mod1Card.addAndMakeVisible(mod1Index);

    //==========================================================================
    // MODULATOR 2 CARD
    //==========================================================================
    setupKnob(mod2Ratio, "mod2_ratio", "Ratio", "", "Modulator 2 frequency ratio");
    setupKnob(mod2Index, "mod2_index", "Index", "", "Modulator 2 depth - higher values add more harmonics");
    mod2Card.addAndMakeVisible(mod2Ratio);
    mod2Card.addAndMakeVisible(mod2Index);

    //==========================================================================
    // FEEDBACK CARD
    //==========================================================================
    setupKnob(feedbackKnob, "feedback", "Amount", "", "Operator self-modulation - adds metallic/harsh tones");
    feedbackCard.addAndMakeVisible(feedbackKnob);

    //==========================================================================
    // ENVELOPES CARD
    //==========================================================================

    // Envelope sub-labels
    createSectionLabel(ampEnvLabel, "AMP");
    createSectionLabel(mod1EnvLabel, "MOD 1");
    createSectionLabel(mod2EnvLabel, "MOD 2");
    envelopesCard.addAndMakeVisible(ampEnvLabel);
    envelopesCard.addAndMakeVisible(mod1EnvLabel);
    envelopesCard.addAndMakeVisible(mod2EnvLabel);

    // Amp Envelope knobs
    setupKnob(ampAttack, "amp_attack", "A", "s", "Attack - time to reach full volume");
    setupKnob(ampDecay, "amp_decay", "D", "s", "Decay - time to fall to sustain level");
    setupKnob(ampSustain, "amp_sustain", "S", "", "Sustain - volume while key is held");
    setupKnob(ampRelease, "amp_release", "R", "s", "Release - time to fade after key release");
    envelopesCard.addAndMakeVisible(ampAttack);
    envelopesCard.addAndMakeVisible(ampDecay);
    envelopesCard.addAndMakeVisible(ampSustain);
    envelopesCard.addAndMakeVisible(ampRelease);

    // Mod 1 Envelope knobs
    setupKnob(mod1Attack, "mod1_attack", "A", "s", "Mod 1 Attack - modulation intensity ramp-up time");
    setupKnob(mod1Decay, "mod1_decay", "D", "s", "Mod 1 Decay - time to sustain level");
    setupKnob(mod1Sustain, "mod1_sustain", "S", "", "Mod 1 Sustain - modulation level while held");
    setupKnob(mod1Release, "mod1_release", "R", "s", "Mod 1 Release - modulation fade time");
    envelopesCard.addAndMakeVisible(mod1Attack);
    envelopesCard.addAndMakeVisible(mod1Decay);
    envelopesCard.addAndMakeVisible(mod1Sustain);
    envelopesCard.addAndMakeVisible(mod1Release);

    // Mod 2 Envelope knobs
    setupKnob(mod2Attack, "mod2_attack", "A", "s", "Mod 2 Attack - modulation intensity ramp-up time");
    setupKnob(mod2Decay, "mod2_decay", "D", "s", "Mod 2 Decay - time to sustain level");
    setupKnob(mod2Sustain, "mod2_sustain", "S", "", "Mod 2 Sustain - modulation level while held");
    setupKnob(mod2Release, "mod2_release", "R", "s", "Mod 2 Release - modulation fade time");
    envelopesCard.addAndMakeVisible(mod2Attack);
    envelopesCard.addAndMakeVisible(mod2Decay);
    envelopesCard.addAndMakeVisible(mod2Sustain);
    envelopesCard.addAndMakeVisible(mod2Release);

    // Initial refresh
    refreshFromSynth();
}

FMSynthEditor::~FMSynthEditor()
{
    presetSelector.removeListener(this);
    algorithmSelector.removeListener(this);
}

void FMSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
                               const juce::String& label, const juce::String& suffix,
                               const juce::String& description)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);

    // Set descriptive tooltip if provided
    if (description.isNotEmpty())
        knob.setTooltipText(description);

    if (auto* param = synth.getParameterInfo(paramId))
    {
        knob.setRange(param->minValue, param->maxValue, param->step);
        knob.setDefaultValue(param->defaultValue);
        knob.setValue(param->value, juce::dontSendNotification);
    }

    knob.onValueChange = [this, paramId](float value)
    {
        synth.setParameter(paramId, value);
    };

    addAndMakeVisible(knob);
}

void FMSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
{
    if (auto* param = synth.getParameterInfo(paramId))
    {
        int id = 1;
        for (const auto& option : param->options)
        {
            box.addItem(option, id++);
        }
        box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    }

    box.addListener(this);
    addAndMakeVisible(box);
}

void FMSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &algorithmSelector)
    {
        synth.setParameterEnum("algorithm", index);
    }
}

void FMSynthEditor::populatePresets()
{
    presetSelector.clear();

    auto presets = synth.getPresets();
    int id = 1;
    for (const auto& preset : presets)
    {
        presetSelector.addItem(preset.name, id++);
    }

    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
    {
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
    }
    else if (!presets.empty())
    {
        synth.loadPreset(0);
        presetSelector.setSelectedId(1, juce::dontSendNotification);
    }
}

void FMSynthEditor::refreshFromSynth()
{
    // Helper lambda to refresh a knob
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    // Helper lambda to refresh a combo box
    auto refreshCombo = [this](juce::ComboBox& box, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    };

    // Master
    refreshKnob(masterVolume, "volume");

    // Algorithm
    refreshCombo(algorithmSelector, "algorithm");

    // Carrier
    refreshKnob(carrierRatio, "carrier_ratio");

    // Modulator 1
    refreshKnob(mod1Ratio, "mod1_ratio");
    refreshKnob(mod1Index, "mod1_index");

    // Modulator 2
    refreshKnob(mod2Ratio, "mod2_ratio");
    refreshKnob(mod2Index, "mod2_index");

    // Feedback
    refreshKnob(feedbackKnob, "feedback");

    // Amp Envelope
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // Mod 1 Envelope
    refreshKnob(mod1Attack, "mod1_attack");
    refreshKnob(mod1Decay, "mod1_decay");
    refreshKnob(mod1Sustain, "mod1_sustain");
    refreshKnob(mod1Release, "mod1_release");

    // Mod 2 Envelope
    refreshKnob(mod2Attack, "mod2_attack");
    refreshKnob(mod2Decay, "mod2_decay");
    refreshKnob(mod2Sustain, "mod2_sustain");
    refreshKnob(mod2Release, "mod2_release");

    // Update preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void FMSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int cardGap = 6;
    const int knobHeight = RotaryKnob::TOTAL_HEIGHT;  // 80px with value display
    const int comboHeight = 28;
    const int compactKnobHeight = 70;  // Slightly smaller for envelopes

    // Two rows: Top (operators), Bottom (envelopes)
    int availableHeight = area.getHeight();
    int rowHeight = (availableHeight - cardGap) / 2;

    auto topRow = area.removeFromTop(rowHeight);
    area.removeFromTop(cardGap);
    auto bottomRow = area;

    //==========================================================================
    // TOP ROW: Algorithm, Carrier, Mod1, Mod2, Feedback - all in one row
    //==========================================================================
    {
        int totalWidth = topRow.getWidth();
        // Give more space to modulator cards since they have 2 knobs
        int smallCardWidth = (totalWidth - cardGap * 4) / 6;  // Algorithm, Carrier, Feedback
        int largeCardWidth = smallCardWidth * 3 / 2;          // Mod1, Mod2

        // Algorithm card
        auto algBounds = topRow.removeFromLeft(smallCardWidth);
        algorithmCard.setBounds(algBounds);
        auto algContent = algorithmCard.getContentArea();
        algorithmSelector.setBounds(algContent.withTrimmedTop((algContent.getHeight() - comboHeight) / 2)
                                              .removeFromTop(comboHeight));

        topRow.removeFromLeft(cardGap);

        // Carrier card
        auto carrierBounds = topRow.removeFromLeft(smallCardWidth);
        carrierCard.setBounds(carrierBounds);
        auto carrierContent = carrierCard.getContentArea();
        carrierRatio.setBounds(carrierContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        topRow.removeFromLeft(cardGap);

        // Mod 1 card
        auto mod1Bounds = topRow.removeFromLeft(largeCardWidth);
        mod1Card.setBounds(mod1Bounds);
        auto mod1Content = mod1Card.getContentArea();
        int knobSpacing = mod1Content.getWidth() / 2;
        mod1Ratio.setBounds(mod1Content.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        mod1Index.setBounds(mod1Content.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        topRow.removeFromLeft(cardGap);

        // Mod 2 card
        auto mod2Bounds = topRow.removeFromLeft(largeCardWidth);
        mod2Card.setBounds(mod2Bounds);
        auto mod2Content = mod2Card.getContentArea();
        knobSpacing = mod2Content.getWidth() / 2;
        mod2Ratio.setBounds(mod2Content.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        mod2Index.setBounds(mod2Content.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        topRow.removeFromLeft(cardGap);

        // Feedback card
        auto feedbackBounds = topRow;
        feedbackCard.setBounds(feedbackBounds);
        auto feedbackContent = feedbackCard.getContentArea();
        feedbackKnob.setBounds(feedbackContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    }

    //==========================================================================
    // BOTTOM ROW: Envelopes card spanning full width
    //==========================================================================
    {
        envelopesCard.setBounds(bottomRow);
        auto envContent = envelopesCard.getContentArea();

        // Layout 3 envelope groups horizontally, all in one row
        int envGroupWidth = envContent.getWidth() / 3;
        const int labelHeight = 14;
        const int labelGap = 2;

        auto layoutEnvGroup = [&](juce::Label& label, RotaryKnob& a, RotaryKnob& d,
                                   RotaryKnob& s, RotaryKnob& r, juce::Rectangle<int> bounds)
        {
            // Label at top
            label.setBounds(bounds.removeFromTop(labelHeight));
            bounds.removeFromTop(labelGap);

            // 4 knobs in a row
            int knobWidth = bounds.getWidth() / 4;
            a.setBounds(bounds.removeFromLeft(knobWidth).withSizeKeepingCentre(KNOB_SIZE, compactKnobHeight));
            d.setBounds(bounds.removeFromLeft(knobWidth).withSizeKeepingCentre(KNOB_SIZE, compactKnobHeight));
            s.setBounds(bounds.removeFromLeft(knobWidth).withSizeKeepingCentre(KNOB_SIZE, compactKnobHeight));
            r.setBounds(bounds.withSizeKeepingCentre(KNOB_SIZE, compactKnobHeight));
        };

        layoutEnvGroup(ampEnvLabel, ampAttack, ampDecay, ampSustain, ampRelease,
                       envContent.removeFromLeft(envGroupWidth));
        layoutEnvGroup(mod1EnvLabel, mod1Attack, mod1Decay, mod1Sustain, mod1Release,
                       envContent.removeFromLeft(envGroupWidth));
        layoutEnvGroup(mod2EnvLabel, mod2Attack, mod2Decay, mod2Sustain, mod2Release,
                       envContent);
    }
}

void FMSynthEditor::drawDividers(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // No dividers needed - CardPanels handle their own styling
}
