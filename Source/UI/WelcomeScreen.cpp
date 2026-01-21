#include "WelcomeScreen.h"

WelcomeScreen::WelcomeScreen()
    : rng(std::random_device{}())
{
    // New Project button - primary action with gradient-like effect
    newProjectButton.setColour(juce::TextButton::buttonColourId, ProgFlowColours::accentBlue());
    newProjectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    newProjectButton.onClick = [this] {
        if (onNewProject)
            onNewProject();
    };
    addAndMakeVisible(newProjectButton);

    // Open Project button - secondary action
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

    // Recent projects list with modern styling
    recentList.setModel(this);
    recentList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    recentList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    recentList.setRowHeight(48);
    addAndMakeVisible(recentList);

    // Initialize particle system
    initParticles();

    // Start animation timer (30fps for smooth particles)
    startTimerHz(30);
}

WelcomeScreen::~WelcomeScreen()
{
    stopTimer();
}

void WelcomeScreen::initParticles()
{
    particles.clear();
    std::uniform_real_distribution<float> xDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> yDist(0.0f, 1.0f);
    std::uniform_real_distribution<float> vDist(-0.001f, 0.001f);
    std::uniform_real_distribution<float> sizeDist(2.0f, 6.0f);
    std::uniform_real_distribution<float> alphaDist(0.1f, 0.4f);

    for (int i = 0; i < 50; ++i)
    {
        Particle p;
        p.x = xDist(rng);
        p.y = yDist(rng);
        p.vx = vDist(rng);
        p.vy = vDist(rng) - 0.0005f; // Slight upward drift
        p.size = sizeDist(rng);
        p.alpha = alphaDist(rng);
        particles.push_back(p);
    }
}

void WelcomeScreen::updateParticles()
{
    animationTime += 0.033f; // ~30fps

    for (auto& p : particles)
    {
        p.x += p.vx;
        p.y += p.vy;

        // Wrap around
        if (p.x < 0.0f) p.x += 1.0f;
        if (p.x > 1.0f) p.x -= 1.0f;
        if (p.y < 0.0f) p.y += 1.0f;
        if (p.y > 1.0f) p.y -= 1.0f;

        // Subtle alpha pulsing
        p.alpha = 0.15f + 0.15f * std::sin(animationTime * 2.0f + p.x * 10.0f);
    }
}

void WelcomeScreen::timerCallback()
{
    updateParticles();
    repaint();
}

void WelcomeScreen::drawParticles(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    for (const auto& p : particles)
    {
        float x = p.x * bounds.getWidth();
        float y = p.y * bounds.getHeight();

        // Draw particle with glow effect
        juce::ColourGradient glow(
            ProgFlowColours::accentBlue().withAlpha(p.alpha),
            x, y,
            ProgFlowColours::accentBlue().withAlpha(0.0f),
            x + p.size * 3.0f, y,
            true);

        g.setGradientFill(glow);
        g.fillEllipse(x - p.size, y - p.size, p.size * 2.0f, p.size * 2.0f);
    }
}

