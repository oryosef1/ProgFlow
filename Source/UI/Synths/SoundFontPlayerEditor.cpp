#include "SoundFontPlayerEditor.h"

// GM Category names
const char* SoundFontPlayerEditor::categoryNames[NUM_CATEGORIES] = {
    "Piano",
    "Chromatic Percussion",
    "Organ",
    "Guitar",
    "Bass",
    "Strings",
    "Ensemble",
    "Brass",
    "Reed",
    "Pipe",
    "Synth Lead",
    "Synth Pad",
    "Synth Effects",
    "Ethnic",
    "Percussive",
    "Sound Effects"
};

SoundFontPlayerEditor::SoundFontPlayerEditor(SoundFontPlayer& s)
    : SynthEditorBase(), synth(s)
{
    // Hide the base class preset selector - SoundFont uses category/instrument instead
    presetSelector.setVisible(false);
    presetLabel.setVisible(false);

    // Setup master volume (from base class)
    setupKnob(masterVolume, "volume", "Volume", "", "Master output volume");

    // Category/Instrument selectors for header
    categorySelector.addListener(this);
    addAndMakeVisible(categorySelector);
    populateCategories();

    instrumentSelector.addListener(this);
    addAndMakeVisible(instrumentSelector);
    populateInstruments(0);

    //==========================================================================
    // CARD PANELS (Saturn design - no headers for compact layout)
    //==========================================================================
    controlsCard.setShowHeader(false);
    controlsCard.setPadding(6);
    addAndMakeVisible(controlsCard);

    envelopeCard.setShowHeader(false);
    envelopeCard.setPadding(6);
    addAndMakeVisible(envelopeCard);

    soundFontCard.setShowHeader(false);
    soundFontCard.setPadding(6);
    addAndMakeVisible(soundFontCard);

    //==========================================================================
    // CONTROLS
    //==========================================================================
    setupKnob(volumeKnob, "volume", "Volume", "", "Channel volume level");
    controlsCard.addAndMakeVisible(volumeKnob);
    setupKnob(panKnob, "pan", "Pan", "", "Stereo position (-1 = left, +1 = right)");
    controlsCard.addAndMakeVisible(panKnob);
    setupKnob(pitchBendKnob, "pitchBend", "Bend", "", "Pitch bend range in semitones");
    controlsCard.addAndMakeVisible(pitchBendKnob);
    setupKnob(modWheelKnob, "modWheel", "Mod", "", "Modulation wheel amount");
    controlsCard.addAndMakeVisible(modWheelKnob);

    //==========================================================================
    // ENVELOPE OVERRIDES
    //==========================================================================
    setupKnob(attackKnob, "attackOverride", "Attack", " s", "Override attack time - slower for pads, faster for plucks");
    envelopeCard.addAndMakeVisible(attackKnob);
    setupKnob(releaseKnob, "releaseOverride", "Release", " s", "Override release time - how long sound rings after note off");
    envelopeCard.addAndMakeVisible(releaseKnob);

    //==========================================================================
    // SOUNDFONT INFO
    //==========================================================================
    loadSF2Button.setButtonText("Load SF2...");
    loadSF2Button.addListener(this);
    soundFontCard.addAndMakeVisible(loadSF2Button);

    soundFontPath.setFont(juce::Font(11.0f));
    soundFontPath.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    soundFontPath.setJustificationType(juce::Justification::topLeft);
    soundFontCard.addAndMakeVisible(soundFontPath);

    refreshFromSynth();
    updateSoundFontInfo();
}

SoundFontPlayerEditor::~SoundFontPlayerEditor()
{
    categorySelector.removeListener(this);
    instrumentSelector.removeListener(this);
    loadSF2Button.removeListener(this);
}

void SoundFontPlayerEditor::setupKnob(RotaryKnob& knob, const juce::String& paramId,
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

void SoundFontPlayerEditor::populateCategories()
{
    categorySelector.clear();

    for (int i = 0; i < NUM_CATEGORIES; ++i)
    {
        categorySelector.addItem(categoryNames[i], i + 1);
    }

    categorySelector.setSelectedId(1, juce::dontSendNotification);
}

void SoundFontPlayerEditor::populateInstruments(int categoryIndex)
{
    instrumentSelector.clear();

    int startProgram = categoryIndex * 8;
    auto allNames = SoundFontPlayer::getAllInstrumentNames();

    for (int i = 0; i < 8; ++i)
    {
        int program = startProgram + i;
        if (program < 128)
        {
            instrumentSelector.addItem(allNames[program], i + 1);
        }
    }

    instrumentSelector.setSelectedId(1, juce::dontSendNotification);
}

void SoundFontPlayerEditor::updateSoundFontInfo()
{
    if (synth.isSoundFontLoaded())
    {
        juce::String path = synth.getCurrentSoundFontPath();
        if (path.length() > 40)
        {
            path = "..." + path.substring(path.length() - 37);
        }
        soundFontPath.setText(path, juce::dontSendNotification);
    }
    else
    {
        soundFontPath.setText(
            "No SoundFont loaded.\n\n"
            "Recommended free SoundFonts:\n"
            "- GeneralUser GS (~30MB)\n"
            "- FluidR3 GM (~140MB)\n\n"
            "Search online for download links.",
            juce::dontSendNotification);
    }
}

void SoundFontPlayerEditor::comboBoxChanged(juce::ComboBox* box)
{
    if (box == &categorySelector)
    {
        int categoryIndex = box->getSelectedId() - 1;
        populateInstruments(categoryIndex);

        // Select first instrument in category
        int program = categoryIndex * 8;
        synth.setParameter("instrument", static_cast<float>(program));
    }
    else if (box == &instrumentSelector)
    {
        int categoryIndex = categorySelector.getSelectedId() - 1;
        int instrumentInCategory = box->getSelectedId() - 1;
        int program = categoryIndex * 8 + instrumentInCategory;

        synth.setParameter("instrument", static_cast<float>(program));
    }
}

void SoundFontPlayerEditor::buttonClicked(juce::Button* button)
{
    if (button == &loadSF2Button)
    {
        auto fileChooser = std::make_unique<juce::FileChooser>(
            "Select SoundFont File",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.sf2;*.SF2"
        );

        auto* chooser = fileChooser.release();

        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this, chooser](const juce::FileChooser& fc)
            {
                auto file = fc.getResult();
                if (file.existsAsFile())
                {
                    if (synth.loadSoundFont(file.getFullPathName()))
                    {
                        updateSoundFontInfo();
                    }
                    else
                    {
                        juce::AlertWindow::showMessageBoxAsync(
                            juce::MessageBoxIconType::WarningIcon,
                            "Load Failed",
                            "Could not load SoundFont file:\n" + file.getFullPathName()
                        );
                    }
                }
                delete chooser;
            });
    }
}

