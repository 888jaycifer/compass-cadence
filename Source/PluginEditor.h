#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/NotebookLookAndFeel.h"
#include "UI/NotebookHeaderComponent.h"
#include "UI/NotebookTabBarComponent.h"
#include "UI/NotebookPageView.h"

namespace CompassCadence
{

class NonTextTooltipWindow : public juce::TooltipWindow
{
public:
    NonTextTooltipWindow(juce::Component* parentComp, int delayMs)
        : juce::TooltipWindow(parentComp, delayMs) {}

    juce::String getTipFor(juce::Component& c) override
    {
        // Suppress tooltips for any text editor, syllable cell, or pulse box (and any component within them)
        if (dynamic_cast<juce::TextEditor*>(&c) != nullptr ||
            c.findParentComponentOfClass<juce::TextEditor>() != nullptr ||
            dynamic_cast<SyllableCellComponent*>(&c) != nullptr ||
            c.findParentComponentOfClass<SyllableCellComponent>() != nullptr ||
            dynamic_cast<PulseGroupComponent*>(&c) != nullptr ||
            c.findParentComponentOfClass<PulseGroupComponent>() != nullptr)
        {
            return {};
        }

        return juce::TooltipWindow::getTipFor(c);
    }
};

class CompassCadenceAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          public juce::Timer
{
public:
    CompassCadenceAudioProcessorEditor(CompassCadenceAudioProcessor&);
    ~CompassCadenceAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

    void timerCallback() override;

private:
    CompassCadenceAudioProcessor& audioProcessor;
    NotebookLookAndFeel lookAndFeel;

    NotebookHeaderComponent header;
    NotebookTabBarComponent tabBar;
    NotebookPageView pageView;

    NonTextTooltipWindow tooltipWindow { this, 400 };

    juce::ComponentBoundsConstrainer constrainer;
    std::unique_ptr<juce::ResizableCornerComponent> resizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompassCadenceAudioProcessorEditor)
};

} // namespace CompassCadence
