#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Model/LyricDocument.h"
#include "../Model/RhymeClassifier.h"
#include <vector>
#include <functional>

namespace CompassCadence
{

class VowelColorCustomizerDialog : public juce::Component
{
public:
    struct RowItem
    {
        juce::String key;
        juce::String label;
        juce::String examples;
        std::unique_ptr<juce::Button> swatchButton;
    };

    VowelColorCustomizerDialog(LyricDocument& doc);
    ~VowelColorCustomizerDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onClose;

    static void showDialog(juce::Component* parent, LyricDocument& doc);

private:
    void openColorPicker(const juce::String& vowelKey, juce::Button* targetButton);
    void updateRepeatButtons();

    LyricDocument& document;
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> contentComp;
    std::vector<RowItem> rows;

    juce::Label repeatLabel { {}, "Repeats Minimum Syllables:" };
    juce::Slider repeatSlider;

    juce::Label repeatDistLabel { {}, "Repeats Max Line Distance:" };
    juce::Slider repeatDistSlider;

    juce::TextButton closeBtn { juce::CharPointer_UTF8("\xc3\x97") };
    juce::TextButton resetBtn { "Reset to Defaults" };
    juce::TextButton doneBtn { "Done" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VowelColorCustomizerDialog)
};

class VowelColorCustomizerOverlay : public juce::Component
{
public:
    VowelColorCustomizerOverlay(LyricDocument& doc);
    ~VowelColorCustomizerOverlay() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void resized() override;

private:
    VowelColorCustomizerDialog dialog;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VowelColorCustomizerOverlay)
};

} // namespace CompassCadence
