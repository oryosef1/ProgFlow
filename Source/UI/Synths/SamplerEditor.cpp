#include "SamplerEditor.h"

SamplerEditor::SamplerEditor(Sampler& s)
    : SynthEditorBase(), synth(s)
{
    // Setup preset selector and master volume (from base class)
    presetSelector.addListener(this);
    populatePresets();
    setupKnob(masterVolume, "master_volume", "Volume", "", "Master output volume");

    //==========================================================================
    // CARD PANELS (Saturn design - no headers for compact layout)
    //==========================================================================
    playbackCard.setShowHeader(false);
    playbackCard.setPadding(6);
    addAndMakeVisible(playbackCard);

    filterCard.setShowHeader(false);
    filterCard.setPadding(6);
    addAndMakeVisible(filterCard);

    ampEnvCard.setShowHeader(false);
    ampEnvCard.setPadding(6);
    addAndMakeVisible(ampEnvCard);

    filterEnvCard.setShowHeader(false);
    filterEnvCard.setPadding(6);
    addAndMakeVisible(filterEnvCard);

    sampleInfoCard.setShowHeader(false);
    sampleInfoCard.setPadding(6);
    addAndMakeVisible(sampleInfoCard);

    //==========================================================================
    // PLAYBACK
    //==========================================================================
    setupKnob(transposeKnob, "transpose", "Trans", " st", "Transpose - shift pitch in semitones");
    playbackCard.addAndMakeVisible(transposeKnob);
    setupKnob(fineTuneKnob, "fine_tune", "Fine", " ct", "Fine tune - subtle pitch adjustment in cents");
    playbackCard.addAndMakeVisible(fineTuneKnob);
    setupKnob(startKnob, "start", "Start", "", "Sample start position");
    playbackCard.addAndMakeVisible(startKnob);
    setupComboBox(loopModeSelector, "loop_mode");
    playbackCard.addAndMakeVisible(loopModeSelector);

    //==========================================================================
    // FILTER
    //==========================================================================
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

    //==========================================================================
    // SAMPLE INFO
    //==========================================================================
    loadSampleButton.setButtonText("Load Sample...");
    loadSampleButton.addListener(this);
    sampleInfoCard.addAndMakeVisible(loadSampleButton);

    sampleInfoText.setFont(juce::Font(11.0f));
    sampleInfoText.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    sampleInfoText.setJustificationType(juce::Justification::topLeft);
    sampleInfoCard.addAndMakeVisible(sampleInfoText);

    // Initial refresh
    refreshFromSynth();
    updateSampleInfo();
}

SamplerEditor::~SamplerEditor()
{
    loopModeSelector.removeListener(this);
    loadSampleButton.removeListener(this);
}

void SamplerEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void SamplerEditor::layoutContent(juce::Rectangle<int> area)
{
    const int cardGap = 6;
    const int knobHeight = RotaryKnob::TOTAL_HEIGHT;
    const int comboHeight = 28;

    // Two rows: Top row (Playback, Filter, Amp Env), Bottom row (Filter Env, Sample Info)
    int availableHeight = area.getHeight();
    int topRowHeight = juce::jmin(110, availableHeight / 2);

    auto topRow = area.removeFromTop(topRowHeight);
    area.removeFromTop(cardGap);
    auto bottomRow = area;

    //==========================================================================
    // TOP ROW: Playback, Filter, Amp Env
    //==========================================================================
    {
        int totalWidth = topRow.getWidth();
        int playbackWidth = (totalWidth - cardGap * 2) * 35 / 100;
        int filterWidth = (totalWidth - cardGap * 2) * 30 / 100;
        int ampWidth = totalWidth - playbackWidth - filterWidth - cardGap * 2;

        // Playback card
        auto playbackBounds = topRow.removeFromLeft(playbackWidth);
        playbackCard.setBounds(playbackBounds);
        auto playbackContent = playbackCard.getContentArea();
        int knobSpacing = playbackContent.getWidth() / 4;
        transposeKnob.setBounds(playbackContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        fineTuneKnob.setBounds(playbackContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        startKnob.setBounds(playbackContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        loopModeSelector.setBounds(playbackContent.withSizeKeepingCentre(80, comboHeight));

        topRow.removeFromLeft(cardGap);

        // Filter card
        auto filterBounds = topRow.removeFromLeft(filterWidth);
        filterCard.setBounds(filterBounds);
        auto filterContent = filterCard.getContentArea();
        knobSpacing = filterContent.getWidth() / 3;
        filterCutoff.setBounds(filterContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterResonance.setBounds(filterContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterEnvAmount.setBounds(filterContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        topRow.removeFromLeft(cardGap);

        // Amp Envelope card
        auto ampBounds = topRow;
        ampEnvCard.setBounds(ampBounds);
        auto ampContent = ampEnvCard.getContentArea();
        knobSpacing = ampContent.getWidth() / 4;
        ampAttack.setBounds(ampContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampDecay.setBounds(ampContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampSustain.setBounds(ampContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        ampRelease.setBounds(ampContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    }

    //==========================================================================
    // BOTTOM ROW: Filter Env, Sample Info
    //==========================================================================
    {
        int totalWidth = bottomRow.getWidth();
        int envWidth = (totalWidth - cardGap) / 2;

        // Filter Envelope card
        auto fltBounds = bottomRow.removeFromLeft(envWidth);
        filterEnvCard.setBounds(fltBounds);
        auto fltContent = filterEnvCard.getContentArea();
        int knobSpacing = fltContent.getWidth() / 4;
        filterAttack.setBounds(fltContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterDecay.setBounds(fltContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterSustain.setBounds(fltContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
        filterRelease.setBounds(fltContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

        bottomRow.removeFromLeft(cardGap);

        // Sample Info card
        auto sampleBounds = bottomRow;
        sampleInfoCard.setBounds(sampleBounds);
        auto sampleContent = sampleInfoCard.getContentArea();
        loadSampleButton.setBounds(sampleContent.removeFromTop(28).removeFromLeft(120));
        sampleContent.removeFromTop(4);
        sampleInfoText.setBounds(sampleContent);
    }
}

void SamplerEditor::drawDividers(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // No dividers needed - CardPanels handle their own styling
}
