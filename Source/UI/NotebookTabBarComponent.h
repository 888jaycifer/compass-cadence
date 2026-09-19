#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../PluginProcessor.h"
#include <vector>

namespace CompassCadence
{

class NotebookTabBarComponent : public juce::Component
{
public:
    NotebookTabBarComponent(CompassCadenceAudioProcessor& proc);
    ~NotebookTabBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

    void refreshTabs();

private:
    void showTabMenu(int tabIndex, juce::Point<int> screenPos);
    void promptRenameTab(int tabIndex);
    void promptSaveTabToSong(int tabIndex);
    void promptLoadSongIntoTab(int tabIndex);
    void promptOpenSongInNewTab();

    CompassCadenceAudioProcessor& processor;

    struct TabRect
    {
        int index = 0;
        juce::Rectangle<int> bounds;
        juce::Rectangle<int> closeBounds;
    };

    std::vector<TabRect> tabLayouts;
    juce::TextButton newTabBtn { "+" };
    int hoveredTabIndex = -1;
    int hoveredCloseTabIndex = -1;

    static constexpr float MARGIN_X = 65.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NotebookTabBarComponent)
};

} // namespace CompassCadence
