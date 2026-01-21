#include "TimeRuler.h"
#include "../LookAndFeel.h"

TimeRuler::TimeRuler()
{
}

void TimeRuler::setBarWidth(int width)
{
    barWidth = std::max(20, width);
    repaint();
}

void TimeRuler::setScrollOffset(double offset)
{
    scrollOffset = std::max(0.0, offset);
    repaint();
}

void TimeRuler::setTimeSignature(int numerator, int denominator)
{
    timeSignatureNumerator = numerator;
    timeSignatureDenominator = denominator;
    repaint();
}

void TimeRuler::setLoopRegion(double startBar, double endBar)
{
    loopStartBar = startBar;
    loopEndBar = endBar;
    repaint();
}

void TimeRuler::setLoopEnabled(bool enabled)
{
    loopEnabled = enabled;
    repaint();
}

void TimeRuler::setMarkerTrack(MarkerTrack* track)
{
    markerTrack = track;
    repaint();
}

void TimeRuler::setBpm(double bpm)
{
    currentBpm = bpm;
}

void TimeRuler::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Saturn design: subtle gradient background
    juce::ColourGradient gradient(
        ProgFlowColours::bgSecondary(),
        0.0f, 0.0f,
        ProgFlowColours::bgPrimary(),
        0.0f, static_cast<float>(bounds.getHeight()),
        false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Draw loop region if enabled (behind everything else)
    if (loopEnabled)
    {
        drawLoopRegion(g);
    }

    // Bottom border (Saturn accent)
    g.setColour(ProgFlowColours::border());
    g.drawHorizontalLine(bounds.getHeight() - 1, 0.0f, static_cast<float>(bounds.getWidth()));

    // Calculate visible bar range
    int firstVisibleBar = static_cast<int>(scrollOffset);
    int lastVisibleBar = firstVisibleBar + (bounds.getWidth() / barWidth) + 2;

    // Draw bar markers
    g.setFont(11.0f);

    for (int bar = firstVisibleBar; bar <= lastVisibleBar; ++bar)
    {
        // Calculate x position for this bar
        double barOffset = bar - scrollOffset;
        int x = static_cast<int>(barOffset * barWidth);

        if (x < -barWidth || x > bounds.getWidth())
            continue;

        // Draw bar number and major tick
        g.setColour(ProgFlowColours::textPrimary());
        g.drawText(juce::String(bar + 1), x + 4, 2, 40, 14, juce::Justification::left);

        g.setColour(ProgFlowColours::textSecondary().withAlpha(0.5f));
        g.drawVerticalLine(x, static_cast<float>(bounds.getHeight() - 10),
                          static_cast<float>(bounds.getHeight()));

        // Draw beat subdivisions
        int beatsPerBar = timeSignatureNumerator;
        double beatWidth = static_cast<double>(barWidth) / beatsPerBar;

        for (int beat = 1; beat < beatsPerBar; ++beat)
        {
            int beatX = x + static_cast<int>(beat * beatWidth);
            g.setColour(ProgFlowColours::textSecondary().withAlpha(0.3f));
            g.drawVerticalLine(beatX, static_cast<float>(bounds.getHeight() - 6),
                              static_cast<float>(bounds.getHeight()));
        }
    }

    // Draw markers on top
    drawMarkers(g);
}

void TimeRuler::mouseDown(const juce::MouseEvent& e)
{
    if (onSeek)
    {
        double bar = xToBar(e.x);
        onSeek(bar);
    }
}

void TimeRuler::mouseDrag(const juce::MouseEvent& e)
{
    if (onSeek)
    {
        double bar = xToBar(e.x);
        onSeek(bar);
    }
}

void TimeRuler::mouseDoubleClick(const juce::MouseEvent& e)
{
    // Double-click adds a marker at this position
    if (onMarkerAdd)
    {
        double beat = xToBeat(e.x);
        onMarkerAdd(beat);
    }
}

double TimeRuler::xToBar(int x) const
{
    return scrollOffset + (static_cast<double>(x) / barWidth);
}

double TimeRuler::xToBeat(int x) const
{
    double bar = xToBar(x);
    // Convert bars to beats (beats per bar = time signature numerator)
    return bar * timeSignatureNumerator;
}

void TimeRuler::drawLoopRegion(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Calculate pixel positions for loop start and end
    double startOffset = loopStartBar - scrollOffset;
    double endOffset = loopEndBar - scrollOffset;

    int startX = static_cast<int>(startOffset * barWidth);
    int endX = static_cast<int>(endOffset * barWidth);

    // Clamp to visible area
    startX = std::max(0, startX);
    endX = std::min(bounds.getWidth(), endX);

    if (endX <= startX) return;  // Loop region not visible

    // Draw loop region background (Saturn: gold/orange accent)
    g.setColour(ProgFlowColours::accentOrange().withAlpha(0.2f));
    g.fillRect(startX, 0, endX - startX, bounds.getHeight());

    // Draw loop markers (vertical lines at start and end)
    g.setColour(ProgFlowColours::accentOrange());

    // Start marker
    int actualStartX = static_cast<int>((loopStartBar - scrollOffset) * barWidth);
    if (actualStartX >= 0 && actualStartX < bounds.getWidth())
    {
        g.drawVerticalLine(actualStartX, 0.0f, static_cast<float>(bounds.getHeight()));
        // Draw small triangle marker at top
        juce::Path startMarker;
        startMarker.addTriangle(
            static_cast<float>(actualStartX), 0.0f,
            static_cast<float>(actualStartX + 8), 0.0f,
            static_cast<float>(actualStartX), 8.0f
        );
        g.fillPath(startMarker);
    }

    // End marker
    int actualEndX = static_cast<int>((loopEndBar - scrollOffset) * barWidth);
    if (actualEndX >= 0 && actualEndX <= bounds.getWidth())
    {
        g.drawVerticalLine(actualEndX, 0.0f, static_cast<float>(bounds.getHeight()));
        // Draw small triangle marker at top (pointing left)
        juce::Path endMarker;
        endMarker.addTriangle(
            static_cast<float>(actualEndX), 0.0f,
            static_cast<float>(actualEndX - 8), 0.0f,
            static_cast<float>(actualEndX), 8.0f
        );
        g.fillPath(endMarker);
    }
}

void TimeRuler::drawMarkers(juce::Graphics& g)
{
    if (!markerTrack) return;

    auto bounds = getLocalBounds();
    const auto& markers = markerTrack->getMarkers();

    for (const auto& marker : markers)
    {
        // Convert beat position to bar position
        double barPosition = marker.beatPosition / timeSignatureNumerator;

        // Calculate x position
        double barOffset = barPosition - scrollOffset;
        int x = static_cast<int>(barOffset * barWidth);

        // Skip if not visible
        if (x < -20 || x > bounds.getWidth() + 20)
            continue;

        // Draw marker flag
        g.setColour(marker.colour);

        // Draw vertical line
        g.drawVerticalLine(x, 0.0f, static_cast<float>(bounds.getHeight()));

        // Draw flag shape at top
        juce::Path flag;
        float flagWidth = 8.0f;
        float flagHeight = 12.0f;
        flag.addRoundedRectangle(
            static_cast<float>(x), 0.0f,
            flagWidth + static_cast<float>(marker.name.length()) * 4.5f, flagHeight,
            2.0f
        );
        g.fillPath(flag);

        // Draw marker name
        g.setColour(juce::Colours::white);
        g.setFont(9.0f);
        g.drawText(marker.name, x + 2, 1, 60, 10, juce::Justification::left, true);
    }
}
