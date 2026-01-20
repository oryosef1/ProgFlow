#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * GlassPanel - Modern frosted glass effect panel
 *
 * Features:
 * - Semi-transparent background with subtle blur simulation
 * - Soft rounded corners
 * - Optional glow border on focus/hover
 * - Optional header section with title
 * - Subtle shadow for depth
 */
class GlassPanel : public juce::Component
{
public:
    GlassPanel(const juce::String& title = "");
    ~GlassPanel() override = default;

    //==========================================================================
    // Configuration
    void setTitle(const juce::String& newTitle);
    void setShowHeader(bool show);
    void setCornerRadius(float radius);
    void setGlowOnHover(bool glow);
    void setCustomBackground(juce::Colour colour);

    //==========================================================================
    // Content area (excludes header if present)
    juce::Rectangle<int> getContentArea() const;

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    //==========================================================================
    // Static constants
    static constexpr int HEADER_HEIGHT = 32;
    static constexpr int DEFAULT_CORNER_RADIUS = 8;
    static constexpr int CONTENT_PADDING = 8;

private:
    juce::String title;
    bool showHeader = false;
    bool glowOnHover = false;
    bool isHovering = false;
    float cornerRadius = DEFAULT_CORNER_RADIUS;
    juce::Colour customBgColour;
    bool useCustomBg = false;

    // Draw helpers
    void drawGlassBackground(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawHeader(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawBorder(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlassPanel)
};
