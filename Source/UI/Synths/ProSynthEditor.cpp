#include "ProSynthEditor.h"

ProSynthEditor::ProSynthEditor(ProSynth& s)
    : SynthEditorBase(), synth(s)
{
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "master_volume", "Volume", "", "Master output volume");

    //==========================================================================
    // CARD PANELS (no headers, compact padding for dense layout)
    //==========================================================================
    osc1Card.setShowHeader(false);
    osc1Card.setPadding(4);
    addAndMakeVisible(osc1Card);

    osc2Card.setShowHeader(false);
    osc2Card.setPadding(4);
    addAndMakeVisible(osc2Card);

    osc3Card.setShowHeader(false);
    osc3Card.setPadding(4);
    addAndMakeVisible(osc3Card);

    subNoiseCard.setShowHeader(false);
    subNoiseCard.setPadding(4);
    addAndMakeVisible(subNoiseCard);

    filterCard.setShowHeader(false);
    filterCard.setPadding(4);
    addAndMakeVisible(filterCard);

    filterEnvCard.setShowHeader(false);
    filterEnvCard.setPadding(4);
    addAndMakeVisible(filterEnvCard);

    ampEnvCard.setShowHeader(false);
    ampEnvCard.setPadding(4);
    addAndMakeVisible(ampEnvCard);

    unisonCard.setShowHeader(false);
    unisonCard.setPadding(4);
    addAndMakeVisible(unisonCard);

    //==========================================================================
    // OSCILLATOR 1
    //==========================================================================
    setupComboBox(osc1Mode, "osc1_mode");
    osc1Card.addAndMakeVisible(osc1Mode);
    setupComboBox(osc1Wave, "osc1_wave");
    osc1Card.addAndMakeVisible(osc1Wave);
    setupKnob(osc1Level, "osc1_level", "Lvl", "", "Oscillator 1 volume level");
    osc1Card.addAndMakeVisible(osc1Level);
    setupKnob(osc1Octave, "osc1_octave", "Oct", "", "Octave shift (-2 to +2)");
    osc1Card.addAndMakeVisible(osc1Octave);
    setupKnob(osc1Fine, "osc1_fine", "Fine", " ct", "Fine tuning in cents");
    osc1Card.addAndMakeVisible(osc1Fine);

    //==========================================================================
    // OSCILLATOR 2
    //==========================================================================
    setupComboBox(osc2Mode, "osc2_mode");
    osc2Card.addAndMakeVisible(osc2Mode);
    setupComboBox(osc2Wave, "osc2_wave");
    osc2Card.addAndMakeVisible(osc2Wave);
    setupKnob(osc2Level, "osc2_level", "Lvl", "", "Oscillator 2 volume level");
    osc2Card.addAndMakeVisible(osc2Level);
    setupKnob(osc2Octave, "osc2_octave", "Oct", "", "Octave shift (-2 to +2)");
    osc2Card.addAndMakeVisible(osc2Octave);
    setupKnob(osc2Fine, "osc2_fine", "Fine", " ct", "Fine tuning in cents");
    osc2Card.addAndMakeVisible(osc2Fine);

    //==========================================================================
    // OSCILLATOR 3
    //==========================================================================
    setupComboBox(osc3Mode, "osc3_mode");
    osc3Card.addAndMakeVisible(osc3Mode);
    setupComboBox(osc3Wave, "osc3_wave");
    osc3Card.addAndMakeVisible(osc3Wave);
    setupKnob(osc3Level, "osc3_level", "Lvl", "", "Oscillator 3 volume level");
    osc3Card.addAndMakeVisible(osc3Level);
    setupKnob(osc3Octave, "osc3_octave", "Oct", "", "Octave shift (-2 to +2)");
    osc3Card.addAndMakeVisible(osc3Octave);
    setupKnob(osc3Fine, "osc3_fine", "Fine", " ct", "Fine tuning in cents");
    osc3Card.addAndMakeVisible(osc3Fine);

    //==========================================================================
    // SUB + NOISE
    //==========================================================================
    setupComboBox(subWave, "sub_wave");
    subNoiseCard.addAndMakeVisible(subWave);
    setupKnob(subLevel, "sub_level", "Sub", "", "Sub oscillator level - adds bass");
    subNoiseCard.addAndMakeVisible(subLevel);
    setupComboBox(noiseType, "noise_type");
    subNoiseCard.addAndMakeVisible(noiseType);
    setupKnob(noiseLevel, "noise_level", "Noise", "", "Noise level - adds texture/air");
    subNoiseCard.addAndMakeVisible(noiseLevel);

    //==========================================================================
    // FILTER
    //==========================================================================
    setupComboBox(filter1Model, "filter1_model");
    filterCard.addAndMakeVisible(filter1Model);
    setupComboBox(filter1Type, "filter_type");
    filterCard.addAndMakeVisible(filter1Type);
    setupKnob(filterCutoff, "filter_cutoff", "Cut", " Hz", "Filter cutoff frequency");
    filterCard.addAndMakeVisible(filterCutoff);
    setupKnob(filterResonance, "filter_resonance", "Res", "", "Resonance - emphasis at cutoff");
    filterCard.addAndMakeVisible(filterResonance);
    setupKnob(filterDrive, "filter_drive", "Drive", "", "Filter drive/saturation");
    filterCard.addAndMakeVisible(filterDrive);

    //==========================================================================
    // FILTER ENVELOPE
    //==========================================================================
    setupKnob(filterEnvAttack, "filter_env_attack", "A", " s", "Filter attack time");
    filterEnvCard.addAndMakeVisible(filterEnvAttack);
    setupKnob(filterEnvDecay, "filter_env_decay", "D", " s", "Filter decay time");
    filterEnvCard.addAndMakeVisible(filterEnvDecay);
    setupKnob(filterEnvSustain, "filter_env_sustain", "S", "", "Filter sustain level");
    filterEnvCard.addAndMakeVisible(filterEnvSustain);
    setupKnob(filterEnvRelease, "filter_env_release", "R", " s", "Filter release time");
    filterEnvCard.addAndMakeVisible(filterEnvRelease);
    setupKnob(filterEnvAmount, "filter_env_amount", "Amt", " Hz", "Envelope to cutoff amount");
    filterEnvCard.addAndMakeVisible(filterEnvAmount);

    //==========================================================================
    // AMP ENVELOPE
    //==========================================================================
    setupKnob(ampAttack, "amp_attack", "A", " s", "Attack - time to full volume");
    ampEnvCard.addAndMakeVisible(ampAttack);
    setupKnob(ampDecay, "amp_decay", "D", " s", "Decay - time to sustain");
    ampEnvCard.addAndMakeVisible(ampDecay);
    setupKnob(ampSustain, "amp_sustain", "S", "", "Sustain level while held");
    ampEnvCard.addAndMakeVisible(ampSustain);
    setupKnob(ampRelease, "amp_release", "R", " s", "Release - fade after key up");
    ampEnvCard.addAndMakeVisible(ampRelease);

    //==========================================================================
    // UNISON
    //==========================================================================
    setupKnob(unisonVoices, "unison_voices", "Voices", "", "Number of unison voices (1-8)");
    unisonCard.addAndMakeVisible(unisonVoices);
    setupKnob(unisonDetune, "unison_detune", "Detune", " ct", "Unison detune spread");
    unisonCard.addAndMakeVisible(unisonDetune);

    refreshFromSynth();
}

