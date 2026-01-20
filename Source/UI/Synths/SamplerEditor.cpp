#include "SamplerEditor.h"

SamplerEditor::SamplerEditor(Sampler& s)
    : synth(s)
{
    // Preset Section
    createSectionLabel(presetLabel, "PRESET");
    addAndMakeVisible(presetLabel);

    presetSelector.addListener(this);
    addAndMakeVisible(presetSelector);
    populatePresets();

    // Master Volume
    createSectionLabel(masterLabel, "MASTER");
    addAndMakeVisible(masterLabel);
    setupKnob(masterVolume, "master_volume", "Volume");

    // Playback Section
    createSectionLabel(playbackLabel, "PLAYBACK");
    addAndMakeVisible(playbackLabel);

    setupKnob(transposeKnob, "transpose", "Trans", " st");
    setupKnob(fineTuneKnob, "fine_tune", "Fine", " ct");
    setupKnob(startKnob, "start", "Start");
    setupComboBox(loopModeSelector, "loop_mode");

    // Filter Section
    createSectionLabel(filterLabel, "FILTER");
    addAndMakeVisible(filterLabel);

    setupKnob(filterCutoff, "filter_cutoff", "Cutoff", " Hz");
    setupKnob(filterResonance, "filter_resonance", "Res");
    setupKnob(filterEnvAmount, "filter_env_amount", "Env", " Hz");

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

    // Sample Info
    createSectionLabel(sampleInfoLabel, "SAMPLE");
    addAndMakeVisible(sampleInfoLabel);

    loadSampleButton.setButtonText("Load Sample...");
    loadSampleButton.addListener(this);
    addAndMakeVisible(loadSampleButton);

    sampleInfoText.setFont(juce::Font(11.0f));
    sampleInfoText.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    sampleInfoText.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(sampleInfoText);

    refreshFromSynth();
    updateSampleInfo();
}

SamplerEditor::~SamplerEditor()
{
    presetSelector.removeListener(this);
    loopModeSelector.removeListener(this);
    loadSampleButton.removeListener(this);
}

void SamplerEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void SamplerEditor::setupComboBox(juce::ComboBox& box, const juce::String& paramId)
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

void SamplerEditor::createSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text.toUpperCase(), juce::dontSendNotification);
    label.setFont(juce::Font(10.0f));
    label.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    label.setJustificationType(juce::Justification::centredLeft);
}

void SamplerEditor::comboBoxChanged(juce::ComboBox* box)
{
    int index = box->getSelectedId() - 1;

    if (box == &presetSelector)
    {
        synth.loadPreset(index);
        refreshFromSynth();
        updateSampleInfo();
    }
    else if (box == &loopModeSelector)
    {
        synth.setParameterEnum("loop_mode", index);
    }
}

void SamplerEditor::buttonClicked(juce::Button* button)
{
    if (button == &loadSampleButton)
    {
        auto fileChooser = std::make_unique<juce::FileChooser>(
            "Select Audio Sample",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.WAV;*.aiff;*.AIFF;*.aif;*.AIF;*.mp3;*.MP3;*.flac;*.FLAC;*.ogg;*.OGG"
        );

        auto* chooser = fileChooser.release();

        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    // Load with C4 (60) as root note, full keyboard range
                    if (synth.loadSample(file, 60, 0, 127))
                    {
                        updateSampleInfo();
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "Load Failed",
                            "Could not load audio file:\n" + file.getFullPathName()
                        );
                    }
                }
                delete chooser;
            });
    }
}

void SamplerEditor::populatePresets()
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

void SamplerEditor::updateSampleInfo()
{
    auto zones = synth.getZones();

    if (zones.empty())
    {
        sampleInfoText.setText(
            "No samples loaded.\n\n"
            "Supports: WAV, AIFF, MP3, FLAC, OGG\n\n"
            "Click 'Load Sample...' or drag & drop.",
            juce::dontSendNotification);
    }
    else
    {
        juce::String info;
        info << "Loaded: " << zones.size() << " sample(s)\n\n";

        for (int i = 0; i < std::min((int)zones.size(), 4); ++i)
        {
            auto* zone = zones[i];
            info << "- " << zone->name << " (C" << (zone->rootNote / 12 - 1) << ")\n";
        }

        if (zones.size() > 4)
        {
            info << "... and " << (zones.size() - 4) << " more";
        }

        sampleInfoText.setText(info, juce::dontSendNotification);
    }
}

