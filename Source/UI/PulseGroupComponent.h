#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SyllableCellComponent.h"
#include "../Model/LyricDocument.h"
#include <vector>
#include <memory>

namespace CompassCadence
{

class PulseGroupComponent : public juce::Component,
                            public SyllableCellComponent::NavigationListener
{
public:
    class PulseNavListener
    {
    public:
        virtual ~PulseNavListener() = default;
        virtual void onSyllableAdvance(int barIdx, int globalSylIdx, bool forward) = 0;
        virtual void onSyllableJumpTo(int targetBarIdx, int targetGlobalSylIdx) = 0;
        virtual void onSyllableEnterNextBar(int barIdx) = 0;
    };

    PulseGroupComponent(LyricDocument& doc, int barIndex, int pulseIndex, int syllableCount, int startGlobalIndex);
    ~PulseGroupComponent() override;

    void setPulseNavListener(PulseNavListener* l) { pulseNavListener = l; }

    void updateContent();
    void setPlayheadActive(int activeSyllableInPulse);

    int getPulseIndex() const noexcept { return pulseIndex; }
    int getSyllableCount() const noexcept { return (int)cells.size(); }
    SyllableCellComponent* getCell(int index);

    void paint(juce::Graphics& g) override;
    void resized() override;

    // SyllableCellComponent::NavigationListener
    void onCellAdvance(int barIdx, int globalSylIdx, bool forward) override;
    void onCellJumpTo(int targetBarIdx, int targetGlobalSylIdx) override;
    void onCellEnterNextBar(int barIdx) override;

private:
    LyricDocument& document;
    int barIndex;
    int pulseIndex;
    int startGlobalIndex;

    std::vector<std::unique_ptr<SyllableCellComponent>> cells;
    PulseNavListener* pulseNavListener = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulseGroupComponent)
};

} // namespace CompassCadence