ProSynthEditor::~ProSynthEditor()
{
    presetSelector.removeListener(this);
    osc1Mode.removeListener(this);
    osc1Wave.removeListener(this);
    osc2Mode.removeListener(this);
    osc2Wave.removeListener(this);
    osc3Mode.removeListener(this);
    osc3Wave.removeListener(this);
    subWave.removeListener(this);
    noiseType.removeListener(this);
    filter1Model.removeListener(this);
    filter1Type.removeListener(this);
}

void ProSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
                                const juce::String& label, const juce::String& suffix,
                                const juce::String& description)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);

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

void ProSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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

void ProSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &osc1Mode) synth.setParameterEnum("osc1_mode", index);
    else if (box == &osc1Wave) synth.setParameterEnum("osc1_wave", index);
    else if (box == &osc2Mode) synth.setParameterEnum("osc2_mode", index);
    else if (box == &osc2Wave) synth.setParameterEnum("osc2_wave", index);
    else if (box == &osc3Mode) synth.setParameterEnum("osc3_mode", index);
    else if (box == &osc3Wave) synth.setParameterEnum("osc3_wave", index);
    else if (box == &subWave) synth.setParameterEnum("sub_wave", index);
    else if (box == &noiseType) synth.setParameterEnum("noise_type", index);
    else if (box == &filter1Model) synth.setParameterEnum("filter1_model", index);
    else if (box == &filter1Type) synth.setParameterEnum("filter_type", index);
}

