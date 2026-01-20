#include "ExportDialog.h"

ExportDialog::ExportDialog(AudioEngine& engine)
    : audioEngine(engine),
      progressBar(progress)
{
    exporter = std::make_unique<AudioExporter>(audioEngine);

    // Title
    titleLabel.setText("Export Audio", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(20.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Format selection
    formatLabel.setText("Format:", juce::dontSendNotification);
    addAndMakeVisible(formatLabel);

    formatCombo.addItem("WAV", 1);
    formatCombo.addItem("MP3", 2);
    formatCombo.setSelectedId(1);
    formatCombo.onChange = [this]() { onFormatChanged(); };
    addAndMakeVisible(formatCombo);

    // Sample rate
    sampleRateLabel.setText("Sample Rate:", juce::dontSendNotification);
    addAndMakeVisible(sampleRateLabel);

    sampleRateCombo.addItem("44100 Hz", 1);
    sampleRateCombo.addItem("48000 Hz", 2);
    sampleRateCombo.addItem("96000 Hz", 3);
    sampleRateCombo.setSelectedId(1);
    addAndMakeVisible(sampleRateCombo);

    // Bit depth (WAV only)
    bitDepthLabel.setText("Bit Depth:", juce::dontSendNotification);
    addAndMakeVisible(bitDepthLabel);

    bitDepthCombo.addItem("16-bit", 1);
    bitDepthCombo.addItem("24-bit", 2);
    bitDepthCombo.addItem("32-bit float", 3);
    bitDepthCombo.setSelectedId(1);
    addAndMakeVisible(bitDepthCombo);

    // Bitrate (MP3 only)
    bitrateLabel.setText("Bitrate:", juce::dontSendNotification);
    addAndMakeVisible(bitrateLabel);

    bitrateCombo.addItem("128 kbps", 1);
    bitrateCombo.addItem("192 kbps", 2);
    bitrateCombo.addItem("256 kbps", 3);
    bitrateCombo.addItem("320 kbps", 4);
    bitrateCombo.setSelectedId(2); // Default 192
    addChildComponent(bitrateCombo); // Hidden by default

    // Range info
    rangeLabel.setText("Export Range:", juce::dontSendNotification);
    addAndMakeVisible(rangeLabel);

    double projectLength = AudioExporter::calculateProjectLengthBars(audioEngine);
    rangeValueLabel.setText("Bar 1 to Bar " + juce::String(static_cast<int>(projectLength)),
                           juce::dontSendNotification);
    addAndMakeVisible(rangeValueLabel);

    // Normalize toggle
    normalizeToggle.setButtonText("Normalize output");
    normalizeToggle.setToggleState(false, juce::dontSendNotification);
    addAndMakeVisible(normalizeToggle);

    // Buttons
    exportButton.setButtonText("Export");
    exportButton.onClick = [this]() { startExport(); };
    addAndMakeVisible(exportButton);

    cancelButton.setButtonText("Cancel");
    cancelButton.onClick = [this]() { cancelExport(); };
    addAndMakeVisible(cancelButton);

    // Progress bar (hidden initially)
    addChildComponent(progressBar);

    // Initial state
    onFormatChanged();

    setSize(400, 350);
}

ExportDialog::~ExportDialog()
{
    if (exporter && exporter->isExporting())
        exporter->cancelExport();
}

void ExportDialog::paint(juce::Graphics& g)
{
    g.fillAll(ProgFlowColours::bgPrimary());

    // Draw border
    g.setColour(ProgFlowColours::border());
    g.drawRect(getLocalBounds(), 1);
}

void ExportDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);

    // Title
    titleLabel.setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(20);

    // Row height
    const int rowHeight = 30;
    const int labelWidth = 100;
    const int spacing = 10;

    // Format row
    auto formatRow = bounds.removeFromTop(rowHeight);
    formatLabel.setBounds(formatRow.removeFromLeft(labelWidth));
    formatCombo.setBounds(formatRow);
    bounds.removeFromTop(spacing);

    // Sample rate row
    auto srRow = bounds.removeFromTop(rowHeight);
    sampleRateLabel.setBounds(srRow.removeFromLeft(labelWidth));
    sampleRateCombo.setBounds(srRow);
    bounds.removeFromTop(spacing);

    // Bit depth row (WAV)
    auto bdRow = bounds.removeFromTop(rowHeight);
    bitDepthLabel.setBounds(bdRow.removeFromLeft(labelWidth));
    bitDepthCombo.setBounds(bdRow);

    // Bitrate row (MP3) - same position as bit depth
    bitrateLabel.setBounds(bdRow.withX(0).withWidth(labelWidth));
    bitrateCombo.setBounds(bdRow);
    bounds.removeFromTop(spacing);

    // Range row
    auto rangeRow = bounds.removeFromTop(rowHeight);
    rangeLabel.setBounds(rangeRow.removeFromLeft(labelWidth));
    rangeValueLabel.setBounds(rangeRow);
    bounds.removeFromTop(spacing);

    // Normalize toggle
    normalizeToggle.setBounds(bounds.removeFromTop(rowHeight));
    bounds.removeFromTop(spacing);

    // Progress bar
    progressBar.setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(20);

    // Buttons
    auto buttonRow = bounds.removeFromTop(35);
    cancelButton.setBounds(buttonRow.removeFromRight(100));
    buttonRow.removeFromRight(10);
    exportButton.setBounds(buttonRow.removeFromRight(100));
}

