#include "EnvelopeVisualizer.h"

EnvelopeVisualizer::EnvelopeVisualizer()
{
}

void EnvelopeVisualizer::setAttack(float value)
{
    attack = juce::jlimit(0.0f, 1.0f, value);
    repaint();
}

void EnvelopeVisualizer::setDecay(float value)
{
    decay = juce::jlimit(0.0f, 1.0f, value);
    repaint();
}

void EnvelopeVisualizer::setSustain(float value)
{
    sustain = juce::jlimit(0.0f, 1.0f, value);
    repaint();
}

void EnvelopeVisualizer::setRelease(float value)
{
    release = juce::jlimit(0.0f, 1.0f, value);
    repaint();
}

void EnvelopeVisualizer::setADSR(float a, float d, float s, float r)
{
    attack = juce::jlimit(0.0f, 1.0f, a);
    decay = juce::jlimit(0.0f, 1.0f, d);
    sustain = juce::jlimit(0.0f, 1.0f, s);
    release = juce::jlimit(0.0f, 1.0f, r);
    repaint();
}

void EnvelopeVisualizer::setAccentColour(juce::Colour colour)
{
    accentColour = colour;
    useCustomAccent = true;
    repaint();
}

void EnvelopeVisualizer::setBackgroundColour(juce::Colour colour)
{
    backgroundColour = colour;
    useCustomBg = true;
    repaint();
}

void EnvelopeVisualizer::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    // Background
    auto bgColour = useCustomBg ? backgroundColour : ProgFlowColours::sectionBg();
    g.setColour(bgColour);
    g.fillRoundedRectangle(bounds, 4.0f);

    // Draw envelope
    auto graphBounds = bounds.reduced(4.0f);

    // Build and draw the filled envelope
    juce::Path envelopePath = buildEnvelopePath(graphBounds);

    // Fill with semi-transparent accent
    auto accent = useCustomAccent ? accentColour : ProgFlowColours::accentBlue();
    g.setColour(accent.withAlpha(0.2f));
    g.fillPath(envelopePath);

    // Stroke with accent colour
    g.setColour(accent);
    g.strokePath(envelopePath, juce::PathStrokeType(1.5f));

    // Draw baseline
    g.setColour(ProgFlowColours::dividerLine());
    g.drawHorizontalLine(static_cast<int>(graphBounds.getBottom()),
                         graphBounds.getX(), graphBounds.getRight());
}

juce::Path EnvelopeVisualizer::buildEnvelopePath(juce::Rectangle<float> bounds) const
{
    juce::Path path;

    float x = bounds.getX();
    float y = bounds.getY();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float bottom = y + h;

    // Allocate width for each segment
    // Attack: 25%, Decay: 25%, Sustain: 25%, Release: 25% (at most)
    // Scale by actual values
    float totalTime = attack + decay + 0.3f + release;  // 0.3 is fixed sustain display time
    float attackW = (attack / totalTime) * w;
    float decayW = (decay / totalTime) * w;
    float sustainW = (0.3f / totalTime) * w;
    float releaseW = (release / totalTime) * w;

    // Start at bottom left
    path.startNewSubPath(x, bottom);

    // Attack: rise from 0 to peak (1.0)
    float attackEndX = x + attackW;
    float peakY = y;  // Top of bounds = peak
    path.lineTo(attackEndX, peakY);

    // Decay: fall from peak to sustain level
    float decayEndX = attackEndX + decayW;
    float sustainY = y + (1.0f - sustain) * h;
    path.lineTo(decayEndX, sustainY);

    // Sustain: hold at sustain level
    float sustainEndX = decayEndX + sustainW;
    path.lineTo(sustainEndX, sustainY);

    // Release: fall from sustain to 0
    float releaseEndX = sustainEndX + releaseW;
    path.lineTo(releaseEndX, bottom);

    // Close the path for fill
    path.lineTo(x, bottom);
    path.closeSubPath();

    return path;
}