void ProSynthEditor::populatePresets()
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
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
    else if (!presets.empty())
    {
        synth.loadPreset(0);
        presetSelector.setSelectedId(1, juce::dontSendNotification);
    }
}

void ProSynthEditor::refreshFromSynth()
{
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    auto refreshCombo = [this](juce::ComboBox& box, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            box.setSelectedId(param->enumIndex + 1, juce::dontSendNotification);
    };

    refreshKnob(masterVolume, "master_volume");

    // OSC 1-3
    refreshCombo(osc1Mode, "osc1_mode"); refreshCombo(osc1Wave, "osc1_wave");
    refreshKnob(osc1Level, "osc1_level"); refreshKnob(osc1Octave, "osc1_octave"); refreshKnob(osc1Fine, "osc1_fine");
    refreshCombo(osc2Mode, "osc2_mode"); refreshCombo(osc2Wave, "osc2_wave");
    refreshKnob(osc2Level, "osc2_level"); refreshKnob(osc2Octave, "osc2_octave"); refreshKnob(osc2Fine, "osc2_fine");
    refreshCombo(osc3Mode, "osc3_mode"); refreshCombo(osc3Wave, "osc3_wave");
    refreshKnob(osc3Level, "osc3_level"); refreshKnob(osc3Octave, "osc3_octave"); refreshKnob(osc3Fine, "osc3_fine");

    // Sub + Noise
    refreshCombo(subWave, "sub_wave"); refreshKnob(subLevel, "sub_level");
    refreshCombo(noiseType, "noise_type"); refreshKnob(noiseLevel, "noise_level");

    // Filter
    refreshCombo(filter1Model, "filter1_model"); refreshCombo(filter1Type, "filter_type");
    refreshKnob(filterCutoff, "filter_cutoff"); refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterDrive, "filter_drive");

    // Envelopes
    refreshKnob(filterEnvAttack, "filter_env_attack"); refreshKnob(filterEnvDecay, "filter_env_decay");
    refreshKnob(filterEnvSustain, "filter_env_sustain"); refreshKnob(filterEnvRelease, "filter_env_release");
    refreshKnob(filterEnvAmount, "filter_env_amount");
    refreshKnob(ampAttack, "amp_attack"); refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain"); refreshKnob(ampRelease, "amp_release");

    // Unison
    refreshKnob(unisonVoices, "unison_voices"); refreshKnob(unisonDetune, "unison_detune");

    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void ProSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int cardGap = 6;
    const int knobHeight = RotaryKnob::TOTAL_HEIGHT;
    const int comboHeight = 22;
    const int smallKnob = 44;  // Smaller knobs for cramped spaces

    // Two rows - give more space to top row which has more content
    int availableHeight = area.getHeight();
    int topRowHeight = availableHeight * 60 / 100;  // 60% to top row

    auto topRow = area.removeFromTop(topRowHeight);
    area.removeFromTop(cardGap);
    auto bottomRow = area;

    //==========================================================================
    // TOP ROW: OSC1 | OSC2 | OSC3 | SUB+NOISE | FILTER
    //==========================================================================
    {
        int totalWidth = topRow.getWidth();
        int oscWidth = (totalWidth - cardGap * 4) / 5;

        // Helper to layout an OSC card with compact layout
        auto layoutOscCard = [&](CardPanel& card, juce::ComboBox& mode, juce::ComboBox& wave,
                                  RotaryKnob& level, RotaryKnob& octave, RotaryKnob& fine,
                                  juce::Rectangle<int> bounds)
        {
            card.setBounds(bounds);
            auto content = card.getContentArea();

            // Two dropdowns stacked vertically (more compact)
            mode.setBounds(content.removeFromTop(comboHeight));
            content.removeFromTop(2);
            wave.setBounds(content.removeFromTop(comboHeight));
            content.removeFromTop(6);

            // 3 knobs: Level, Oct, Fine - use smaller size
            int knobSpacing = content.getWidth() / 3;
            level.setBounds(content.removeFromLeft(knobSpacing).withSizeKeepingCentre(smallKnob, knobHeight));
            octave.setBounds(content.removeFromLeft(knobSpacing).withSizeKeepingCentre(smallKnob, knobHeight));
            fine.setBounds(content.withSizeKeepingCentre(smallKnob, knobHeight));
        };

        layoutOscCard(osc1Card, osc1Mode, osc1Wave, osc1Level, osc1Octave, osc1Fine,
                      topRow.removeFromLeft(oscWidth));
        topRow.removeFromLeft(cardGap);

        layoutOscCard(osc2Card, osc2Mode, osc2Wave, osc2Level, osc2Octave, osc2Fine,
                      topRow.removeFromLeft(oscWidth));
        topRow.removeFromLeft(cardGap);

        layoutOscCard(osc3Card, osc3Mode, osc3Wave, osc3Level, osc3Octave, osc3Fine,
                      topRow.removeFromLeft(oscWidth));
        topRow.removeFromLeft(cardGap);

        // SUB + NOISE card
        auto subBounds = topRow.removeFromLeft(oscWidth);
        subNoiseCard.setBounds(subBounds);
        auto subContent = subNoiseCard.getContentArea();
        subWave.setBounds(subContent.removeFromTop(comboHeight));
        subContent.removeFromTop(2);
        noiseType.setBounds(subContent.removeFromTop(comboHeight));
        subContent.removeFromTop(6);
        int knobSpacing = subContent.getWidth() / 2;
        subLevel.setBounds(subContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(smallKnob, knobHeight));
        noiseLevel.setBounds(subContent.withSizeKeepingCentre(smallKnob, knobHeight));

        topRow.removeFromLeft(cardGap);

        // FILTER card
        auto filterBounds = topRow;
        filterCard.setBounds(filterBounds);
        auto filterContent = filterCard.getContentArea();
        filter1Model.setBounds(filterContent.removeFromTop(comboHeight));
        filterContent.removeFromTop(2);
        filter1Type.setBounds(filterContent.removeFromTop(comboHeight));
        filterContent.removeFromTop(6);
        knobSpacing = filterContent.getWidth() / 3;
        filterCutoff.setBounds(filterContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(smallKnob, knobHeight));
        filterResonance.setBounds(filterContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(smallKnob, knobHeight));
        filterDrive.setBounds(filterContent.withSizeKeepingCentre(smallKnob, knobHeight));
    }

    //==========================================================================
    // BOTTOM ROW: FILTER ENV | AMP ENV | UNISON
    //==========================================================================
    {
        int totalWidth = bottomRow.getWidth();
        int envWidth = (totalWidth - cardGap * 2) * 40 / 100;
        int unisonWidth = totalWidth - envWidth * 2 - cardGap * 2;

        // Filter Envelope card (5 knobs: ADSR + Amount)
        auto filterEnvBounds = bottomRow.removeFromLeft(envWidth);
        filterEnvCard.setBounds(filterEnvBounds);
        auto fenvContent = filterEnvCard.getContentArea();
        int knobSpacing = fenvContent.getWidth() / 5;
        filterEnvAttack.setBounds(fenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterEnvDecay.setBounds(fenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterEnvSustain.setBounds(fenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterEnvRelease.setBounds(fenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterEnvAmount.setBounds(fenvContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        bottomRow.removeFromLeft(cardGap);

        // Amp Envelope card (4 knobs: ADSR)
        auto ampEnvBounds = bottomRow.removeFromLeft(envWidth);
        ampEnvCard.setBounds(ampEnvBounds);
        auto aenvContent = ampEnvCard.getContentArea();
        knobSpacing = aenvContent.getWidth() / 4;
        ampAttack.setBounds(aenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampDecay.setBounds(aenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampSustain.setBounds(aenvContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampRelease.setBounds(aenvContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        bottomRow.removeFromLeft(cardGap);

        // Unison card (2 knobs)
        auto unisonBounds = bottomRow;
        unisonCard.setBounds(unisonBounds);
        auto uniContent = unisonCard.getContentArea();
        knobSpacing = uniContent.getWidth() / 2;
        unisonVoices.setBounds(uniContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        unisonDetune.setBounds(uniContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    }
}

void ProSynthEditor::drawDividers(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // No dividers needed - CardPanels handle their own styling
}
