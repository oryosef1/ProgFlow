#include "WelcomeScreen.h"

WelcomeScreen::WelcomeScreen()
{
    // New Project button
    newProjectButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
    newProjectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    newProjectButton.onClick = [this] {
        if (onNewProject)
            onNewProject();
    };
    addAndMakeVisible(newProjectButton);

    // Open Project button
    openProjectButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::bgTertiary());
    openProjectButton.setColour(juce::TextButton::textColourOffId, ProgFlowColours::textPrimary());
    openProjectButton.onClick = [this] {
        if (onOpenProject)
            onOpenProject();
    };
    addAndMakeVisible(openProjectButton);

    // Recent projects label
    recentLabel.setText("Recent Projects", juce::dontSendNotification);
    recentLabel.setColour(juce::Label::textColourId, ProgFlowColours::textSecondary());
    recentLabel.setFont(juce::FontOptions(13.0f).withStyle("Bold"));
    addAndMakeVisible(recentLabel);

    // Recent projects list
    recentList.setModel(this);
    recentList.setColour(juce::ListBox::backgroundColourId, ProgFlowColours::sectionBg());
    recentList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    recentList.setRowHeight(36);
    addAndMakeVisible(recentList);
}

void WelcomeScreen::setRecentProjects(const juce::StringArray& paths)
{
    recentProjects = paths;
    recentList.updateContent();
    recentList.repaint();

    // Show/hide recent section based on whether there are any
    recentLabel.setVisible(!recentProjects.isEmpty());
    recentList.setVisible(!recentProjects.isEmpty());
}

int WelcomeScreen::getNumRows()
{
    return recentProjects.size();
}

void WelcomeScreen::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber < 0 || rowNumber >= recentProjects.size())
        return;

    // Background
    if (rowIsSelected)
        g.fillAll(ProgFlowColours::accentBlue().withAlpha(0.3f));

    juce::File file(recentProjects[rowNumber]);
    juce::String projectName = file.getFileNameWithoutExtension();
    juce::String folderPath = file.getParentDirectory().getFileName();

    // Project name
    g.setColour(ProgFlowColours::textPrimary());
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(projectName, 12, 4, width - 24, 18, juce::Justification::centredLeft);

    // Path hint
    g.setColour(ProgFlowColours::textMuted());
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(folderPath, 12, 20, width - 24, 14, juce::Justification::centredLeft);
}

void WelcomeScreen::listBoxItemClicked(int row, const juce::MouseEvent& /*e*/)
{
    // Single click just selects
    recentList.selectRow(row);
}

void WelcomeScreen::listBoxItemDoubleClicked(int row, const juce::MouseEvent& /*e*/)
{
    if (row >= 0 && row < recentProjects.size() && onOpenRecentProject)
    {
        onOpenRecentProject(recentProjects[row]);
    }
}

void WelcomeScreen::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(ProgFlowColours::bgPrimary());

    auto bounds = getLocalBounds();
    auto centreX = bounds.getCentreX();

    // Logo area (icon) - move up if we have recent projects
    int logoY = recentProjects.isEmpty() ? bounds.getHeight() / 2 - 180 : 60;

    // Draw music note icon in a rounded square
    juce::Rectangle<float> logoBounds(centreX - 40.0f, static_cast<float>(logoY), 80.0f, 80.0f);

    // Gradient background for logo
    juce::ColourGradient gradient(ProgFlowColours::accentBlue(), logoBounds.getX(), logoBounds.getY(),
                                   juce::Colour(0xff9333ea), logoBounds.getRight(), logoBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(logoBounds, 16.0f);

    // Draw music note icon
    g.setColour(juce::Colours::white);
    juce::Path notePath;
    float noteX = centreX - 12.0f;
    float noteY = logoY + 20.0f;
    // Note head
    notePath.addEllipse(noteX - 8, noteY + 28, 16, 12);
    // Note stem
    notePath.addRectangle(noteX + 6, noteY, 3, 32);
    // Note flag
    notePath.startNewSubPath(noteX + 9, noteY);
    notePath.quadraticTo(noteX + 20, noteY + 8, noteX + 16, noteY + 20);
    g.fillPath(notePath);

    // Title
    g.setColour(ProgFlowColours::textPrimary());
    g.setFont(juce::FontOptions(28.0f).withStyle("Bold"));
    g.drawText("ProgFlow", bounds.withY(logoY + 100).withHeight(40), juce::Justification::centredTop);

    // Subtitle
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("Music production made simple", bounds.withY(logoY + 135).withHeight(30), juce::Justification::centredTop);

    // Keyboard shortcuts at bottom
    g.setColour(ProgFlowColours::textDisabled());
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Cmd+S Save  |  Space Play/Pause  |  Cmd+Z Undo",
               bounds.withY(bounds.getHeight() - 50).withHeight(30),
               juce::Justification::centredTop);
}

void WelcomeScreen::resized()
{
    auto bounds = getLocalBounds();
    auto centreX = bounds.getCentreX();

    int buttonWidth = 180;
    int buttonHeight = 44;
    int gap = 16;

    if (recentProjects.isEmpty())
    {
        // Original centered layout
        auto buttonY = bounds.getHeight() / 2;
        newProjectButton.setBounds(centreX - buttonWidth - gap/2, buttonY, buttonWidth, buttonHeight);
        openProjectButton.setBounds(centreX + gap/2, buttonY, buttonWidth, buttonHeight);

        recentLabel.setBounds(0, 0, 0, 0);
        recentList.setBounds(0, 0, 0, 0);
    }
    else
    {
        // Layout with recent projects
        int logoOffset = 60 + 80 + 100 + 30; // logo + title + subtitle space
        int buttonY = logoOffset + 30;

        newProjectButton.setBounds(centreX - buttonWidth - gap/2, buttonY, buttonWidth, buttonHeight);
        openProjectButton.setBounds(centreX + gap/2, buttonY, buttonWidth, buttonHeight);

        // Recent projects section
        int recentY = buttonY + buttonHeight + 40;
        int listWidth = juce::jmin(400, bounds.getWidth() - 80);
        int listHeight = juce::jmin(static_cast<int>(recentProjects.size()) * 36 + 4, 220);

        recentLabel.setBounds(centreX - listWidth/2, recentY, listWidth, 20);
        recentList.setBounds(centreX - listWidth/2, recentY + 26, listWidth, listHeight);
    }
}
