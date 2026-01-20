#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../LookAndFeel.h"

/**
 * ResizablePanel - Panel with drag-to-resize functionality
 *
 * Features:
 * - Drag handle at specified edge
 * - Minimum/maximum size constraints
 * - Double-click to toggle between min and default size
 * - Saves/restores size from preferences (optional)
 * - Visual feedback on drag handle hover
 */
class ResizablePanel : public juce::Component
{
public:
    enum class Edge { Top, Bottom, Left, Right };

    ResizablePanel(Edge resizeEdge = Edge::Top);
    ~ResizablePanel() override = default;

    //==========================================================================
    // Configuration
    void setMinSize(int size);
    void setMaxSize(int size);
    void setDefaultSize(int size);
    void setPreferenceKey(const juce::String& key);  // For persistence

    // Get current size (height for top/bottom, width for left/right)
    int getCurrentSize() const;
    void setCurrentSize(int size);

    //==========================================================================
    // Callbacks
    std::function<void(int)> onResize;  // Called with new size during drag

    //==========================================================================
    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    //==========================================================================
    // Content area (excludes drag handle)
    juce::Rectangle<int> getContentArea() const;

    //==========================================================================
    // Static constants
    static constexpr int HANDLE_SIZE = 6;

private:
    Edge edge;
    int minSize = 100;
    int maxSize = 600;
    int defaultSize = 280;
    int currentSize = 280;
    juce::String preferenceKey;

    bool isDragging = false;
    bool isHoveringHandle = false;
    int dragStartSize = 0;
    int dragStartMousePos = 0;

    // Check if mouse is over the drag handle
    bool isInDragHandle(const juce::Point<int>& pos) const;
    juce::Rectangle<int> getDragHandleArea() const;

    // Update cursor
    void updateMouseCursor();

    // Save/load from preferences
    void saveSize();
    void loadSize();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizablePanel)
};
