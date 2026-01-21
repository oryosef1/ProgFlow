#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "LookAndFeel.h"
#include <random>

/**
 * WelcomeScreen - Initial screen shown when app starts
 *
 * Features:
 * - Animated background with floating particles
 * - Gradient backdrop
 * - Modern card-based recent projects
 * - Smooth hover effects on buttons
 */
class WelcomeScreen : public juce::Component,
                      public juce::ListBoxModel,
                      private juce::Timer
{
public:
    WelcomeScreen();
    ~WelcomeScreen() override;

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
    void timerCallback() override;

    juce::TextButton newProjectButton{"+ New Project"};
    juce::TextButton openProjectButton{"Open Project..."};

    juce::Label recentLabel;
    juce::ListBox recentList;
    juce::StringArray recentProjects;

    // Animated particles
    struct Particle
    {
        float x, y;
        float vx, vy;
        float size;
        float alpha;
    };
    std::vector<Particle> particles;
    std::mt19937 rng;
    float animationTime = 0.0f;

    void initParticles();
    void updateParticles();
    void drawParticles(juce::Graphics& g);
    void drawGlowingLogo(juce::Graphics& g, float centreX, float logoY);
    void drawWaveform(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WelcomeScreen)
};
