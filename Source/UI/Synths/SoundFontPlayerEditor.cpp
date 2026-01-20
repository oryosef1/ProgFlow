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
    : synth(s)
{
    // Category Section
    createSectionLabel(categoryLabel, "CATEGORY");
    addAndMakeVisible(categoryLabel);

    categorySelector.addListener(this);
    addAndMakeVisible(categorySelector);
    populateCategories();

    // Instrument Section
    createSectionLabel(instrumentLabel, "INSTRUMENT");
    addAndMakeVisible(instrumentLabel);

    instrumentSelector.addListener(this);
    addAndMakeVisible(instrumentSelector);
    populateInstruments(0);

    // Master Volume
    createSectionLabel(masterLabel, "MASTER");
    addAndMakeVisible(masterLabel);
    setupKnob(masterVolume, "volume", "Volume");

    // Controls Section
    createSectionLabel(controlsLabel, "CONTROLS");
    addAndMakeVisible(controlsLabel);

    setupKnob(volumeKnob, "volume", "Volume");
    setupKnob(panKnob, "pan", "Pan");
    setupKnob(pitchBendKnob, "pitchBend", "Bend");
    setupKnob(modWheelKnob, "modWheel", "Mod");

    // Envelope Section
    createSectionLabel(envelopeLabel, "ENVELOPE OVERRIDE");
    addAndMakeVisible(envelopeLabel);

    setupKnob(attackKnob, "attackOverride", "Attack", " s");
    setupKnob(releaseKnob, "releaseOverride", "Release", " s");

    // SoundFont Section
    createSectionLabel(soundFontLabel, "SOUNDFONT");
    addAndMakeVisible(soundFontLabel);

    loadSF2Button.setButtonText("Load SF2...");
    loadSF2Button.addListener(this);
    addAndMakeVisible(loadSF2Button);

    soundFontPath.setFont(juce::Font(11.0f));
    soundFontPath.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    soundFontPath.setJustificationType(juce::Justification::topLeft);
    addAndMakeVisible(soundFontPath);

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

void SoundFontPlayerEditor::createSectionLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text.toUpperCase(), juce::dontSendNotification);
    label.setFont(juce::Font(10.0f));
    label.setColour(juce::Label::textColourId, ProgFlowColours::textMuted());
    label.setJustificationType(juce::Justification::centredLeft);
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

void SoundFontPlayerEditor::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());

    const float cornerRadius = static_cast<float>(ProgFlowSpacing::SECTION_CORNER_RADIUS);
    const int gap = ProgFlowSpacing::SM;
    const int padding = ProgFlowSpacing::MD;
    const int totalHeight = getHeight();
    const int totalWidth = getWidth();

    const int headerHeight = 44;
    const int contentHeight = totalHeight - headerHeight - padding;

    const int contentStart = headerHeight;
    const int col3Width = (totalWidth - padding * 2 - gap * 2) / 3;

    g.setColour(ProgFlowColours::sectionBg());

    // 3 sections in content row
    for (int i = 0; i < 3; i++)
    {
        int x = padding + i * (col3Width + gap);
        g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(contentStart),
                               static_cast<float>(col3Width), static_cast<float>(contentHeight), cornerRadius);
    }
}

void SoundFontPlayerEditor::resized()
{
    const int margin = 6;
    const int innerMargin = 4;
    const int knobSize = 50;
    const int labelHeight = 16;
    const int comboHeight = 24;

    auto bounds = getLocalBounds();

    // ROW 0: Category, Instrument, Master
    {
        auto area = bounds.removeFromTop(40).reduced(margin);

        // Category
        categoryLabel.setBounds(area.removeFromLeft(70).withHeight(labelHeight));
        categorySelector.setBounds(area.removeFromLeft(140).withHeight(comboHeight).withY(area.getY() + (area.getHeight() - comboHeight) / 2));

        area.removeFromLeft(20);

        // Instrument
        instrumentLabel.setBounds(area.removeFromLeft(80).withHeight(labelHeight));
        instrumentSelector.setBounds(area.removeFromLeft(180).withHeight(comboHeight).withY(area.getY() + (area.getHeight() - comboHeight) / 2));

        // Master
        auto masterArea = area.removeFromRight(80);
        masterLabel.setBounds(masterArea.removeFromTop(labelHeight));
        masterVolume.setBounds(masterArea.withSizeKeepingCentre(knobSize, knobSize));
    }

    // ROW 1: Controls | Envelope | SoundFont
    {
        auto row = bounds.reduced(margin, innerMargin);
        int sectionWidth = row.getWidth() / 3;

        // Controls
        {
            auto area = row.removeFromLeft(sectionWidth).reduced(innerMargin, 0);
            controlsLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(4);

            auto knobRow1 = area.removeFromTop(knobSize + 10);
            volumeKnob.setBounds(knobRow1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            panKnob.setBounds(knobRow1.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));

            auto knobRow2 = area.removeFromTop(knobSize + 10);
            pitchBendKnob.setBounds(knobRow2.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            modWheelKnob.setBounds(knobRow2.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
        }

        // Envelope
        {
            auto area = row.removeFromLeft(sectionWidth).reduced(innerMargin, 0);
            envelopeLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(4);

            attackKnob.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
            releaseKnob.setBounds(area.removeFromLeft(knobSize).withSizeKeepingCentre(knobSize, knobSize));
        }

        // SoundFont Info
        {
            auto area = row.reduced(innerMargin, 0);
            soundFontLabel.setBounds(area.removeFromTop(labelHeight));
            area.removeFromTop(4);

            loadSF2Button.setBounds(area.removeFromTop(28).withWidth(100));
            area.removeFromTop(8);
            soundFontPath.setBounds(area);
        }
    }
}
