#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Project/AudioExporter.h"
#include "../LookAndFeel.h"

/**
 * ExportDialog - UI for audio export settings
 *
 * Shows:
 * - Format selection (WAV/MP3)
 * - Quality options (sample rate, bit depth, bitrate)
 * - Export range (full project or selection)
 * - Progress during export
 */
class ExportDialog : public juce::Component
{
public:
    ExportDialog(AudioEngine& engine);
    ~ExportDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Show the dialog in a modal window
    static void show(AudioEngine& engine, juce::Component* parent = nullptr);

private:
    AudioEngine& audioEngine;
    std::unique_ptr<AudioExporter> exporter;

    // UI Components
    juce::Label titleLabel;

    juce::Label formatLabel;
    juce::ComboBox formatCombo;

    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateCombo;

    juce::Label bitDepthLabel;
    juce::ComboBox bitDepthCombo;

    juce::Label bitrateLabel;
    juce::ComboBox bitrateCombo;

    juce::Label rangeLabel;
    juce::Label rangeValueLabel;

    juce::ToggleButton normalizeToggle;

    juce::TextButton exportButton;
    juce::TextButton cancelButton;

    juce::ProgressBar progressBar;
    double progress = 0.0;

    // State
    bool exporting = false;

    // Handlers
    void onFormatChanged();
    void startExport();
    void cancelExport();
    void closeDialog();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
};
