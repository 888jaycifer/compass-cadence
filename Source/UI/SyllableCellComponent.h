#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include "../Model/LyricDocument.h"

namespace CompassCadence
{

class SyllableCellComponent : public juce::Component,
                              public juce::TextEditor::Listener
{
public:
    class NavigationListener
    {
    public:
        virtual ~NavigationListener() = default;
        virtual void onCellAdvance(int barIdx, int globalSylIdx, bool forward) = 0;
        virtual void onCellJumpTo(int targetBarIdx, int targetGlobalSylIdx) = 0;
        virtual void onCellEnterNextBar(int barIdx) = 0;
    };

    SyllableCellComponent(LyricDocument& doc, int bar, int pulse, int sylInPulse, int globalSyl);
    ~SyllableCellComponent() override;

    void setNavigationListener(NavigationListener* l) { navListener = l; }

    void setPlayheadActive(bool active);
    void updateContent();

    void startEditing(juce::juce_wchar initialChar = 0);
    void stopEditing();
    void toggleBold();
    bool isBold() const;
    void setAlignment(LyricDocument::CellAlignment align);
    LyricDocument::CellAlignment getAlignment() const;

    void showContextMenu(const juce::MouseEvent& e);
    void openCustomColorPicker();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    // Actions
    void undoDocument();
    void redoDocument();
    void joinWithNext();
    void joinWithPrevious();
    void splitCurrent();

    // TextEditor::Listener
    void textEditorTextChanged(juce::TextEditor& editor) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;

    int getBarIndex() const noexcept { return barIndex; }
    int getPulseIndex() const noexcept { return pulseIndex; }
    int getSyllableInPulse() const noexcept { return syllableInPulse; }
    int getGlobalSyllableIndex() const noexcept { return globalSyllableIndex; }

    void commitText(const juce::String& text, bool advanceFocus = false);
    void advanceFocus(bool forward);
    void jumpToNextBar();
    void handleMultiWordPaste(const juce::String& text);

private:
    LyricDocument& document;
    int barIndex;
    int pulseIndex;
    int syllableInPulse;
    int globalSyllableIndex;

    bool isPlayheadActive = false;
    juce::Colour rhymeHighlight = juce::Colours::transparentBlack;
    juce::String currentText;

    std::unique_ptr<juce::TextEditor> editor;
    NavigationListener* navListener = nullptr;

    std::unique_ptr<juce::Button> alignLeftBtn;
    std::unique_ptr<juce::Button> alignCenterBtn;
    std::unique_ptr<juce::Button> alignRightBtn;
    void updateAlignButtonStates();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SyllableCellComponent)
};

} // namespace CompassCadence
