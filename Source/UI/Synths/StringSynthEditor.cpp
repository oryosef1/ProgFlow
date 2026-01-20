#include "StringSynthEditor.h"

StringSynthEditor::StringSynthEditor(StringSynth& s)
    : SynthEditorBase(), synth(s)
{
    // Setup preset selector
    presetSelector.addListener(this);
    populatePresets();

    // Setup master volume
    setupKnob(masterVolume, "volume", "Volume");

    // String Sections
    createSectionLabel(sectionsLabel, "SECTIONS");
    addAndMakeVisible(sectionsLabel);

    setupKnob(violinsKnob, "violins", "Violins");
    setupKnob(violasKnob, "violas", "Violas");
    setupKnob(cellosKnob, "cellos", "Cellos");
    setupKnob(bassesKnob, "basses", "Basses");

    // Ensemble
    createSectionLabel(ensembleLabel, "ENSEMBLE");
    addAndMakeVisible(ensembleLabel);

    setupKnob(ensembleVoices, "ensemble_voices", "Voices");
    setupKnob(ensembleSpread, "ensemble_spread", "Spread", " ct");

    // Filter
    createSectionLabel(filterLabel, "FILTER");
    addAndMakeVisible(filterLabel);

    setupKnob(filterCutoff, "filter_cutoff", "Cutoff", " Hz");
    setupKnob(filterResonance, "filter_resonance", "Res");
    setupKnob(filterEnvAmount, "filter_env_amount", "Env Amt", " Hz");

    // Amp Envelope
    createSectionLabel(ampEnvLabel, "AMP ENV");
    addAndMakeVisible(ampEnvLabel);

    setupKnob(ampAttack, "amp_attack", "A", " s");
    setupKnob(ampDecay, "amp_decay", "D", " s");
    setupKnob(ampSustain, "amp_sustain", "S");
    setupKnob(ampRelease, "amp_release", "R", " s");

    // Filter Envelope
    createSectionLabel(filterEnvLabel, "FILTER ENV");
    addAndMakeVisible(filterEnvLabel);

    setupKnob(filterAttack, "filter_attack", "A", " s");
    setupKnob(filterDecay, "filter_decay", "D", " s");
    setupKnob(filterSustain, "filter_sustain", "S");
    setupKnob(filterRelease, "filter_release", "R", " s");

    // Chorus
    createSectionLabel(chorusLabel, "CHORUS");
    addAndMakeVisible(chorusLabel);

    setupKnob(chorusRate, "chorus_rate", "Rate", " Hz");
    setupKnob(chorusDepth, "chorus_depth", "Depth");
    setupKnob(chorusWet, "chorus_wet", "Mix");

    // Phaser
    createSectionLabel(phaserLabel, "PHASER");
    addAndMakeVisible(phaserLabel);

    setupKnob(phaserWet, "phaser_wet", "Mix");

    refreshFromSynth();
}

StringSynthEditor::~StringSynthEditor()
{
    presetSelector.removeListener(this);
}

void StringSynthEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
                                   const juce::String& label, const juce::String& suffix)
{
    knob.setLabel(label);
    knob.setValueSuffix(suffix);

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


void StringSynthEditor::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &presetSelector)
    {
        int index = box->getSelectedId() - 1;
        synth.loadPreset(index);
        refreshFromSynth();
    }
}

void StringSynthEditor::populatePresets()
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

