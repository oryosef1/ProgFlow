#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <functional>
#include <memory>

/**
 * PluginHost - Manages VST3/AU plugin scanning, loading, and hosting
 *
 * Responsibilities:
 * - Scan for installed plugins on the system
 * - Maintain a KnownPluginList with persistence
 * - Create plugin instances
 * - Provide plugin browser data to UI
 */
class PluginHost
{
public:
    PluginHost();
    ~PluginHost();

    //==========================================================================
    // Plugin Format Management
    void addDefaultFormats();  // Add VST3, AU, etc.

    //==========================================================================
    // Plugin Scanning
    void scanForPlugins();
    void scanForPluginsAsync(std::function<void()> onComplete = nullptr);
    bool isScanning() const { return scanning.load(); }
    float getScanProgress() const { return scanProgress.load(); }
    juce::String getCurrentScanPlugin() const;

    // Scan paths
    void addPluginSearchPath(const juce::File& path);
    void setDefaultSearchPaths();
    const juce::FileSearchPath& getSearchPaths() const { return searchPaths; }

    //==========================================================================
    // Plugin List
    juce::KnownPluginList& getKnownPlugins() { return knownPlugins; }
    const juce::KnownPluginList& getKnownPlugins() const { return knownPlugins; }

    // Get plugins by category
    std::vector<juce::PluginDescription> getInstruments() const;
    std::vector<juce::PluginDescription> getEffects() const;
    std::vector<juce::PluginDescription> getAllPlugins() const;

    // Persistence
    void savePluginList(const juce::File& file);
    void loadPluginList(const juce::File& file);
    juce::File getDefaultPluginListFile() const;

    //==========================================================================
    // Plugin Instance Creation
    std::unique_ptr<juce::AudioPluginInstance> createPluginInstance(
        const juce::PluginDescription& description,
        double sampleRate,
        int blockSize,
        juce::String& errorMessage);

    // Get format manager for direct access if needed
    juce::AudioPluginFormatManager& getFormatManager() { return formatManager; }

    //==========================================================================
    // Callbacks
    std::function<void()> onPluginListChanged;
    std::function<void(const juce::String&)> onScanProgress;

private:
    juce::AudioPluginFormatManager formatManager;
    juce::KnownPluginList knownPlugins;
    juce::FileSearchPath searchPaths;

    // Scanning state
    std::atomic<bool> scanning{false};
    std::atomic<float> scanProgress{0.0f};
    juce::String currentScanPlugin;
    juce::CriticalSection scanLock;

    // Background scanning
    std::unique_ptr<juce::Thread> scanThread;

    // Helper to scan a single directory
    void scanDirectory(const juce::File& dir, juce::AudioPluginFormat& format);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginHost)
};
