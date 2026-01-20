#include "VerticalMeter.h"

VerticalMeter::VerticalMeter()
{
    startTimerHz(60);  // 60fps updates
}

VerticalMeter::~VerticalMeter()
{
    stopTimer();
}

void VerticalMeter::setLevel(float level)
{
    currentLevel = level;

    // Update peak
    if (level > peakLevel)
    {
        peakLevel = level;
        peakHoldCounter = peakHoldTime;
    }
}

void VerticalMeter::timerCallback()
{
    // Smooth level display with decay
    if (currentLevel > displayLevel)
    {
        displayLevel = currentLevel;
    }
    else
    {
        displayLevel = displayLevel * decaySpeed + currentLevel * (1.0f - decaySpeed);
    }

    // Peak hold and decay
    if (peakHoldCounter > 0)
    {
        peakHoldCounter--;
    }
    else
    {
        peakLevel *= peakDecay;
        if (peakLevel < 0.001f)
            peakLevel = 0.0f;
    }

    repaint();
}

void VerticalMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float cornerSize = 2.0f;

    // Background
    g.setColour(ProgFlowColours::bgPrimary());
    g.fillRoundedRectangle(bounds, cornerSize);

    // Draw border
    g.setColour(ProgFlowColours::border());
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

    // Calculate meter height
    auto meterBounds = bounds.reduced(2.0f);
    float meterHeight = meterBounds.getHeight();

    // Convert level to height (with some dB scaling for visual appeal)
    // Level 0.0 = -∞ dB, Level 1.0 = 0 dB, Level 2.0 = +6 dB
    float displayDb = (displayLevel > 0.0f)
        ? 20.0f * std::log10(displayLevel)
        : -60.0f;
    displayDb = juce::jlimit(-60.0f, 6.0f, displayDb);
    float normalizedLevel = (displayDb + 60.0f) / 66.0f;  // Map -60 to +6 dB to 0-1
    float levelHeight = meterHeight * normalizedLevel;

    // Draw meter segments with gradient
    if (levelHeight > 0.0f)
    {
        // Green zone (bottom, up to -12 dB = 48/66 = ~73% from bottom)
        float greenZoneHeight = meterHeight * (48.0f / 66.0f);
        float greenHeight = std::min(levelHeight, greenZoneHeight);

        if (greenHeight > 0.0f)
        {
            g.setColour(ProgFlowColours::accentGreen());
            g.fillRect(meterBounds.getX(),
                       meterBounds.getBottom() - greenHeight,
                       meterBounds.getWidth(),
                       greenHeight);
        }

        // Yellow zone (-12 to -6 dB = 48 to 54/66 = ~73% to ~82%)
        if (levelHeight > greenZoneHeight)
        {
            float yellowZoneHeight = meterHeight * (6.0f / 66.0f);  // 6 dB range
            float yellowHeight = std::min(levelHeight - greenZoneHeight, yellowZoneHeight);

            g.setColour(juce::Colour(0xfffbbf24));  // Yellow
            g.fillRect(meterBounds.getX(),
                       meterBounds.getBottom() - greenZoneHeight - yellowHeight,
                       meterBounds.getWidth(),
                       yellowHeight);
        }

        // Red zone (-6 dB to +6 dB = 54 to 66/66 = ~82% to 100%)
        float yellowEnd = meterHeight * (54.0f / 66.0f);
        if (levelHeight > yellowEnd)
        {
            float redHeight = levelHeight - yellowEnd;
            g.setColour(ProgFlowColours::accentRed());
            g.fillRect(meterBounds.getX(),
                       meterBounds.getBottom() - levelHeight,
                       meterBounds.getWidth(),
                       redHeight);
        }
    }

    // Draw peak indicator
    if (showPeak && peakLevel > 0.001f)
    {
        float peakDb = 20.0f * std::log10(peakLevel);
        peakDb = juce::jlimit(-60.0f, 6.0f, peakDb);
        float peakNormalized = (peakDb + 60.0f) / 66.0f;
        float peakY = meterBounds.getBottom() - (meterHeight * peakNormalized);

        // Choose peak color based on level
        if (peakDb > -6.0f)
            g.setColour(ProgFlowColours::accentRed());
        else if (peakDb > -12.0f)
            g.setColour(juce::Colour(0xfffbbf24));
        else
            g.setColour(ProgFlowColours::accentGreen());

        g.fillRect(meterBounds.getX(), peakY - 2.0f, meterBounds.getWidth(), 2.0f);
    }
}

void VerticalMeter::resized()
{
    // Nothing specific needed here
}