void SamplerEditor::refreshFromSynth()
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

    // Master
    refreshKnob(masterVolume, "master_volume");

    // Playback
    refreshKnob(transposeKnob, "transpose");
    refreshKnob(fineTuneKnob, "fine_tune");
    refreshKnob(startKnob, "start");
    refreshCombo(loopModeSelector, "loop_mode");

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

    // Preset selector
    int currentPreset = synth.getCurrentPresetIndex();
    if (currentPreset >= 0)
        presetSelector.setSelectedId(currentPreset + 1, juce::dontSendNotification);
}

void SamplerEditor::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgSecondary());

    // Calculate proportional heights
    const int totalHeight = getHeight();
    const int headerHeight = 36;
    const int row1Height = (totalHeight - headerHeight) * 55 / 100;

    const int row1 = headerHeight;
    const int row2 = row1 + row1Height;

    g.setColour(ProgFlowColours::border());

    // Horizontal dividers
    g.drawHorizontalLine(row1, 0.0f, static_cast<float>(getWidth()));
    g.drawHorizontalLine(row2, 0.0f, static_cast<float>(getWidth()));

    // Vertical dividers in row 1
    int sectionWidth = getWidth() / 3;
    g.drawVerticalLine(sectionWidth, static_cast<float>(row1), static_cast<float>(row2));
    g.drawVerticalLine(sectionWidth * 2, static_cast<float>(row1), static_cast<float>(row2));

    // Vertical dividers in row 2
    g.drawVerticalLine(getWidth() / 2, static_cast<float>(row2), static_cast<float>(getHeight()));
}

void SamplerEditor::resized()
{
    const int margin = 6;
    const int innerMargin = 4;
    const int knobSize = 50;
    const int labelHeight = 16;
    const int comboHeight = 22;

    auto bounds = getLocalBounds();
    const int totalHeight = bounds.getHeight();

    // Calculate row heights proportionally
    const int headerHeight = 36;
    const int row1Height = (totalHeight - headerHeight) * 55 / 100;

    // ROW 0: Preset & Master
    {
        auto area = bounds.removeFromTop(headerHeight).reduced(margin);

        presetLabel.setBounds(area.removeFromLeft(55).withHeight(labelHeight));
        presetSelector.setBounds(area.removeFromLeft(180).withHeight(comboHeight).withY(area.getY() + (area.getHeight() - comboHeight) / 2));

        auto masterArea = area.removeFromRight(80);
        masterLabel.setBounds(masterArea.removeFromTop(labelHeight));
        masterVolume.setBounds(masterArea.withSizeKeepingCentre(knobSize, knobSize));
    }

    // ROW 1: Playback | Filter | Amp Env
    {
        auto row = bounds.removeFromTop(row1Height).reduced(margin, innerMargin);
        int sectionWidth = row.getWidth() / 3;

        // Playback
        {
            auto area = row.removeFromLeft(sectionWidth).reduced(innerMargin, 0);
            playbackLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(2);

            auto knobRow1 = area.removeFromTop(knobSize);
            transposeKnob.setBounds(knobRow1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            fineTuneKnob.setBounds(knobRow1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));

            area.removeFromTop(2);
            startKnob.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            loopModeSelector.setBounds(area.removeFromLeft(90).withHeight(comboHeight).withY(area.getY() + 12));
        }

        // Filter
        {
            auto area = row.removeFromLeft(sectionWidth).reduced(innerMargin, 0);
            filterLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(2);

            filterCutoff.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            filterResonance.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            filterEnvAmount.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
        }

        // Amp Envelope
        {
            auto area = row.reduced(innerMargin, 0);
            ampEnvLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(2);

            ampAttack.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            ampDecay.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            ampSustain.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            ampRelease.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
        }
    }

    // ROW 2: Filter Env | Sample Info
    {
        auto row = bounds.reduced(margin, innerMargin);

        // Filter Envelope
        {
            auto area = row.removeFromLeft(row.getWidth() / 2).reduced(innerMargin, 0);
            filterEnvLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(2);

            filterAttack.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            filterDecay.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            filterSustain.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            filterRelease.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
        }

        // Sample Info
        {
            auto area = row.reduced(innerMargin, 0);
            sampleInfoLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(4);
            loadSampleButton.setBounds(area.removeFromTop(26).withWidth(110));
            area.removeFromTop(6);
            sampleInfoText.setBounds(area);
        }
    }
}
