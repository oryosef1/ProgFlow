#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * CardPanel - Saturn UI card component
 *
 * Features:
 * - Gradient background (top darker to bottom)
 * - Soft shadow for depth
 * - 6px rounded corners
 * - Optional header with uppercase title
 * - 12px internal padding
 */
class CardPanel : public juce::Component
{
public:
    CardPanel(const juce::String& title = "");
    ~CardPanel() override = default;

    //==========================================================================
    // Configuration
    void setTitle(const juce::String& newTitle);
    void setShowHeader(bool show);
    void setCornerRadius(float radius);
    void setPadding(int padding);

    //==========================================================================
    // Content area (excludes header and padding)
    juce::Rectangle<int> getContentArea() const;

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    //==========================================================================
    // Static constants (Saturn design - from ProgFlowSpacing)
    static constexpr int HEADER_HEIGHT = 24;
    static constexpr int DEFAULT_CORNER_RADIUS = ProgFlowSpacing::CARD_CORNER_RADIUS;
    static constexpr int DEFAULT_PADDING = ProgFlowSpacing::CARD_PADDING;
    static constexpr int HEADER_BOTTOM_MARGIN = ProgFlowSpacing::SM;  // 8px

private:
    juce::String title;
    bool showHeader = false;
    float cornerRadius = DEFAULT_CORNER_RADIUS;
    int padding = DEFAULT_PADDING;

    // Draw helpers
    void drawCardBackground(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawHeader(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CardPanel)
};
