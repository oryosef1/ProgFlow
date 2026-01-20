#include "PluginHost.h"

//==============================================================================
// Background scanner thread
class PluginScannerThread : public juce::Thread
{
public:
    PluginScannerThread(PluginHost& host, std::function<void()> onComplete)
        : Thread("Plugin Scanner"), pluginHost(host), completionCallback(onComplete)
    {
    }

    void run() override
    {
        pluginHost.scanForPlugins();

        if (completionCallback)
        {
            juce::MessageManager::callAsync(completionCallback);
        }
    }

private:
    PluginHost& pluginHost;
    std::function<void()> completionCallback;
};

//==============================================================================
PluginHost::PluginHost()
{
    addDefaultFormats();
    setDefaultSearchPaths();

    // Try to load cached plugin list
    auto listFile = getDefaultPluginListFile();
    if (listFile.existsAsFile())
    {
        loadPluginList(listFile);
    }
}

PluginHost::~PluginHost()
{
    // Stop any background scanning
    if (scanThread)
    {
        scanThread->stopThread(5000);
        scanThread.reset();
    }

    // Save plugin list on exit
    savePluginList(getDefaultPluginListFile());
}

void PluginHost::addDefaultFormats()
{
    // Add VST3 format
    formatManager.addFormat(new juce::VST3PluginFormat());

#if JUCE_MAC
    // Add AudioUnit format on macOS
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
#endif

    // Note: AAX requires special licensing from Avid
    // formatManager.addFormat(new juce::AAXPluginFormat());
}

void PluginHost::setDefaultSearchPaths()
{
    searchPaths = juce::FileSearchPath();

#if JUCE_MAC
    // macOS VST3 paths
    searchPaths.add(juce::File("/Library/Audio/Plug-Ins/VST3"));
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Audio/Plug-Ins/VST3"));

    // macOS AudioUnit paths
    searchPaths.add(juce::File("/Library/Audio/Plug-Ins/Components"));
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Audio/Plug-Ins/Components"));
#elif JUCE_WINDOWS
    // Windows VST3 paths
    searchPaths.add(juce::File("C:\\Program Files\\Common Files\\VST3"));
    searchPaths.add(juce::File("C:\\Program Files (x86)\\Common Files\\VST3"));
#elif JUCE_LINUX
    // Linux VST3 paths
    searchPaths.add(juce::File("/usr/lib/vst3"));
    searchPaths.add(juce::File("/usr/local/lib/vst3"));
    searchPaths.add(juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile(".vst3"));
#endif
}

void PluginHost::addPluginSearchPath(const juce::File& path)
{
    if (path.isDirectory())
    {
        searchPaths.add(path);
    }
}

void PluginHost::scanForPlugins()
{
    scanning.store(true);
    scanProgress.store(0.0f);

    // Get all directories to scan
    juce::StringArray paths;
    for (int i = 0; i < searchPaths.getNumPaths(); ++i)
    {
        paths.add(searchPaths[i].getFullPathName());
    }

    int totalPaths = paths.size() * formatManager.getNumFormats();
    int currentPath = 0;

    // Scan each format
    for (int formatIdx = 0; formatIdx < formatManager.getNumFormats(); ++formatIdx)
    {
        auto* format = formatManager.getFormat(formatIdx);
        if (!format) continue;

        juce::FileSearchPath formatPaths;
        for (const auto& path : paths)
        {
            formatPaths.add(juce::File(path));
        }

        // Create scanner for this format
        juce::PluginDirectoryScanner scanner(
            knownPlugins,
            *format,
            formatPaths,
            true,  // Recursive
            juce::File()  // No dead plugins file
        );

        juce::String pluginName;
        while (scanner.scanNextFile(true, pluginName))
        {
            {
                juce::ScopedLock lock(scanLock);
                currentScanPlugin = pluginName;
            }

            if (onScanProgress)
            {
                juce::MessageManager::callAsync([this, pluginName]() {
                    if (onScanProgress)
                        onScanProgress(pluginName);
                });
            }

            // Check if thread should stop
            if (auto* thread = juce::Thread::getCurrentThread())
            {
                if (thread->threadShouldExit())
                {
                    scanning.store(false);
                    return;
                }
            }
        }

        currentPath++;
        scanProgress.store(static_cast<float>(currentPath) / static_cast<float>(totalPaths));
    }

    scanning.store(false);
    scanProgress.store(1.0f);

    // Notify that plugin list has changed
    if (onPluginListChanged)
    {
        juce::MessageManager::callAsync([this]() {
            if (onPluginListChanged)
                onPluginListChanged();
        });
    }

    // Save the updated list
    savePluginList(getDefaultPluginListFile());
}

void PluginHost::scanForPluginsAsync(std::function<void()> onComplete)
{
    if (scanning.load())
        return;  // Already scanning

    // Stop any existing scan thread
    if (scanThread)
    {
        scanThread->stopThread(1000);
        scanThread.reset();
    }

    scanThread = std::make_unique<PluginScannerThread>(*this, onComplete);
    scanThread->startThread();
}

juce::String PluginHost::getCurrentScanPlugin() const
{
    juce::ScopedLock lock(const_cast<juce::CriticalSection&>(scanLock));
    return currentScanPlugin;
}

std::vector<juce::PluginDescription> PluginHost::getInstruments() const
{
    std::vector<juce::PluginDescription> result;

    for (const auto& desc : knownPlugins.getTypes())
    {
        if (desc.isInstrument)
        {
            result.push_back(desc);
        }
    }

    // Sort by name
    std::sort(result.begin(), result.end(),
        [](const juce::PluginDescription& a, const juce::PluginDescription& b) {
            return a.name.compareIgnoreCase(b.name) < 0;
        });

    return result;
}

std::vector<juce::PluginDescription> PluginHost::getEffects() const
{
    std::vector<juce::PluginDescription> result;

    for (const auto& desc : knownPlugins.getTypes())
    {
        if (!desc.isInstrument)
        {
            result.push_back(desc);
        }
    }

    // Sort by name
    std::sort(result.begin(), result.end(),
        [](const juce::PluginDescription& a, const juce::PluginDescription& b) {
            return a.name.compareIgnoreCase(b.name) < 0;
        });

    return result;
}

std::vector<juce::PluginDescription> PluginHost::getAllPlugins() const
{
    std::vector<juce::PluginDescription> result;

    for (const auto& desc : knownPlugins.getTypes())
    {
        result.push_back(desc);
    }

    // Sort by name
    std::sort(result.begin(), result.end(),
        [](const juce::PluginDescription& a, const juce::PluginDescription& b) {
            return a.name.compareIgnoreCase(b.name) < 0;
        });

    return result;
}

void PluginHost::savePluginList(const juce::File& file)
{
    auto xml = knownPlugins.createXml();
    if (xml)
    {
        file.getParentDirectory().createDirectory();
        xml->writeTo(file);
    }
}

void PluginHost::loadPluginList(const juce::File& file)
{
    if (!file.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse(file);
    if (xml)
    {
        knownPlugins.recreateFromXml(*xml);
    }
}

juce::File PluginHost::getDefaultPluginListFile() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ProgFlow")
        .getChildFile("PluginList.xml");
}

std::unique_ptr<juce::AudioPluginInstance> PluginHost::createPluginInstance(
    const juce::PluginDescription& description,
    double sampleRate,
    int blockSize,
    juce::String& errorMessage)
{
    return formatManager.createPluginInstance(
        description,
        sampleRate,
        blockSize,
        errorMessage);
}