void WelcomeScreen::drawGlowingLogo(juce::Graphics& g, float centreX, float logoY)
{
    juce::Rectangle<float> logoBounds(centreX - 50.0f, logoY, 100.0f, 100.0f);

    // Outer glow
    for (int i = 4; i > 0; --i)
    {
        float expand = static_cast<float>(i) * 8.0f;
        float alpha = 0.05f * (5 - i);
        g.setColour(ProgFlowColours::accentBlue().withAlpha(alpha));
        g.fillRoundedRectangle(logoBounds.expanded(expand), 20.0f + expand * 0.3f);
    }

    // Main logo background with animated gradient
    float gradientOffset = std::sin(animationTime * 0.5f) * 0.2f;
    juce::ColourGradient gradient(
        ProgFlowColours::accentBlue(),
        logoBounds.getX(), logoBounds.getY() + logoBounds.getHeight() * gradientOffset,
        juce::Colour(0xff9333ea), // Purple
        logoBounds.getRight(), logoBounds.getBottom(),
        false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(logoBounds, 20.0f);

    // Inner highlight
    g.setColour(juce::Colour(0x20ffffff));
    g.fillRoundedRectangle(logoBounds.reduced(2).withTrimmedBottom(logoBounds.getHeight() * 0.5f), 18.0f);

    // Music note icon
    g.setColour(juce::Colours::white);
    juce::Path notePath;
    float noteX = centreX - 15.0f;
    float noteY = logoY + 22.0f;
    // Note head (ellipse)
    notePath.addEllipse(noteX - 10, noteY + 36, 20, 14);
    // Note stem
    notePath.addRectangle(noteX + 8, noteY, 4, 42);
    // Note flag (beam)
    notePath.startNewSubPath(noteX + 12, noteY);
    notePath.quadraticTo(noteX + 28, noteY + 12, noteX + 22, noteY + 28);
    notePath.quadraticTo(noteX + 26, noteY + 18, noteX + 12, noteY + 14);
    notePath.closeSubPath();
    g.fillPath(notePath);
}

void WelcomeScreen::drawWaveform(juce::Graphics& g, juce::Rectangle<float> bounds)
{
    // Draw animated waveform visualization
    juce::Path wavePath;
    wavePath.startNewSubPath(bounds.getX(), bounds.getCentreY());

    int numPoints = 100;
    for (int i = 0; i <= numPoints; ++i)
    {
        float x = bounds.getX() + (bounds.getWidth() * i / numPoints);
        float normalizedX = static_cast<float>(i) / numPoints;

        // Combine multiple sine waves for organic look
        float y = bounds.getCentreY();
        y += std::sin(normalizedX * 8.0f + animationTime * 1.5f) * 15.0f;
        y += std::sin(normalizedX * 12.0f - animationTime * 2.0f) * 8.0f;
        y += std::sin(normalizedX * 20.0f + animationTime * 0.8f) * 5.0f;

        // Fade at edges
        float edgeFade = std::min(normalizedX, 1.0f - normalizedX) * 4.0f;
        edgeFade = juce::jmin(1.0f, edgeFade);
        y = bounds.getCentreY() + (y - bounds.getCentreY()) * edgeFade;

        if (i == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }

    // Draw waveform with glow
    g.setColour(ProgFlowColours::accentBlue().withAlpha(0.1f));
    g.strokePath(wavePath, juce::PathStrokeType(8.0f));
    g.setColour(ProgFlowColours::accentBlue().withAlpha(0.3f));
    g.strokePath(wavePath, juce::PathStrokeType(3.0f));
    g.setColour(ProgFlowColours::accentBlue().withAlpha(0.6f));
    g.strokePath(wavePath, juce::PathStrokeType(1.5f));
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

    // Card background
    auto itemBounds = juce::Rectangle<float>(4.0f, 2.0f, width - 8.0f, height - 4.0f);

    if (rowIsSelected)
    {
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.2f));
        g.fillRoundedRectangle(itemBounds, 8.0f);
        g.setColour(ProgFlowColours::accentBlue().withAlpha(0.5f));
        g.drawRoundedRectangle(itemBounds, 8.0f, 1.0f);
    }
    else
    {
        g.setColour(ProgFlowColours::bgTertiary().withAlpha(0.5f));
        g.fillRoundedRectangle(itemBounds, 8.0f);
    }

    juce::File file(recentProjects[rowNumber]);
    juce::String projectName = file.getFileNameWithoutExtension();
    juce::String folderPath = file.getParentDirectory().getFullPathName();

    // Project icon
    g.setColour(ProgFlowColours::accentBlue());
    g.fillRoundedRectangle(16.0f, (height - 28.0f) / 2.0f, 28.0f, 28.0f, 6.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(projectName.substring(0, 1).toUpperCase(), 16, (height - 28) / 2, 28, 28, juce::Justification::centred);

    // Project name
    g.setColour(ProgFlowColours::textPrimary());
    g.setFont(juce::FontOptions(14.0f).withStyle("Bold"));
    g.drawText(projectName, 54, 8, width - 70, 20, juce::Justification::centredLeft);

    // Path hint
    g.setColour(ProgFlowColours::textMuted());
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(folderPath, 54, 26, width - 70, 16, juce::Justification::centredLeft);
}

void WelcomeScreen::listBoxItemClicked(int row, const juce::MouseEvent& /*e*/)
{
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
    auto bounds = getLocalBounds();
    auto centreX = static_cast<float>(bounds.getCentreX());

    // Background gradient (dark with subtle color tint)
    juce::ColourGradient bgGradient(
        ProgFlowColours::bgPrimary(),
        0.0f, 0.0f,
        juce::Colour(0xff0a0f1a), // Slightly blue-tinted dark
        0.0f, static_cast<float>(bounds.getHeight()),
        false);
    g.setGradientFill(bgGradient);
    g.fillRect(bounds);

    // Draw animated particles
    drawParticles(g);

    // Draw waveform visualization at bottom
    auto waveformBounds = bounds.toFloat().withTrimmedTop(bounds.getHeight() * 0.7f);
    drawWaveform(g, waveformBounds);

    // Logo area
    float logoY = recentProjects.isEmpty() ? bounds.getHeight() / 2.0f - 200.0f : 50.0f;
    drawGlowingLogo(g, centreX, logoY);

    // Title with glow
    float titleY = logoY + 120.0f;
    g.setColour(ProgFlowColours::accentBlue().withAlpha(0.3f));
    g.setFont(juce::FontOptions(32.0f).withStyle("Bold"));
    g.drawText("ProgFlow", bounds.withY(static_cast<int>(titleY) + 2).withHeight(40), juce::Justification::centredTop);
    g.setColour(ProgFlowColours::textPrimary());
    g.drawText("ProgFlow", bounds.withY(static_cast<int>(titleY)).withHeight(40), juce::Justification::centredTop);

    // Subtitle
    g.setColour(ProgFlowColours::textSecondary());
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("Professional Music Production", bounds.withY(static_cast<int>(titleY) + 40).withHeight(30), juce::Justification::centredTop);

    // Version badge
    g.setColour(ProgFlowColours::bgTertiary());
    auto versionBounds = juce::Rectangle<float>(centreX - 30.0f, titleY + 70.0f, 60.0f, 20.0f);
    g.fillRoundedRectangle(versionBounds, 10.0f);
    g.setColour(ProgFlowColours::textMuted());
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("v1.0", versionBounds.toNearestInt(), juce::Justification::centred);

    // Keyboard shortcuts at bottom
    g.setColour(ProgFlowColours::textDisabled());
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("Cmd+N New  |  Cmd+O Open  |  Space Play  |  Cmd+S Save",
               bounds.withY(bounds.getHeight() - 40).withHeight(30),
               juce::Justification::centredTop);
}

void WelcomeScreen::resized()
{
    auto bounds = getLocalBounds();
    auto centreX = bounds.getCentreX();

    int buttonWidth = 180;
    int buttonHeight = 48;
    int gap = 20;

    // Calculate logo position
    float logoY = recentProjects.isEmpty() ? bounds.getHeight() / 2.0f - 200.0f : 50.0f;
    int buttonY = static_cast<int>(logoY) + 200;

    // Buttons
    newProjectButton.setBounds(centreX - buttonWidth - gap/2, buttonY, buttonWidth, buttonHeight);
    openProjectButton.setBounds(centreX + gap/2, buttonY, buttonWidth, buttonHeight);

    if (recentProjects.isEmpty())
    {
        recentLabel.setBounds(0, 0, 0, 0);
        recentList.setBounds(0, 0, 0, 0);
    }
    else
    {
        // Recent projects section
        int recentY = buttonY + buttonHeight + 50;
        int listWidth = juce::jmin(500, bounds.getWidth() - 80);
        int listHeight = juce::jmin(static_cast<int>(recentProjects.size()) * 52 + 8, 280);

        recentLabel.setBounds(centreX - listWidth/2, recentY, listWidth, 20);
        recentList.setBounds(centreX - listWidth/2, recentY + 28, listWidth, listHeight);
    }
}
