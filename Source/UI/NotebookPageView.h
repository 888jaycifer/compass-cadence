#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "BarLineComponent.h"
#include "../Model/LyricDocument.h"
#include "../PluginProcessor.h"
#include <vector>
#include <memory>

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

    int getBarY(int barIndex) const;
    int getBarHeight() const noexcept { return barHeight; }
    int getDisplayedStartBar() const noexcept { return barLines.empty() ? -1 : barLines.front()->getBarIndex(); }
    int getDisplayedBarCount() const noexcept { return (int)barLines.size(); }

    // BarLineComponent::BarNavListener
    void onBarNavigateSyllable(int barIdx, int globalSylIdx, bool forward) override;
    void onBarJumpToCell(int targetBarIdx, int targetGlobalSylIdx) override;
    void onBarEnterNext(int barIdx) override;

private:
    LyricDocument& document;
    float marginX;
    int barHeight = 36;
    int blankLineGap = 20;

    std::vector<std::unique_ptr<BarLineComponent>> barLines;
    std::unordered_map<int, int> barYPositions;
    bool isUpdatingSize = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookPageContent)
};

class NotebookViewport : public juce::Viewport
{
public:
    std::function<void()> onVisibleAreaChanged;

    void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override
    {
        juce::Viewport::visibleAreaChanged(newVisibleArea);
        if (onVisibleAreaChanged)
            onVisibleAreaChanged();
    }
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

    // LyricDocument::Listener
    void lyricDocumentChanged() override;
    void metricNotationChanged(const MetricNotation& newNotation) override;
    void barNotationChanged(int barIndex, const MetricNotation& newNotation) override;
    void selectionChanged() override;

private:
    CompassCadenceAudioProcessor& processor;
    LyricDocument* document = nullptr;

    static constexpr float MARGIN_X = 65.0f;

    std::unique_ptr<NotebookPageContent> pageContent;
    NotebookViewport viewport;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookPageView)
};

} // namespace CompassCadence
