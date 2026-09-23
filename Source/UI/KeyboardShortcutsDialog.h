#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <functional>

namespace CompassCadence
{

class KeyboardShortcutsDialog : public juce::Component
{
public:
    struct ShortcutRow
    {
        juce::String category;
        juce::String keys;
        juce::String action;
        juce::String notes;
    };

    KeyboardShortcutsDialog();
    ~KeyboardShortcutsDialog() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onClose;

    static void showDialog(juce::Component* parent);

private:
    juce::Viewport viewport;
    std::unique_ptr<juce::Component> contentComp;
    std::vector<ShortcutRow> shortcuts;

    juce::TextButton closeBtn { juce::CharPointer_UTF8("\xc3\x97") };
    juce::TextButton resetBtn { "Reset to Defaults" };
    juce::TextButton doneBtn { "Done" };
    juce::Label statusLabel;

    void buildDefaultShortcuts();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardShortcutsDialog)
};

class KeyboardShortcutsOverlay : public juce::Component
{
public:
    KeyboardShortcutsOverlay();
    ~KeyboardShortcutsOverlay() override = default;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void resized() override;

private:
    KeyboardShortcutsDialog dialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardShortcutsOverlay)
};

} // namespace CompassCadence
