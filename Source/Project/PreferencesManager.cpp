#include "PreferencesManager.h"
#include "../UI/LookAndFeel.h"

PreferencesManager::PreferencesManager()
{
    options = std::make_unique<juce::PropertiesFile::Options>();
    options->applicationName = "ProgFlow";
    options->folderName = "ProgFlow";
    options->filenameSuffix = ".settings";
    options->osxLibrarySubFolder = "Application Support";

    appProperties = std::make_unique<juce::ApplicationProperties>();
    appProperties->setStorageParameters(*options);

    // Initialize the ThemeManager based on saved preference
    auto savedTheme = getTheme();
    ThemeManager::getInstance().setTheme(
        savedTheme == Theme::Dark ? ThemeManager::Theme::Dark : ThemeManager::Theme::Light
    );
}

PreferencesManager::~PreferencesManager()
{
    saveIfNeeded();
}

PreferencesManager& PreferencesManager::getInstance()
{
    static PreferencesManager instance;
    return instance;
}

juce::PropertiesFile* PreferencesManager::getProps() const
{
    return appProperties->getUserSettings();
}

//==============================================================================
// Audio Settings

juce::String PreferencesManager::getAudioDeviceName() const
{
    return getProps()->getValue(KEY_AUDIO_DEVICE, "");
}

void PreferencesManager::setAudioDeviceName(const juce::String& deviceName)
{
    getProps()->setValue(KEY_AUDIO_DEVICE, deviceName);
    notifyAudioSettingsChanged();
}

double PreferencesManager::getSampleRate() const
{
    return getProps()->getDoubleValue(KEY_SAMPLE_RATE, DEFAULT_SAMPLE_RATE);
}

void PreferencesManager::setSampleRate(double rate)
{
    getProps()->setValue(KEY_SAMPLE_RATE, rate);
    notifyAudioSettingsChanged();
}

int PreferencesManager::getBufferSize() const
{
    return getProps()->getIntValue(KEY_BUFFER_SIZE, DEFAULT_BUFFER_SIZE);
}

void PreferencesManager::setBufferSize(int size)
{
    getProps()->setValue(KEY_BUFFER_SIZE, size);
    notifyAudioSettingsChanged();
}

//==============================================================================
// Project Settings

double PreferencesManager::getDefaultBpm() const
{
    return getProps()->getDoubleValue(KEY_DEFAULT_BPM, DEFAULT_BPM);
}

void PreferencesManager::setDefaultBpm(double bpm)
{
    getProps()->setValue(KEY_DEFAULT_BPM, bpm);
    notifyPreferencesChanged();
}

int PreferencesManager::getDefaultTimeSignatureNumerator() const
{
    return getProps()->getIntValue(KEY_DEFAULT_TIME_SIG_NUM, DEFAULT_TIME_SIG_NUM);
}

void PreferencesManager::setDefaultTimeSignatureNumerator(int num)
{
    getProps()->setValue(KEY_DEFAULT_TIME_SIG_NUM, num);
    notifyPreferencesChanged();
}

int PreferencesManager::getDefaultTimeSignatureDenominator() const
{
    return getProps()->getIntValue(KEY_DEFAULT_TIME_SIG_DENOM, DEFAULT_TIME_SIG_DENOM);
}

void PreferencesManager::setDefaultTimeSignatureDenominator(int denom)
{
    getProps()->setValue(KEY_DEFAULT_TIME_SIG_DENOM, denom);
    notifyPreferencesChanged();
}

int PreferencesManager::getAutosaveIntervalMinutes() const
{
    return getProps()->getIntValue(KEY_AUTOSAVE_INTERVAL, DEFAULT_AUTOSAVE_INTERVAL);
}

void PreferencesManager::setAutosaveIntervalMinutes(int minutes)
{
    getProps()->setValue(KEY_AUTOSAVE_INTERVAL, minutes);
    notifyPreferencesChanged();
}

bool PreferencesManager::getAutosaveEnabled() const
{
    return getProps()->getBoolValue(KEY_AUTOSAVE_ENABLED, true);
}

void PreferencesManager::setAutosaveEnabled(bool enabled)
{
    getProps()->setValue(KEY_AUTOSAVE_ENABLED, enabled);
    notifyPreferencesChanged();
}

//==============================================================================
// UI Settings

PreferencesManager::Theme PreferencesManager::getTheme() const
{
    return static_cast<Theme>(getProps()->getIntValue(KEY_THEME, static_cast<int>(Theme::Dark)));
}

void PreferencesManager::setTheme(Theme theme)
{
    getProps()->setValue(KEY_THEME, static_cast<int>(theme));

    // Update the actual visual theme
    ThemeManager::getInstance().setTheme(
        theme == Theme::Dark ? ThemeManager::Theme::Dark : ThemeManager::Theme::Light
    );

    notifyPreferencesChanged();
}

int PreferencesManager::getMeterRefreshRateHz() const
{
    return getProps()->getIntValue(KEY_METER_REFRESH_RATE, DEFAULT_METER_REFRESH_RATE);
}

void PreferencesManager::setMeterRefreshRateHz(int hz)
{
    getProps()->setValue(KEY_METER_REFRESH_RATE, hz);
    notifyPreferencesChanged();
}

bool PreferencesManager::getShowTooltips() const
{
    return getProps()->getBoolValue(KEY_SHOW_TOOLTIPS, true);
}

