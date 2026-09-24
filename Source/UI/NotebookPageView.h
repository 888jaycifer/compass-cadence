#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BarLineComponent.h"
#include "../Model/LyricDocument.h"
#include "../PluginProcessor.h"
#include <vector>
#include <memory>
#include <functional>

namespace CompassCadence
{

class NotebookPageContent : public juce::Component,
                            public BarLineComponent::BarNavListener
{
public:
    NotebookPageContent(LyricDocument& doc, float marginLineX);
    ~NotebookPageContent() override;

    void rebuildBars();
    void appendBars(int newEndBar);
    void updateContent();
    void updateSelection();
    void updatePlayhead(const PlayheadLocation& loc, bool isPlaying);
    void barNotationChanged(int barIndex);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseExit(const juce::MouseEvent& e) override;

    int getBarY(int barIndex) const;
    int getBarHeight() const noexcept { return barHeight; }
    void setBarHeight(int h);
    int getDisplayedStartBar() const noexcept { return barLines.empty() ? -1 : barLines.front()->getBarIndex(); }
    int getDisplayedBarCount() const noexcept { return (int)barLines.size(); }

    // BarLineComponent::BarNavListener
    void onBarNavigateSyllable(int barIdx, int globalSylIdx, bool forward) override;
    void onBarJumpToCell(int targetBarIdx, int targetGlobalSylIdx) override;
    void onBarEnterNext(int barIdx) override;

private:
    LyricDocument& document;
    float marginX;
    int barHeight = 50;
    int blankLineGap = 20;

    std::vector<std::unique_ptr<BarLineComponent>> barLines;
    std::unordered_map<int, int> barYPositions;
    bool isUpdatingSize = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookPageContent)
};

class NotebookVerticalScrollBar : public juce::ScrollBar
{
public:
    enum class ResizeEdge { None, Top, Bottom };

    NotebookVerticalScrollBar();
    ~NotebookVerticalScrollBar() override;

    std::function<void()> onResizeStarted;
    std::function<void(ResizeEdge, float)> onResizeDragged;
    std::function<void()> onResizeEnded;
    std::function<void(int)> onZoomWheel;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void paint(juce::Graphics& g) override;

    juce::Rectangle<int> getThumbBounds() const;
    ResizeEdge getResizeEdgeAt(int mouseY) const;
    bool isResizingActive() const noexcept { return isResizing; }

private:
    bool isResizing = false;
    ResizeEdge activeEdge = ResizeEdge::None;
    float dragStartY = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookVerticalScrollBar)
};

class NotebookViewport : public juce::Viewport
{
public:
    NotebookViewport();
    ~NotebookViewport() override;

    std::function<void()> onVisibleAreaChanged;

    void setScrollFrozen(bool frozen) noexcept { scrollFrozen = frozen; }
    bool isScrollFrozen() const noexcept { return scrollFrozen; }

    void scrollBarMoved(juce::ScrollBar* sb, double newRangeStart) override;
    void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override;

protected:
    juce::ScrollBar* createScrollBarComponent(bool isVertical) override;

private:
    bool scrollFrozen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookViewport)
};

class DAWTimelineRulerComponent : public juce::Component
{
public:
    DAWTimelineRulerComponent(LyricDocument& doc, float marginLineX);
    void setContentWidth(int w) { contentWidth = w; repaint(); }
    void paint(juce::Graphics& g) override;

private:
    LyricDocument& document;
    float marginX;
    int contentWidth = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DAWTimelineRulerComponent)
};

class NotebookPageView : public juce::Component,
                         public LyricDocument::Listener
{
public:
    NotebookPageView(CompassCadenceAudioProcessor& processor, LyricDocument& doc);
    ~NotebookPageView() override;

    void setDocument(LyricDocument& newDoc);

    void updatePlayhead(const PlayheadLocation& loc, bool isPlaying, bool followDAW);
    void scrollToBar(int barIndex);
    void scrollToNormalized(float normY);
    void scrollByBars(int numBars);
    void checkInfiniteScroll();

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    // LyricDocument::Listener
    void lyricDocumentChanged() override;
    void metricNotationChanged(const MetricNotation& newNotation) override;
    void barNotationChanged(int barIndex, const MetricNotation& newNotation) override;
    void selectionChanged() override;

private:
    void handleResizeStarted();
    void handleResizeDragged(NotebookVerticalScrollBar::ResizeEdge edge, float deltaY);
    void handleResizeEnded();
    void handleZoomWheel(int delta);

    CompassCadenceAudioProcessor& processor;
    LyricDocument* document = nullptr;

    static constexpr float MARGIN_X = 65.0f;

    std::unique_ptr<DAWTimelineRulerComponent> timelineRuler;
    std::unique_ptr<NotebookPageContent> pageContent;
    NotebookViewport viewport;

    int resizeStartBarHeight = 50;
    int resizeAnchorBarTop = 0;
    int resizeAnchorOffsetTop = 0;
    int resizeAnchorBarBottom = 0;
    int resizeAnchorOffsetBottom = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookPageView)
};

} // namespace CompassCadence
