#include "WaveSelector.h"

WaveSelector::WaveSelector()
{
    setWantsKeyboardFocus(false);
}

void WaveSelector::paint(juce::Graphics& g)
{
    const int numWaves = 4;
    const float buttonWidth = static_cast<float>(getWidth()) / numWaves;
    const float buttonHeight = static_cast<float>(getHeight());
    const float cornerRadius = 4.0f;
    const float gap = 2.0f;

    for (int i = 0; i < numWaves; ++i)
    {
        juce::Rectangle<float> buttonBounds(
            i * buttonWidth + gap / 2,
            gap / 2,
            buttonWidth - gap,
            buttonHeight - gap);

        // Background
        if (i == selectedIndex)
        {
            g.setColour(ProgFlowColours::accentBlue());
        }
        else
        {
            g.setColour(ProgFlowColours::bgTertiary());
        }
        g.fillRoundedRectangle(buttonBounds, cornerRadius);

        // Draw wave icon
        juce::Colour iconColour = (i == selectedIndex) ? juce::Colours::white : ProgFlowColours::textSecondary();
        g.setColour(iconColour);

        auto iconBounds = buttonBounds.reduced(6, 6);
        juce::Path wavePath;

        switch (i)
        {
            case 0: // Sine wave
            {
                float startX = iconBounds.getX();
                float midY = iconBounds.getCentreY();
                float amp = iconBounds.getHeight() * 0.4f;
                wavePath.startNewSubPath(startX, midY);
                for (float x = 0; x <= iconBounds.getWidth(); x += 2)
                {
                    float phase = (x / iconBounds.getWidth()) * juce::MathConstants<float>::twoPi;
                    float y = midY - std::sin(phase) * amp;
                    wavePath.lineTo(startX + x, y);
                }
                g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                break;
            }
            case 1: // Triangle wave
            {
                float y1 = iconBounds.getBottom();
                float y2 = iconBounds.getY();
                float w = iconBounds.getWidth() / 4;
                wavePath.startNewSubPath(iconBounds.getX(), iconBounds.getCentreY());
                wavePath.lineTo(iconBounds.getX() + w, y2);
                wavePath.lineTo(iconBounds.getX() + w * 3, y1);
                wavePath.lineTo(iconBounds.getRight(), iconBounds.getCentreY());
                g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                break;
            }
            case 2: // Sawtooth wave
            {
                float y1 = iconBounds.getBottom();
                float y2 = iconBounds.getY();
                wavePath.startNewSubPath(iconBounds.getX(), y1);
                wavePath.lineTo(iconBounds.getCentreX(), y2);
                wavePath.lineTo(iconBounds.getCentreX(), y1);
                wavePath.lineTo(iconBounds.getRight(), y2);
                g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                break;
            }
            case 3: // Square wave
            {
                float y1 = iconBounds.getBottom();
                float y2 = iconBounds.getY();
                float w = iconBounds.getWidth() / 4;
                wavePath.startNewSubPath(iconBounds.getX(), y1);
                wavePath.lineTo(iconBounds.getX(), y2);
                wavePath.lineTo(iconBounds.getX() + w * 2, y2);
                wavePath.lineTo(iconBounds.getX() + w * 2, y1);
                wavePath.lineTo(iconBounds.getRight(), y1);
                g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                break;
            }
        }
    }
}

void WaveSelector::resized()
{
    repaint();
}

void WaveSelector::mouseDown(const juce::MouseEvent& e)
{
    int index = getIndexAtPosition(e.x);
    if (index >= 0 && index < 4)
    {
        setSelectedIndex(index);
    }
}

void WaveSelector::setSelectedIndex(int index, juce::NotificationType notification)
{
    index = juce::jlimit(0, 3, index);

    if (index != selectedIndex)
    {
        selectedIndex = index;
        repaint();

        if (notification != juce::dontSendNotification && onSelectionChanged)
        {
            onSelectionChanged(selectedIndex);
        }
    }
}

int WaveSelector::getIndexAtPosition(int x) const
{
    const int numWaves = 4;
    const int buttonWidth = getWidth() / numWaves;
    return x / buttonWidth;
}
