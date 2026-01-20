#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"

/**
 * WelcomeScreen - Initial screen shown when app starts
 *
 * Shows:
 * - App logo and title
 * - Create New Project button
 * - Open Project button
 * - Recent projects list
 */
class WelcomeScreen : public juce::Component,
                      public juce::ListBoxModel
{
public:
    WelcomeScreen();

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Set recent projects to display
    void setRecentProjects(const juce::StringArray& paths);

    // ListBoxModel overrides
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;

    // Callbacks
    std::function<void()> onNewProject;
    std::function<void()> onOpenProject;
    std::function<void(const juce::String& path)> onOpenRecentProject;

private:
    juce::TextButton newProjectButton{"Create New Project"};
    juce::TextButton openProjectButton{"Open Project..."};

    juce::Label recentLabel;
    juce::ListBox recentList;
    juce::StringArray recentProjects;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WelcomeScreen)
};