void SoundFontPlayerEditor::refreshFromSynth()
{
    auto refreshKnob = [this](RotaryKnob& knob, const juce::String& paramId)
    {
        if (auto* param = synth.getParameterInfo(paramId))
            knob.setValue(param->value, juce::dontSendNotification);
    };

    // Controls
    refreshKnob(masterVolume, "volume");
    refreshKnob(volumeKnob, "volume");
    refreshKnob(panKnob, "pan");
    refreshKnob(pitchBendKnob, "pitchBend");
    refreshKnob(modWheelKnob, "modWheel");

    // Envelope
    refreshKnob(attackKnob, "attackOverride");
    refreshKnob(releaseKnob, "releaseOverride");

    // Update instrument selection
    int instrument = static_cast<int>(synth.getParameter("instrument"));
    int category = instrument / 8;
    int instrumentInCategory = instrument % 8;

    categorySelector.setSelectedId(category + 1, juce::dontSendNotification);
    populateInstruments(category);
    instrumentSelector.setSelectedId(instrumentInCategory + 1, juce::dontSendNotification);
}

void SoundFontPlayerEditor::layoutContent(juce::Rectangle<int> area)
{
    const int cardGap = 6;
    const int knobHeight = RotaryKnob::TOTAL_HEIGHT;

    // Add Category/Instrument selectors to header area (where preset selector would be)
    auto headerBounds = getLocalBounds().removeFromTop(HEADER_HEIGHT).reduced(SECTION_PADDING, 0);
    auto selectorArea = headerBounds.reduced(0, 16);
    categorySelector.setBounds(selectorArea.removeFromLeft(130).withHeight(28));
    selectorArea.removeFromLeft(8);
    instrumentSelector.setBounds(selectorArea.removeFromLeft(150).withHeight(28));

    // Single row: Controls | Envelope | SoundFont
    int totalWidth = area.getWidth();
    int controlsWidth = (totalWidth - cardGap * 2) * 40 / 100;
    int envWidth = (totalWidth - cardGap * 2) * 25 / 100;
    int sfWidth = totalWidth - controlsWidth - envWidth - cardGap * 2;

    //==========================================================================
    // CONTROLS CARD (4 knobs: 2x2 grid)
    //==========================================================================
    auto controlsBounds = area.removeFromLeft(controlsWidth);
    controlsCard.setBounds(controlsBounds);
    auto controlsContent = controlsCard.getContentArea();

    int knobSpacing = controlsContent.getWidth() / 2;
    int halfHeight = controlsContent.getHeight() / 2;

    auto topRow = controlsContent.removeFromTop(halfHeight);
    volumeKnob.setBounds(topRow.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    panKnob.setBounds(topRow.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

    pitchBendKnob.setBounds(controlsContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    modWheelKnob.setBounds(controlsContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

    area.removeFromLeft(cardGap);

    //==========================================================================
    // ENVELOPE CARD
    //==========================================================================
    auto envBounds = area.removeFromLeft(envWidth);
    envelopeCard.setBounds(envBounds);
    auto envContent = envelopeCard.getContentArea();
    knobSpacing = envContent.getWidth() / 2;
    attackKnob.setBounds(envContent.removeFromLeft(knobSpacing).withSizeKeepingCentre(KNOB_SIZE, knobHeight));
    releaseKnob.setBounds(envContent.withSizeKeepingCentre(KNOB_SIZE, knobHeight));

    area.removeFromLeft(cardGap);

    //==========================================================================
    // SOUNDFONT CARD
    //==========================================================================
    soundFontCard.setBounds(area);
    auto sfContent = soundFontCard.getContentArea();
    loadSF2Button.setBounds(sfContent.removeFromTop(28).removeFromLeft(110));
    sfContent.removeFromTop(6);
    soundFontPath.setBounds(sfContent);
}

void SoundFontPlayerEditor::drawDividers(juce::Graphics& /*g*/, juce::Rectangle<int> /*area*/)
{
    // No dividers needed - CardPanels handle their own styling
}
