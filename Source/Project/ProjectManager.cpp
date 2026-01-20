#include "ProjectManager.h"

ProjectManager::ProjectManager(AudioEngine& engine)
    : audioEngine(engine)
{
    initializeProperties();

    // Start autosave timer (check every minute)
    if (autosaveEnabled)
        startTimer(60000);
}

ProjectManager::~ProjectManager()
{
    stopTimer();

    // Clean up recovery file on clean exit
    if (!dirty)
        clearRecoveryFile();
}

void ProjectManager::initializeProperties()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "ProgFlow";
    options.folderName = "ProgFlow";
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";

    appProperties.setStorageParameters(options);

    // Load recent files from properties
    if (auto* props = appProperties.getUserSettings())
    {
        recentFiles.restoreFromString(props->getValue("recentFiles", ""));
        recentFiles.setMaxNumberOfItems(MAX_RECENT_FILES);

        // Load autosave settings
        autosaveEnabled = props->getBoolValue("autosaveEnabled", true);
        autosaveIntervalMinutes = props->getIntValue("autosaveInterval", 2);
    }
}

//==============================================================================
// Project state

void ProjectManager::setProjectName(const juce::String& name)
{
    if (projectName != name)
    {
        projectName = name;
        markDirty();
        notifyStateChanged();
    }
}

void ProjectManager::markDirty()
{
    if (!dirty)
    {
        dirty = true;
        notifyStateChanged();
    }
}

void ProjectManager::markClean()
{
    if (dirty)
    {
        dirty = false;
        lastSaveTime = juce::Time::currentTimeMillis();
        notifyStateChanged();
    }
}

void ProjectManager::setBpm(double newBpm)
{
    newBpm = juce::jlimit(20.0, 300.0, newBpm);
    if (bpm != newBpm)
    {
        bpm = newBpm;
        audioEngine.setBpm(newBpm);
        markDirty();
    }
}

void ProjectManager::setTimeSignature(int num, int den)
{
    if (timeSigNum != num || timeSigDen != den)
    {
        timeSigNum = num;
        timeSigDen = den;
        markDirty();
    }
}

//==============================================================================
// File operations

bool ProjectManager::newProject()
{
    if (!checkUnsavedChanges())
        return false;

    // Clear all tracks
    while (audioEngine.getNumTracks() > 0)
        audioEngine.removeTrack(0);

    // Reset state
    projectName = "Untitled";
    projectFile = juce::File();
    bpm = 120.0;
    timeSigNum = 4;
    timeSigDen = 4;
    audioEngine.setBpm(bpm);

    markClean();
    notifyProjectLoaded();

    return true;
}

bool ProjectManager::saveProject()
{
    if (!projectFile.existsAsFile())
        return saveProjectAs();

    // Serialize and write
    juce::String json = ProjectSerializer::serializeFromEngine(
        audioEngine, projectName, bpm, timeSigNum, timeSigDen);

    if (projectFile.replaceWithText(json))
    {
        markClean();
        clearRecoveryFile();
        notifyProjectSaved();
        return true;
    }

    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Save Failed",
        "Could not write to file: " + projectFile.getFullPathName());

    return false;
}

bool ProjectManager::saveProjectAs()
{
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save Project",
        projectFile.existsAsFile() ? projectFile.getParentDirectory() : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.progflow");

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto results = fc.getResults();
        if (results.isEmpty())
            return;

        juce::File file = results.getFirst();

        // Ensure .progflow extension
        if (!file.hasFileExtension(".progflow"))
            file = file.withFileExtension(".progflow");

        // Update project name from filename
        projectName = file.getFileNameWithoutExtension();
        projectFile = file;

        // Serialize and write
        juce::String json = ProjectSerializer::serializeFromEngine(
            audioEngine, projectName, bpm, timeSigNum, timeSigDen);

        if (file.replaceWithText(json))
        {
            addToRecentProjects(file);
            markClean();
            clearRecoveryFile();
            notifyProjectSaved();
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Save Failed",
                "Could not write to file: " + file.getFullPathName());
        }
    });

    return true; // Async, always returns true
}

