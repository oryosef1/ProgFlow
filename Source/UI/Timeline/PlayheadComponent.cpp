#include "PlayheadComponent.h"
#include "../LookAndFeel.h"

PlayheadComponent::PlayheadComponent()
{
    setInterceptsMouseClicks(false, false);
    setOpaque(false);
}

void PlayheadComponent::setPosition(int xPosition)
{
    if (xPos != xPosition)
    {
        xPos = xPosition;
        repaint();
    }
}

void PlayheadComponent::paint(juce::Graphics& g)
{
    // Draw playhead line
    g.setColour(ProgFlowColours::accentRed());
    g.drawVerticalLine(xPos, 0.0f, static_cast<float>(getHeight()));

    // Draw small triangle at top
    juce::Path triangle;
    triangle.addTriangle(
        static_cast<float>(xPos - 6), 0.0f,
        static_cast<float>(xPos + 6), 0.0f,
        static_cast<float>(xPos), 8.0f
    );
    g.fillPath(triangle);
}
