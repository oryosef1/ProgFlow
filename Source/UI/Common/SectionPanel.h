#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * SectionPanel - Container with consistent section styling
 *
 * Provides:
 * - Dark inset background
 * - Uppercase section title
 * - Consistent padding
 * - Rounded corners
 */
class SectionPanel : public juce::Component
{
public:
    SectionPanel(const juce::String& title = "");
    ~SectionPanel() override = default;

    void setTitle(const juce::String& newTitle);
    juce::String getTitle() const { return title; }

    // Get the content area bounds (inside padding)
    juce::Rectangle<int> getContentBounds() const;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::String title;

    // Layout constants
    static constexpr int HEADER_HEIGHT = 20;
    static constexpr int PADDING = ProgFlowSpacing::MD;  // 16px
    static constexpr int CORNER_RADIUS = ProgFlowSpacing::SECTION_CORNER_RADIUS;  // 6px

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SectionPanel)
};
