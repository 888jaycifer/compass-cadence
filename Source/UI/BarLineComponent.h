#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PulseGroupComponent.h"
#include "../Model/LyricDocument.h"
#include <vector>
#include <memory>

namespace CompassCadence
{

class BarLineComponent : public juce::Component,
                         public juce::Label::Listener,
                         public PulseGroupComponent::PulseNavListener
{
public:
    class BarNavListener
    {
    public:
        virtual ~BarNavListener() = default;
        virtual void onBarNavigateSyllable(int barIdx, int globalSylIdx, bool forward) = 0;
        virtual void onBarJumpToCell(int targetBarIdx, int targetGlobalSylIdx) = 0;
        virtual void onBarEnterNext(int barIdx) = 0;
    };

    BarLineComponent(LyricDocument& doc, int barIndex, float marginLineX);
    ~BarLineComponent() override;

    void setBarNavListener(BarNavListener* l) { navListener = l; }

    int getBarIndex() const noexcept { return barIndex; }

    void rebuildPulses();
    void updateContent();
    void updateSelection();
    void updateMetricLabel();
    void updateSyllableCounter();
    void setPlayheadActive(bool isBarActive, double progress = 0.0, bool isPlaying = false);

    void focusSyllable(int globalSyllableIndex);

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    bool hitTest(int x, int y) override;

    // juce::Label::Listener
    void labelTextChanged(juce::Label* labelThatHasChanged) override;
    void editorShown(juce::Label* label, juce::TextEditor& editor) override;
    void editorHidden(juce::Label* label, juce::TextEditor& editor) override;

    // PulseGroupComponent::PulseNavListener
    void onSyllableAdvance(int barIdx, int globalSylIdx, bool forward) override;
    void onSyllableJumpTo(int targetBarIdx, int targetGlobalSylIdx) override;
    void onSyllableEnterNextBar(int barIdx) override;

private:
    LyricDocument& document;
    int barIndex;
    float marginX;
    bool isBarActiveInDAW = false;
    bool isDAWPlaying = false;
    double barProgress = 0.0;

    juce::Label metricLabel;
    std::vector<std::unique_ptr<PulseGroupComponent>> pulseGroups;

    juce::TextButton decSylBtn { "-" };
    juce::Label sylCountLabel;
    juce::TextButton incSylBtn { "+" };
    juce::TextButton addLineBtn { "+" };

    BarNavListener* navListener = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BarLineComponent)
};

} // namespace CompassCadence