void ExportDialog::onFormatChanged()
{
    bool isWav = (formatCombo.getSelectedId() == 1);

    bitDepthLabel.setVisible(isWav);
    bitDepthCombo.setVisible(isWav);

    bitrateLabel.setVisible(!isWav);
    bitrateCombo.setVisible(!isWav);
}

void ExportDialog::startExport()
{
    if (exporting)
        return;

    // Determine format and settings
    AudioExporter::Format format = (formatCombo.getSelectedId() == 1)
                                   ? AudioExporter::Format::WAV
                                   : AudioExporter::Format::MP3;

    AudioExporter::ExportSettings settings;

    // Sample rate
    switch (sampleRateCombo.getSelectedId())
    {
        case 1: settings.sampleRate = 44100; break;
        case 2: settings.sampleRate = 48000; break;
        case 3: settings.sampleRate = 96000; break;
        default: settings.sampleRate = 44100;
    }

    // Bit depth (WAV)
    switch (bitDepthCombo.getSelectedId())
    {
        case 1: settings.bitDepth = 16; break;
        case 2: settings.bitDepth = 24; break;
        case 3: settings.bitDepth = 32; break;
        default: settings.bitDepth = 16;
    }

    // Bitrate (MP3)
    switch (bitrateCombo.getSelectedId())
    {
        case 1: settings.mp3Bitrate = 128; break;
        case 2: settings.mp3Bitrate = 192; break;
        case 3: settings.mp3Bitrate = 256; break;
        case 4: settings.mp3Bitrate = 320; break;
        default: settings.mp3Bitrate = 192;
    }

    settings.startBar = 0.0;
    settings.endBar = AudioExporter::calculateProjectLengthBars(audioEngine);
    settings.normalizeOutput = normalizeToggle.getToggleState();

    // File chooser
    juce::String extension = (format == AudioExporter::Format::WAV) ? "*.wav" : "*.mp3";
    juce::String defaultName = juce::String("export") + ((format == AudioExporter::Format::WAV) ? ".wav" : ".mp3");

    auto chooser = std::make_shared<juce::FileChooser>(
        "Export Audio",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile(defaultName),
        extension);

    auto flags = juce::FileBrowserComponent::saveMode
               | juce::FileBrowserComponent::canSelectFiles
               | juce::FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync(flags, [this, chooser, format, settings](const juce::FileChooser& fc)
    {
        auto results = fc.getResults();
        if (results.isEmpty())
            return;

        juce::File outputFile = results.getFirst();

        // Ensure correct extension
        juce::String ext = (format == AudioExporter::Format::WAV) ? ".wav" : ".mp3";
        if (!outputFile.hasFileExtension(ext.substring(1)))
            outputFile = outputFile.withFileExtension(ext.substring(1));

        // Start export
        exporting = true;
        progress = 0.0;
        progressBar.setVisible(true);
        exportButton.setEnabled(false);
        cancelButton.setButtonText("Cancel Export");

        exporter->exportAsync(outputFile, format, settings,
            // Progress callback
            [this](float p)
            {
                progress = p;
                progressBar.repaint();
            },
            // Completion callback
            [this, outputFile](bool success, const juce::String& errorMessage)
            {
                exporting = false;
                progressBar.setVisible(false);
                exportButton.setEnabled(true);
                cancelButton.setButtonText("Close");

                if (success)
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::InfoIcon,
                        "Export Complete",
                        "Audio exported successfully to:\n" + outputFile.getFullPathName());

                    closeDialog();
                }
                else
                {
                    juce::AlertWindow::showMessageBoxAsync(
                        juce::AlertWindow::WarningIcon,
                        "Export Failed",
                        errorMessage.isEmpty() ? "Unknown error occurred" : errorMessage);
                }
            });
    });
}

void ExportDialog::cancelExport()
{
    if (exporting)
    {
        exporter->cancelExport();
    }
    else
    {
        closeDialog();
    }
}

void ExportDialog::closeDialog()
{
    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState(0);
    }
}

//==============================================================================
// Static show method

void ExportDialog::show(AudioEngine& engine, juce::Component* parent)
{
    auto* dialog = new ExportDialog(engine);

    juce::DialogWindow::LaunchOptions options;
    options.dialogTitle = "Export Audio";
    options.dialogBackgroundColour = ProgFlowColours::bgPrimary();
    options.content.setOwned(dialog);
    options.componentToCentreAround = parent;
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = true;
    options.resizable = false;

    options.launchAsync();
}