bool ProjectManager::openProject()
{
    if (!checkUnsavedChanges())
        return false;

    auto chooser = std::make_shared<juce::FileChooser>(
        "Open Project",
        projectFile.existsAsFile() ? projectFile.getParentDirectory() : juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.progflow");

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this, chooser](const juce::FileChooser& fc)
    {
        auto results = fc.getResults();
        if (!results.isEmpty())
        {
            openProject(results.getFirst());
        }
    });

    return true; // Async, always returns true
}

bool ProjectManager::openProject(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Open Failed",
            "File not found: " + file.getFullPathName());
        return false;
    }

    juce::String json = file.loadFileAsString();
    if (json.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Open Failed",
            "Could not read file: " + file.getFullPathName());
        return false;
    }

    juce::String loadedName;
    double loadedBpm;

    if (ProjectSerializer::deserializeToEngine(json, audioEngine, loadedName, loadedBpm))
    {
        projectName = loadedName;
        projectFile = file;
        bpm = loadedBpm;
        audioEngine.setBpm(bpm);

        addToRecentProjects(file);
        markClean();
        notifyProjectLoaded();

        return true;
    }

    juce::AlertWindow::showMessageBoxAsync(
        juce::AlertWindow::WarningIcon,
        "Open Failed",
        "Could not parse project file: " + file.getFullPathName());

    return false;
}

//==============================================================================
// Recent projects

juce::StringArray ProjectManager::getRecentProjects() const
{
    juce::StringArray paths;
    for (int i = 0; i < recentFiles.getNumFiles(); ++i)
    {
        paths.add(recentFiles.getFile(i).getFullPathName());
    }
    return paths;
}

void ProjectManager::clearRecentProjects()
{
    recentFiles.clear();

    if (auto* props = appProperties.getUserSettings())
    {
        props->setValue("recentFiles", "");
        props->saveIfNeeded();
    }
}

void ProjectManager::addToRecentProjects(const juce::File& file)
{
    recentFiles.addFile(file);

    if (auto* props = appProperties.getUserSettings())
    {
        props->setValue("recentFiles", recentFiles.toString());
        props->saveIfNeeded();
    }
}

//==============================================================================
// Autosave

void ProjectManager::setAutosaveEnabled(bool enabled)
{
    autosaveEnabled = enabled;

    if (enabled)
        startTimer(60000);
    else
        stopTimer();

    if (auto* props = appProperties.getUserSettings())
    {
        props->setValue("autosaveEnabled", enabled);
        props->saveIfNeeded();
    }
}

void ProjectManager::setAutosaveIntervalMinutes(int minutes)
{
    autosaveIntervalMinutes = juce::jlimit(1, 30, minutes);

    if (auto* props = appProperties.getUserSettings())
    {
        props->setValue("autosaveInterval", autosaveIntervalMinutes);
        props->saveIfNeeded();
    }
}

juce::File ProjectManager::getAutosaveDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("ProgFlow")
        .getChildFile("autosave");
}

juce::File ProjectManager::getAutosaveFile() const
{
    juce::String safeName = projectName.replaceCharacter(' ', '_')
                                       .replaceCharacter('/', '_')
                                       .replaceCharacter('\\', '_');
    return getAutosaveDirectory().getChildFile(safeName + "-recovery.progflow");
}

bool ProjectManager::hasRecoveryFile() const
{
    return getAutosaveFile().existsAsFile();
}

bool ProjectManager::recoverFromAutosave()
{
    juce::File recovery = getAutosaveFile();
    if (!recovery.existsAsFile())
        return false;

    return openProject(recovery);
}

void ProjectManager::clearRecoveryFile()
{
    juce::File recovery = getAutosaveFile();
    if (recovery.existsAsFile())
        recovery.deleteFile();
}

void ProjectManager::performAutosave()
{
    if (!dirty || projectName == "Untitled")
        return;

    // Create autosave directory if needed
    juce::File autosaveDir = getAutosaveDirectory();
    if (!autosaveDir.exists())
        autosaveDir.createDirectory();

    juce::File autosaveFile = getAutosaveFile();

    // Serialize and write
    juce::String json = ProjectSerializer::serializeFromEngine(
        audioEngine, projectName, bpm, timeSigNum, timeSigDen);

    if (autosaveFile.replaceWithText(json))
    {
        DBG("Autosaved to: " << autosaveFile.getFullPathName());
    }
}

