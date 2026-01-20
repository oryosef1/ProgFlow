#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_core/juce_core.h>
#include "ProjectSerializer.h"
#include "../Audio/AudioEngine.h"

/**
 * ProjectManager - Handles project file operations
 *
 * Responsibilities:
 * - Save/Load projects (.progflow files)
 * - Native file dialogs (open, save, save as)
 * - Recent projects list (stored in app preferences)
 * - Autosave with crash recovery
 * - Dirty state tracking
 *
 * Uses JUCE ApplicationProperties for persistent storage.
 */
class ProjectManager : public juce::Timer
{
public:
    ProjectManager(AudioEngine& engine);
    ~ProjectManager() override;

    //==========================================================================
    // Project state
    const juce::String& getProjectName() const { return projectName; }
    void setProjectName(const juce::String& name);

    juce::File getProjectFile() const { return projectFile; }
    bool hasProjectFile() const { return projectFile.existsAsFile(); }

    bool isDirty() const { return dirty; }
    void markDirty();
    void markClean();

    double getBpm() const { return bpm; }
    void setBpm(double newBpm);

    int getTimeSignatureNum() const { return timeSigNum; }
    int getTimeSignatureDen() const { return timeSigDen; }
    void setTimeSignature(int num, int den);

    //==========================================================================
    // File operations
    bool newProject();
    bool saveProject();
    bool saveProjectAs();
    bool saveProjectSync();  // Synchronous save (blocks until complete)
    bool openProject();
    bool openProject(const juce::File& file);

    //==========================================================================
    // Recent projects
    juce::StringArray getRecentProjects() const;
    void clearRecentProjects();

    //==========================================================================
    // Autosave
    void setAutosaveEnabled(bool enabled);
    bool isAutosaveEnabled() const { return autosaveEnabled; }
    void setAutosaveIntervalMinutes(int minutes);
    int getAutosaveIntervalMinutes() const { return autosaveIntervalMinutes; }

    // Check for recovery file on startup
    bool hasRecoveryFile() const;
    bool recoverFromAutosave();
    void clearRecoveryFile();

    //==========================================================================
    // Listeners for UI updates
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void projectStateChanged() {}
        virtual void projectLoaded() {}
        virtual void projectSaved() {}
    };

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    AudioEngine& audioEngine;

    // Project state
    juce::String projectName = "Untitled";
    juce::File projectFile;
    bool dirty = false;
    double bpm = 120.0;
    int timeSigNum = 4;
    int timeSigDen = 4;

    // Recent projects (managed through ApplicationProperties)
    juce::ApplicationProperties appProperties;
    juce::RecentlyOpenedFilesList recentFiles;
    static constexpr int MAX_RECENT_FILES = 10;

    // Autosave
    bool autosaveEnabled = true;
    int autosaveIntervalMinutes = 2;
    juce::int64 lastSaveTime = 0;

    // Listeners
    juce::ListenerList<Listener> listeners;

    //==========================================================================
    // Timer callback for autosave
    void timerCallback() override;

    //==========================================================================
    // Internal helpers
    void addToRecentProjects(const juce::File& file);
    juce::File getAutosaveFile() const;
    juce::File getAutosaveDirectory() const;
    void performAutosave();
    void notifyStateChanged();
    void notifyProjectLoaded();
    void notifyProjectSaved();
    void initializeProperties();

    // Show "unsaved changes" dialog - returns true if user wants to proceed
    bool checkUnsavedChanges();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};