void PreferencesManager::setShowTooltips(bool show)
{
    getProps()->setValue(KEY_SHOW_TOOLTIPS, show);
    notifyPreferencesChanged();
}

bool PreferencesManager::getShowCpuMeter() const
{
    return getProps()->getBoolValue(KEY_SHOW_CPU_METER, true);
}

void PreferencesManager::setShowCpuMeter(bool show)
{
    getProps()->setValue(KEY_SHOW_CPU_METER, show);
    notifyPreferencesChanged();
}

//==============================================================================
// MIDI Settings

juce::String PreferencesManager::getMidiInputDevice() const
{
    return getProps()->getValue(KEY_MIDI_INPUT_DEVICE, "");
}

void PreferencesManager::setMidiInputDevice(const juce::String& deviceName)
{
    getProps()->setValue(KEY_MIDI_INPUT_DEVICE, deviceName);
    notifyMidiSettingsChanged();
}

bool PreferencesManager::getMidiLearnEnabled() const
{
    return getProps()->getBoolValue(KEY_MIDI_LEARN_ENABLED, false);
}

void PreferencesManager::setMidiLearnEnabled(bool enabled)
{
    getProps()->setValue(KEY_MIDI_LEARN_ENABLED, enabled);
    notifyMidiSettingsChanged();
}

juce::var PreferencesManager::getMidiMappings() const
{
    auto jsonStr = getProps()->getValue(KEY_MIDI_MAPPINGS, "{}");
    return juce::JSON::parse(jsonStr);
}

void PreferencesManager::setMidiMappings(const juce::var& mappings)
{
    getProps()->setValue(KEY_MIDI_MAPPINGS, juce::JSON::toString(mappings));
    notifyMidiSettingsChanged();
}

//==============================================================================
// Window State

juce::Rectangle<int> PreferencesManager::getMainWindowBounds() const
{
    int x = getProps()->getIntValue(KEY_WINDOW_X, 100);
    int y = getProps()->getIntValue(KEY_WINDOW_Y, 100);
    int w = getProps()->getIntValue(KEY_WINDOW_WIDTH, 1400);
    int h = getProps()->getIntValue(KEY_WINDOW_HEIGHT, 800);
    return { x, y, w, h };
}

void PreferencesManager::setMainWindowBounds(const juce::Rectangle<int>& bounds)
{
    getProps()->setValue(KEY_WINDOW_X, bounds.getX());
    getProps()->setValue(KEY_WINDOW_Y, bounds.getY());
    getProps()->setValue(KEY_WINDOW_WIDTH, bounds.getWidth());
    getProps()->setValue(KEY_WINDOW_HEIGHT, bounds.getHeight());
}

bool PreferencesManager::getMainWindowMaximized() const
{
    return getProps()->getBoolValue(KEY_WINDOW_MAXIMIZED, false);
}

void PreferencesManager::setMainWindowMaximized(bool maximized)
{
    getProps()->setValue(KEY_WINDOW_MAXIMIZED, maximized);
}

//==============================================================================
// Listener Management

void PreferencesManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void PreferencesManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void PreferencesManager::notifyPreferencesChanged()
{
    listeners.call(&Listener::preferencesChanged);
}

void PreferencesManager::notifyAudioSettingsChanged()
{
    listeners.call(&Listener::audioSettingsChanged);
    listeners.call(&Listener::preferencesChanged);
}

void PreferencesManager::notifyMidiSettingsChanged()
{
    listeners.call(&Listener::midiSettingsChanged);
    listeners.call(&Listener::preferencesChanged);
}

//==============================================================================
// Utility

void PreferencesManager::saveIfNeeded()
{
    if (auto* props = getProps())
        props->saveIfNeeded();
}

void PreferencesManager::resetToDefaults()
{
    auto* props = getProps();
    if (!props)
        return;

    // Clear all settings
    props->clear();

    // Set defaults explicitly
    props->setValue(KEY_SAMPLE_RATE, DEFAULT_SAMPLE_RATE);
    props->setValue(KEY_BUFFER_SIZE, DEFAULT_BUFFER_SIZE);
    props->setValue(KEY_DEFAULT_BPM, DEFAULT_BPM);
    props->setValue(KEY_DEFAULT_TIME_SIG_NUM, DEFAULT_TIME_SIG_NUM);
    props->setValue(KEY_DEFAULT_TIME_SIG_DENOM, DEFAULT_TIME_SIG_DENOM);
    props->setValue(KEY_AUTOSAVE_INTERVAL, DEFAULT_AUTOSAVE_INTERVAL);
    props->setValue(KEY_AUTOSAVE_ENABLED, true);
    props->setValue(KEY_THEME, static_cast<int>(Theme::Dark));
    ThemeManager::getInstance().setTheme(ThemeManager::Theme::Dark);
    props->setValue(KEY_METER_REFRESH_RATE, DEFAULT_METER_REFRESH_RATE);
    props->setValue(KEY_SHOW_TOOLTIPS, true);
    props->setValue(KEY_SHOW_CPU_METER, true);
    props->setValue(KEY_MIDI_LEARN_ENABLED, false);

    notifyPreferencesChanged();
    notifyAudioSettingsChanged();
    notifyMidiSettingsChanged();
}