void ProjectManager::timerCallback()
{
    if (!autosaveEnabled || !dirty)
        return;

    juce::int64 now = juce::Time::currentTimeMillis();
    juce::int64 intervalMs = autosaveIntervalMinutes * 60 * 1000;

    if (now - lastSaveTime >= intervalMs)
    {
        performAutosave();
        lastSaveTime = now;
    }
}

//==============================================================================
// Listeners

void ProjectManager::addListener(Listener* listener)
{
    listeners.add(listener);
}

void ProjectManager::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void ProjectManager::notifyStateChanged()
{
    listeners.call(&Listener::projectStateChanged);
}

void ProjectManager::notifyProjectLoaded()
{
    listeners.call(&Listener::projectLoaded);
}

void ProjectManager::notifyProjectSaved()
{
    listeners.call(&Listener::projectSaved);
}

//==============================================================================
// Helpers

bool ProjectManager::checkUnsavedChanges()
{
    if (!dirty)
        return true;

    int result = juce::AlertWindow::showYesNoCancelBox(
        juce::AlertWindow::QuestionIcon,
        "Unsaved Changes",
        "Do you want to save changes to \"" + projectName + "\"?",
        "Save",
        "Don't Save",
        "Cancel",
        nullptr,
        nullptr);

    if (result == 1) // Save
    {
        // Use synchronous save to ensure save completes before proceeding
        return saveProjectSync();
    }
    else if (result == 2) // Don't Save
    {
        return true;
    }
    else // Cancel
    {
        return false;
    }
}

bool ProjectManager::saveProjectSync()
{
    // If we already have a file, save directly to it
    if (projectFile.existsAsFile())
    {
        juce::String json = ProjectSerializer::serializeFromEngine(
            audioEngine, projectName, bpm, timeSigNum, timeSigDen);

        if (projectFile.replaceWithText(json))
        {
            markClean();
            clearRecoveryFile();
            notifyProjectSaved();
            return true;
        }

        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::WarningIcon,
            "Save Failed",
            "Could not write to file: " + projectFile.getFullPathName());
        return false;
    }

    // No file yet - use modal file chooser
    // We use a shared pointer and WaitableEvent to make async work synchronously
    auto chooser = std::make_shared<juce::FileChooser>(
        "Save Project",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.progflow");

    bool saveSuccess = false;
    bool dialogCompleted = false;

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    // Capture 'this' for the lambda
    auto* thisPtr = this;

    chooser->launchAsync(chooserFlags, [thisPtr, chooser, &saveSuccess, &dialogCompleted](const juce::FileChooser& fc)
    {
        auto results = fc.getResults();
        if (!results.isEmpty())
        {
            juce::File file = results.getFirst();

            // Ensure .progflow extension
            if (!file.hasFileExtension(".progflow"))
                file = file.withFileExtension(".progflow");

            // Update project name from filename
            thisPtr->projectName = file.getFileNameWithoutExtension();
            thisPtr->projectFile = file;

            // Serialize and write
            juce::String json = ProjectSerializer::serializeFromEngine(
                thisPtr->audioEngine, thisPtr->projectName, thisPtr->bpm, thisPtr->timeSigNum, thisPtr->timeSigDen);

            if (file.replaceWithText(json))
            {
                thisPtr->addToRecentProjects(file);
                thisPtr->markClean();
                thisPtr->clearRecoveryFile();
                thisPtr->notifyProjectSaved();
                saveSuccess = true;
            }
            else
            {
                juce::AlertWindow::showMessageBoxAsync(
                    juce::AlertWindow::WarningIcon,
                    "Save Failed",
                    "Could not write to file: " + file.getFullPathName());
            }
        }
        dialogCompleted = true;
    });

    // Run the message loop until the dialog completes
    // This blocks the current function while keeping the UI responsive
    while (!dialogCompleted)
    {
        juce::MessageManager::getInstance()->runDispatchLoopUntil(10);
    }

    return saveSuccess;
}