void StringSynthEditor::refreshFromSynth()
{
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    // Master
    refreshKnob(masterVolume, "volume");

    // Sections
    refreshKnob(violinsKnob, "violins");
    refreshKnob(violasKnob, "violas");
    refreshKnob(cellosKnob, "cellos");
    refreshKnob(bassesKnob, "basses");

    // Ensemble
    refreshKnob(ensembleVoices, "ensemble_voices");
    refreshKnob(ensembleSpread, "ensemble_spread");

    // Filter
    refreshKnob(filterCutoff, "filter_cutoff");
    refreshKnob(filterResonance, "filter_resonance");
    refreshKnob(filterEnvAmount, "filter_env_amount");

    // Amp Envelope
    refreshKnob(ampAttack, "amp_attack");
    refreshKnob(ampDecay, "amp_decay");
    refreshKnob(ampSustain, "amp_sustain");
    refreshKnob(ampRelease, "amp_release");

    // Filter Envelope
    refreshKnob(filterAttack, "filter_attack");
    refreshKnob(filterDecay, "filter_decay");
    refreshKnob(filterSustain, "filter_sustain");
    refreshKnob(filterRelease, "filter_release");

    // Effects
    refreshKnob(chorusRate, "chorus_rate");
    refreshKnob(chorusDepth, "chorus_depth");
    refreshKnob(chorusWet, "chorus_wet");
    refreshKnob(phaserWet, "phaser_wet");

    // Preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void StringSynthEditor::layoutContent(juce::Rectangle<int> area)
{
    const int labelHeight = 18;
    const int knobSpacing = 6;

    // Split into two rows (55% / 45%)
    int row1Height = area.getHeight() * 55 / 100;
    auto row1 = area.removeFromTop(row1Height);
    row1.removeFromBottom(8); // Gap between rows
    auto row2 = area;

    // Clear divider vectors
    row1Dividers.clear();
    row2Dividers.clear();

    // ROW 1: SECTIONS | ENSEMBLE | FILTER | AMP ENV (25% each)
    {
        int sectionWidth = row1.getWidth() / 4;

        // SECTIONS (2x2 grid)
        {
            auto section = row1.removeFromLeft(sectionWidth).reduced(SECTION_PADDING, 0);
            createSectionLabel(sectionsLabel, "SECTIONS");
            addAndMakeVisible(sectionsLabel);
            sectionsLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow1 = section.removeFromTop(KNOB_SIZE);
            violinsKnob.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));
            knobRow1.removeFromLeft(knobSpacing);
            violasKnob.setBounds(knobRow1.removeFromLeft(KNOB_SIZE));

            section.removeFromTop(knobSpacing);
            auto knobRow2 = section.removeFromTop(KNOB_SIZE);
            cellosKnob.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));
            knobRow2.removeFromLeft(knobSpacing);
            bassesKnob.setBounds(knobRow2.removeFromLeft(KNOB_SIZE));

            row1Dividers.push_back(row1.getX());
        }

        // ENSEMBLE
        {
            auto section = row1.removeFromLeft(sectionWidth).reduced(SECTION_PADDING, 0);
            createSectionLabel(ensembleLabel, "ENSEMBLE");
            addAndMakeVisible(ensembleLabel);
            ensembleLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow = section.removeFromTop(KNOB_SIZE);
            ensembleVoices.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            ensembleSpread.setBounds(knobRow.removeFromLeft(KNOB_SIZE));

            row1Dividers.push_back(row1.getX());
        }

        // FILTER
        {
            auto section = row1.removeFromLeft(sectionWidth).reduced(SECTION_PADDING, 0);
            createSectionLabel(filterLabel, "FILTER");
            addAndMakeVisible(filterLabel);
            filterLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow = section.removeFromTop(KNOB_SIZE);
            filterCutoff.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            filterResonance.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            filterEnvAmount.setBounds(knobRow.removeFromLeft(KNOB_SIZE));

            row1Dividers.push_back(row1.getX());
        }

        // AMP ENV
        {
            auto section = row1.reduced(SECTION_PADDING, 0);
            createSectionLabel(ampEnvLabel, "AMP ENV");
            addAndMakeVisible(ampEnvLabel);
            ampEnvLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow = section.removeFromTop(KNOB_SIZE);
            ampAttack.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            ampDecay.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            ampSustain.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            ampRelease.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
        }
    }

    // Store row divider position
    rowDividerY = row2.getY();

    // ROW 2: FILTER ENV | CHORUS | PHASER (33% each)
    {
        int sectionWidth = row2.getWidth() / 3;

        // FILTER ENV
        {
            auto section = row2.removeFromLeft(sectionWidth).reduced(SECTION_PADDING, 0);
            createSectionLabel(filterEnvLabel, "FILTER ENV");
            addAndMakeVisible(filterEnvLabel);
            filterEnvLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow = section.removeFromTop(KNOB_SIZE);
            filterAttack.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            filterDecay.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            filterSustain.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            filterRelease.setBounds(knobRow.removeFromLeft(KNOB_SIZE));

            row2Dividers.push_back(row2.getX());
        }

        // CHORUS
        {
            auto section = row2.removeFromLeft(sectionWidth).reduced(SECTION_PADDING, 0);
            createSectionLabel(chorusLabel, "CHORUS");
            addAndMakeVisible(chorusLabel);
            chorusLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow = section.removeFromTop(KNOB_SIZE);
            chorusRate.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            chorusDepth.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
            knobRow.removeFromLeft(knobSpacing);
            chorusWet.setBounds(knobRow.removeFromLeft(KNOB_SIZE));

            row2Dividers.push_back(row2.getX());
        }

        // PHASER
        {
            auto section = row2.reduced(SECTION_PADDING, 0);
            createSectionLabel(phaserLabel, "PHASER");
            addAndMakeVisible(phaserLabel);
            phaserLabel.setBounds(section.removeFromTop(labelHeight));

            section.removeFromTop(4);
            auto knobRow = section.removeFromTop(KNOB_SIZE);
            phaserWet.setBounds(knobRow.removeFromLeft(KNOB_SIZE));
        }
    }
}

void StringSynthEditor::drawDividers(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Calculate row positions
    int row1Height = area.getHeight() * 55 / 100;
    int row1Bottom = area.getY() + row1Height - 8;
    int row2Top = area.getY() + row1Height;

    // Draw vertical dividers for row 1
    for (int x : row1Dividers)
    {
        drawVerticalDivider(g, x, area.getY(), row1Bottom);
    }

    // Draw horizontal divider between rows
    drawHorizontalDivider(g, area.getX(), area.getRight(), rowDividerY - 4);

    // Draw vertical dividers for row 2
    for (int x : row2Dividers)
    {
        drawVerticalDivider(g, x, row2Top, area.getBottom());
    }
}
