#include "AnalogSynthEditor.h"

AnalogSynthEditor::AnalogSynthEditor(AnalogSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Setup preset selector and master volume (from base class)
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "master_volume", "Volume", "", "Master output volume");

    //==========================================================================
    // CARD PANELS (Saturn design - no headers for compact layout)
    //==========================================================================
    osc1Card.setShowHeader(false);
    osc1Card.setPadding(6);
    addAndMakeVisible(osc1Card);

    osc2Card.setShowHeader(false);
    osc2Card.setPadding(6);
    addAndMakeVisible(osc2Card);

    filterCard.setShowHeader(false);
    filterCard.setPadding(6);
    addAndMakeVisible(filterCard);

    ampEnvCard.setShowHeader(false);
    ampEnvCard.setPadding(6);
    addAndMakeVisible(ampEnvCard);

    filterEnvCard.setShowHeader(false);
    filterEnvCard.setPadding(6);
    addAndMakeVisible(filterEnvCard);

    //==========================================================================
    // OSCILLATOR 1
    //==========================================================================
    setupWaveSelector(osc1Wave, "osc1_wave");
    osc1Card.addAndMakeVisible(osc1Wave);
    setupKnob(osc1Octave, "osc1_octave", "Semi", "", "Oscillator 1 pitch offset in semitones");
    osc1Card.addAndMakeVisible(osc1Octave);
    setupKnob(osc1Detune, "osc1_detune", "Fine", " ct", "Fine tuning in cents for subtle detuning");
    osc1Card.addAndMakeVisible(osc1Detune);

    //==========================================================================
    // OSCILLATOR 2
    //==========================================================================
    setupWaveSelector(osc2Wave, "osc2_wave");
    osc2Card.addAndMakeVisible(osc2Wave);
    setupKnob(osc2Octave, "osc2_octave", "Semi", "", "Oscillator 2 pitch offset in semitones");
    osc2Card.addAndMakeVisible(osc2Octave);
    setupKnob(osc2Detune, "osc2_detune", "Detune", " ct", "Fine tuning - offset from OSC1 for fat sounds");
    osc2Card.addAndMakeVisible(osc2Detune);

    //==========================================================================
    // FILTER
    //==========================================================================
    setupComboBox(filterType, "filter_type");
    filterCard.addAndMakeVisible(filterType);
    setupKnob(filterCutoff, "filter_cutoff", "Cut", " Hz", "Filter cutoff frequency - lower = darker sound");
    filterCard.addAndMakeVisible(filterCutoff);
    setupKnob(filterResonance, "filter_resonance", "Res", "", "Resonance - boost at cutoff frequency");
    filterCard.addAndMakeVisible(filterResonance);
    setupKnob(filterEnvAmount, "filter_env_amount", "Env", " Hz", "How much filter envelope affects cutoff");
    filterCard.addAndMakeVisible(filterEnvAmount);

    //==========================================================================
    // AMP ENVELOPE
    //==========================================================================
    setupKnob(ampAttack, "amp_attack", "A", " s", "Attack - time to reach full volume");
    ampEnvCard.addAndMakeVisible(ampAttack);
    setupKnob(ampDecay, "amp_decay", "D", " s", "Decay - time to fall to sustain level");
    ampEnvCard.addAndMakeVisible(ampDecay);
    setupKnob(ampSustain, "amp_sustain", "S", "", "Sustain - volume while key is held");
    ampEnvCard.addAndMakeVisible(ampSustain);
    setupKnob(ampRelease, "amp_release", "R", " s", "Release - time to fade after key release");
    ampEnvCard.addAndMakeVisible(ampRelease);

    //==========================================================================
    // FILTER ENVELOPE
    //==========================================================================
    setupKnob(filterAttack, "filter_attack", "A", " s", "Filter attack - cutoff sweep time");
    filterEnvCard.addAndMakeVisible(filterAttack);
    setupKnob(filterDecay, "filter_decay", "D", " s", "Filter decay - time to sustain");
    filterEnvCard.addAndMakeVisible(filterDecay);
    setupKnob(filterSustain, "filter_sustain", "S", "", "Filter sustain level");
    filterEnvCard.addAndMakeVisible(filterSustain);
    setupKnob(filterRelease, "filter_release", "R", " s", "Filter release time");
    filterEnvCard.addAndMakeVisible(filterRelease);

    // Initial refresh
    refreshFromSynth();
}

AnalogSynthEditor::~AnalogSynthEditor()
{
    filterType.removeListener(this);
}

void AnalogSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void AnalogSynthEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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

void AnalogSynthEditor::setupWaveSelector(WaveSelector& selector, const juce::String& paramId)
{
    if (auto* param = synth.getParameterInfo(paramId))
    {
        selector.setSelectedIndex(param->enumIndex, juce::dontSendNotification);
    }

    selector.onSelectionChanged = [this, paramId](int index)
    {
        synth.setParameterEnum(paramId, index);
    };

    addAndMakeVisible(selector);
}

void AnalogSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
    }
    else if (box == &filterType)
    {
        synth.setParameterEnum("filter_type", index);
    }
}

void AnalogSynthEditor::populatePresets()
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

void AnalogSynthEditor::refreshFromSynth()
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

    auto refreshWave = [this](WaveSelector& selector, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            selector.setSelectedIndex(param->enumIndex, juce::dontSendNotification);
    };

    // Master
    refreshKnob(masterVolume, "master_volume");

    // OSC 1
    refreshWave(osc1Wave, "osc1_wave");
    refreshKnob(osc1Octave, "osc1_octave");
    refreshKnob(osc1Detune, "osc1_detune");

    // OSC 2
    refreshWave(osc2Wave, "osc2_wave");
    refreshKnob(osc2Octave, "osc2_octave");
    refreshKnob(osc2Detune, "osc2_detune");

    // FILTER
    refreshCombo(filterType, "filter_type");
    refreshKnob(filterCutoff, "filter_cutoff");
    refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterEnvAmount, "filter_env_amount");

    // AMP ENV
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // FILTER ENV
    refreshKnob(filterAttack, "filter_attack");
    refreshKnob(filterDecay, "filter_decay");
    refreshKnob(filterSustain, "filter_sustain");
    refreshKnob(filterRelease, "filter_release");

    // Update preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void AnalogSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int cardGap = 6;
    const int knobHeight = RotaryKnob::TOTAL_HEIGHT;
    const int selectorHeight = 28;

    // Two rows: Top row (OSC1, OSC2, Filter), Bottom row (Amp Env, Filter Env)
    int availableHeight = area.getHeight();
    int rowHeight = (availableHeight - cardGap) / 2;  // Split evenly

    auto topRow = area.removeFromTop(rowHeight);
    area.removeFromTop(cardGap);
    auto bottomRow = area;

    //==========================================================================
    // TOP ROW: OSC1, OSC2, Filter
    //==========================================================================
    {
        int totalWidth = topRow.getWidth();
        int oscWidth = (totalWidth - cardGap * 2) / 4;      // Each OSC gets 1/4
        int filterWidth = totalWidth - oscWidth * 2 - cardGap * 2;  // Filter gets rest

        // OSC 1 card
        auto osc1Bounds = topRow.removeFromLeft(oscWidth);
        osc1Card.setBounds(osc1Bounds);
        auto osc1Content = osc1Card.getContentArea();
        osc1Wave.setBounds(osc1Content.removeFromTop(selectorHeight));
        osc1Content.removeFromTop(4);
        int knobSpacing = osc1Content.getWidth() / 2;
        osc1Octave.setBounds(osc1Content.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        osc1Detune.setBounds(osc1Content.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        topRow.removeFromLeft(cardGap);

        // OSC 2 card
        auto osc2Bounds = topRow.removeFromLeft(oscWidth);
        osc2Card.setBounds(osc2Bounds);
        auto osc2Content = osc2Card.getContentArea();
        osc2Wave.setBounds(osc2Content.removeFromTop(selectorHeight));
        osc2Content.removeFromTop(4);
        knobSpacing = osc2Content.getWidth() / 2;
        osc2Octave.setBounds(osc2Content.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        osc2Detune.setBounds(osc2Content.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        topRow.removeFromLeft(cardGap);

        // Filter card
        auto filterBounds = topRow;
        filterCard.setBounds(filterBounds);
        auto filterContent = filterCard.getContentArea();
        filterType.setBounds(filterContent.removeFromTop(selectorHeight).removeFromLeft(120));
        filterContent.removeFromTop(4);
        knobSpacing = filterContent.getWidth() / 3;
        filterCutoff.setBounds(filterContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterResonance.setBounds(filterContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterEnvAmount.setBounds(filterContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    }

    //==========================================================================
    // BOTTOM ROW: Amp Envelope, Filter Envelope
    //==========================================================================
    {
        int totalWidth = bottomRow.getWidth();
        int envWidth = (totalWidth - cardGap) / 2;

        // Amp Envelope card
        auto ampBounds = bottomRow.removeFromLeft(envWidth);
        ampEnvCard.setBounds(ampBounds);
        auto ampContent = ampEnvCard.getContentArea();
        int knobSpacing = ampContent.getWidth() / 4;
        ampAttack.setBounds(ampContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampDecay.setBounds(ampContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampSustain.setBounds(ampContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampRelease.setBounds(ampContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        bottomRow.removeFromLeft(cardGap);

        // Filter Envelope card
        auto fltBounds = bottomRow;
        filterEnvCard.setBounds(fltBounds);
        auto fltContent = filterEnvCard.getContentArea();
        knobSpacing = fltContent.getWidth() / 4;
        filterAttack.setBounds(fltContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterDecay.setBounds(fltContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterSustain.setBounds(fltContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterRelease.setBounds(fltContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    }
}

void AnalogSynthEditor::drawDividers(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // No dividers needed - CardPanels handle their own styling
}
