#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_data_structures/juce_data_structures.h>

/**
 * PreferencesManager - Centralized application settings using JUCE ApplicationProperties
 *
 * Settings categories:
 * - Audio: Output device, sample rate, buffer size
 * - Project: Default BPM, time signature, autosave interval
 * - UI: Theme, meter refresh rate, show tooltips
 * - MIDI: MIDI input device, MIDI learn mappings
 */
class PreferencesManager
{
public:
    PreferencesManager();
    ~PreferencesManager();

    // Singleton access
    static PreferencesManager& getInstance();

    //==========================================================================
    // Audio Settings

    juce::String getAudioDeviceName() const;
    void setAudioDeviceName(const juce::String& deviceName);

    double getSampleRate() const;
    void setSampleRate(double rate);

    int getBufferSize() const;
    void setBufferSize(int size);

    //==========================================================================
    // Project Settings

    double getDefaultBpm() const;
    void setDefaultBpm(double bpm);

    int getDefaultTimeSignatureNumerator() const;
    void setDefaultTimeSignatureNumerator(int num);

    int getDefaultTimeSignatureDenominator() const;
    void setDefaultTimeSignatureDenominator(int denom);

    int getAutosaveIntervalMinutes() const;
    void setAutosaveIntervalMinutes(int minutes);

    bool getAutosaveEnabled() const;
    void setAutosaveEnabled(bool enabled);

    //==========================================================================
    // UI Settings

    enum class Theme { Dark, Light };
    Theme getTheme() const;
    void setTheme(Theme theme);

    int getMeterRefreshRateHz() const;
    void setMeterRefreshRateHz(int hz);

    bool getShowTooltips() const;
    void setShowTooltips(bool show);

    bool getShowCpuMeter() const;
    void setShowCpuMeter(bool show);

    //==========================================================================
    // MIDI Settings

    juce::String getMidiInputDevice() const;
    void setMidiInputDevice(const juce::String& deviceName);

    bool getMidiLearnEnabled() const;
    void setMidiLearnEnabled(bool enabled);

    // MIDI learn mappings stored as JSON
    juce::var getMidiMappings() const;
    void setMidiMappings(const juce::var& mappings);

    //==========================================================================
    // Window State

    juce::Rectangle<int> getMainWindowBounds() const;
    void setMainWindowBounds(const juce::Rectangle<int>& bounds);

    bool getMainWindowMaximized() const;
    void setMainWindowMaximized(bool maximized);

    //==========================================================================
    // Listener interface

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void preferencesChanged() {}
        virtual void audioSettingsChanged() {}
        virtual void midiSettingsChanged() {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    //==========================================================================
    // Utility

    void saveIfNeeded();
    void resetToDefaults();

private:
    std::unique_ptr<juce::PropertiesFile::Options> options;
    std::unique_ptr<juce::ApplicationProperties> appProperties;

    juce::ListenerList<Listener> listeners;

    // Property keys
    static constexpr const char* KEY_AUDIO_DEVICE = "audioDevice";
    static constexpr const char* KEY_SAMPLE_RATE = "sampleRate";
    static constexpr const char* KEY_BUFFER_SIZE = "bufferSize";

    static constexpr const char* KEY_DEFAULT_BPM = "defaultBpm";
    static constexpr const char* KEY_DEFAULT_TIME_SIG_NUM = "defaultTimeSigNum";
    static constexpr const char* KEY_DEFAULT_TIME_SIG_DENOM = "defaultTimeSigDenom";
    static constexpr const char* KEY_AUTOSAVE_INTERVAL = "autosaveInterval";
    static constexpr const char* KEY_AUTOSAVE_ENABLED = "autosaveEnabled";

    static constexpr const char* KEY_THEME = "theme";
    static constexpr const char* KEY_METER_REFRESH_RATE = "meterRefreshRate";
    static constexpr const char* KEY_SHOW_TOOLTIPS = "showTooltips";
    static constexpr const char* KEY_SHOW_CPU_METER = "showCpuMeter";

    static constexpr const char* KEY_MIDI_INPUT_DEVICE = "midiInputDevice";
    static constexpr const char* KEY_MIDI_LEARN_ENABLED = "midiLearnEnabled";
    static constexpr const char* KEY_MIDI_MAPPINGS = "midiMappings";

    static constexpr const char* KEY_WINDOW_X = "windowX";
    static constexpr const char* KEY_WINDOW_Y = "windowY";
    static constexpr const char* KEY_WINDOW_WIDTH = "windowWidth";
    static constexpr const char* KEY_WINDOW_HEIGHT = "windowHeight";
    static constexpr const char* KEY_WINDOW_MAXIMIZED = "windowMaximized";

    // Defaults
    static constexpr double DEFAULT_SAMPLE_RATE = 44100.0;
    static constexpr int DEFAULT_BUFFER_SIZE = 512;
    static constexpr double DEFAULT_BPM = 120.0;
    static constexpr int DEFAULT_TIME_SIG_NUM = 4;
    static constexpr int DEFAULT_TIME_SIG_DENOM = 4;
    static constexpr int DEFAULT_AUTOSAVE_INTERVAL = 2;
    static constexpr int DEFAULT_METER_REFRESH_RATE = 30;

    void notifyPreferencesChanged();
    void notifyAudioSettingsChanged();
    void notifyMidiSettingsChanged();

    juce::PropertiesFile* getProps() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PreferencesManager)
};
