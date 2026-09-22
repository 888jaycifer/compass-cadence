#include "SyllableCellComponent.h"
#include "NotebookLookAndFeel.h"
#include "NotebookPageView.h"
#include "VowelColorCustomizerDialog.h"
#include "../Model/SyllableSplitter.h"

namespace CompassCadence
{

class SyllableInlineEditor : public juce::TextEditor
{
public:
    SyllableInlineEditor(SyllableCellComponent& ownerComponent)
        : owner(ownerComponent)
    {
        setMultiLine(false);
        setReturnKeyStartsNewLine(false);
        setTabKeyUsedAsCharacter(false);
        setBorder(juce::BorderSize<int>(0));
        setIndents(2, 0);
        setColour(backgroundColourId, juce::Colours::transparentBlack);
        setColour(textColourId, NotebookLookAndFeel::getGraphiteColour());
        setColour(outlineColourId, juce::Colours::transparentBlack);
        setColour(focusedOutlineColourId, juce::Colours::transparentBlack);
        updateEditorFont(owner.isBold());
    }

    ~SyllableInlineEditor() override = default;

    void updateEditorFont(bool isBold)
    {
        setFont(juce::Font(juce::FontOptions("Segoe UI", 14.0f, isBold ? juce::Font::bold : juce::Font::plain)));
    }

    bool keyPressed(const juce::KeyPress& key) override
    {
        // Ctrl+B: Toggle bold emphasis
        if (key == juce::KeyPress('b', juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress('b', juce::ModifierKeys::commandModifier, 0))
        {
            owner.toggleBold();
            return true;
        }

        // Ctrl+Left / Cmd+Left: Align Left
        if (key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier, 0))
        {
            owner.setAlignment(LyricDocument::AlignLeft);
            setJustification(juce::Justification::centredLeft);
            return true;
        }

        // Ctrl+Right / Cmd+Right: Align Right
        if (key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier, 0))
        {
            owner.setAlignment(LyricDocument::AlignRight);
            setJustification(juce::Justification::centredRight);
            return true;
        }

        // Ctrl+Up / Ctrl+Down: Align Center
        if (key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier, 0) ||
            key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier, 0))
        {
            owner.setAlignment(LyricDocument::AlignCenter);
            setJustification(juce::Justification::centred);
            return true;
        }

        // Space or Hyphen: natural inline typing flow -> advance to next syllable cell
        if (key.getTextCharacter() == ' ' || key.getTextCharacter() == '-')
        {
            juce::String current = getText().trim();
            if (key.getTextCharacter() == '-' && !current.isEmpty())
                current += "-";

            owner.commitText(current, true);
            return true;
        }

        // Tab or Shift+Tab: commit and navigate
        if (key.isKeyCode(juce::KeyPress::tabKey))
        {
            owner.commitText(getText().trim(), false);
            owner.advanceFocus(!key.getModifiers().isShiftDown());
            return true;
        }

        // Return / Enter: Jump to next bar
        if (key.isKeyCode(juce::KeyPress::returnKey))
        {
            owner.commitText(getText().trim(), false);
            owner.jumpToNextBar();
            return true;
        }

        // Escape: cancel / stop editing
        if (key.isKeyCode(juce::KeyPress::escapeKey))
        {
            owner.stopEditing();
            return true;
        }

        // Ctrl+Z: Document Undo
        if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
        {
            owner.stopEditing();
            owner.undoDocument();
            return true;
        }

        // Redo: Ctrl+Alt+Z (FL Studio), Ctrl+Y, Ctrl+Shift+Z
        if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::altModifier, 0) ||
            key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::altModifier, 0) ||
            key == juce::KeyPress('y', juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) ||
            key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0) ||
            key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
        {
            owner.stopEditing();
            owner.redoDocument();
            return true;
        }

        // Ctrl+J: Join with next cell
        if (key == juce::KeyPress('j', juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress('j', juce::ModifierKeys::commandModifier, 0))
        {
            owner.commitText(getText().trim(), false);
            owner.joinWithNext();
            return true;
        }

        // Ctrl+K or Ctrl+Shift+S: Split current cell into syllables
        if (key == juce::KeyPress('k', juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0) ||
            key == juce::KeyPress('s', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0) ||
            key == juce::KeyPress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
        {
            owner.commitText(getText().trim(), false);
            owner.splitCurrent();
            return true;
        }

        // Backspace on empty text: backstep into previous cell
        if (key.isKeyCode(juce::KeyPress::backspaceKey) && getText().isEmpty())
        {
            owner.advanceFocus(false);
            return true;
        }

        // Backspace at position 0 when text is not empty: join with previous cell
        if (key.isKeyCode(juce::KeyPress::backspaceKey) && getCaretPosition() == 0)
        {
            owner.commitText(getText().trim(), false);
            owner.joinWithPrevious();
            return true;
        }

        // Intercept paste for multi-word or multi-syllable auto-spread
        if (key == juce::KeyPress('v', juce::ModifierKeys::ctrlModifier, 0) ||
            key == juce::KeyPress('v', juce::ModifierKeys::commandModifier, 0))
        {
            juce::String clipboard = juce::SystemClipboard::getTextFromClipboard();
            if (clipboard.containsAnyOf(" \t\r\n") || SyllableSplitter::splitLineIntoSyllables(clipboard).size() > 1)
            {
                owner.handleMultiWordPaste(clipboard);
                return true;
            }
        }

        return TextEditor::keyPressed(key);
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            owner.showContextMenu(e);
            return;
        }
        juce::TextEditor::mouseDown(e);
    }

private:
    SyllableCellComponent& owner;
};

class AlignArrowButton : public juce::Button
{
public:
    enum ArrowDirection { DirectionLeft, DirectionRight };

    AlignArrowButton(ArrowDirection dir, const juce::String& tip)
        : juce::Button(tip), direction(dir)
    {
        setTooltip(tip);
        setTriggeredOnMouseDown(true);
    }

    void paintButton(juce::Graphics& g, bool isOver, bool isDown) override
    {
        auto b = getLocalBounds().toFloat().reduced(0.5f);

        // Vibrant copper amber theme (NOT gray!)
        const juce::Colour amberPrimary(0xFFD97706);
        const juce::Colour amberHover(0xFFF59E0B);
        const juce::Colour amberDown(0xFFB45309);

        // Stationery pill background with border
        if (isActive)
        {
            g.setColour(amberPrimary); // Filled amber for active alignment
            g.fillRoundedRectangle(b, 2.5f);
        }
        else if (isDown)
        {
            g.setColour(amberDown.withAlpha(0.35f));
            g.fillRoundedRectangle(b, 2.5f);
            g.setColour(amberDown);
            g.drawRoundedRectangle(b, 2.5f, 1.2f);
        }
        else if (isOver)
        {
            g.setColour(NotebookLookAndFeel::getPaperColour().withAlpha(0.98f));
            g.fillRoundedRectangle(b, 2.5f);
            g.setColour(amberHover);
            g.drawRoundedRectangle(b, 2.5f, 1.2f);
        }
        else
        {
            g.setColour(NotebookLookAndFeel::getPaperColour().withAlpha(0.92f));
            g.fillRoundedRectangle(b, 2.5f);
            g.setColour(amberPrimary.withAlpha(0.70f));
            g.drawRoundedRectangle(b, 2.5f, 1.0f);
        }

        // Arrow vector color
        juce::Colour arrowCol;
        if (isActive)
            arrowCol = juce::Colours::white;
        else if (isDown)
            arrowCol = amberDown;
        else if (isOver)
            arrowCol = amberHover;
        else
            arrowCol = amberPrimary;

        g.setColour(arrowCol);

        float cx = b.getCentreX();
        float cy = b.getCentreY();
        float halfLen = std::clamp(std::min(b.getWidth(), b.getHeight()) * 0.28f, 3.2f, 5.0f);
        float headSize = halfLen * 0.75f;

        juce::Path p;
        if (direction == DirectionLeft)
        {
            // Horizontal stem pointing left
            p.startNewSubPath(cx + halfLen, cy);
            p.lineTo(cx - halfLen, cy);
            // Arrowhead <
            p.startNewSubPath(cx - halfLen + headSize, cy - headSize);
            p.lineTo(cx - halfLen, cy);
            p.lineTo(cx - halfLen + headSize, cy + headSize);
        }
        else if (direction == DirectionRight)
        {
            // Horizontal stem pointing right
            p.startNewSubPath(cx - halfLen, cy);
            p.lineTo(cx + halfLen, cy);
            // Arrowhead >
            p.startNewSubPath(cx + halfLen - headSize, cy - headSize);
            p.lineTo(cx + halfLen, cy);
            p.lineTo(cx + halfLen - headSize, cy + headSize);
        }

        g.strokePath(p, juce::PathStrokeType(1.6f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }

    void setActive(bool active)
    {
        if (isActive != active)
        {
            isActive = active;
            repaint();
        }
    }

private:
    ArrowDirection direction;
    bool isActive = false;
};

class AlignUnderlineButton : public juce::Button
{
public:
    AlignUnderlineButton(const juce::String& tip)
        : juce::Button(tip)
    {
        setTooltip(tip);
        setTriggeredOnMouseDown(true);
    }

    bool hitTest(int x, int y) override
    {
        return x >= -2 && x < getWidth() + 2 && y >= -3 && y <= getHeight() + 3;
    }

    void paintButton(juce::Graphics& g, bool isOver, bool isDown) override
    {
        auto b = getLocalBounds().toFloat();
        float barH = 3.5f;
        float y = b.getBottom() - barH - 1.0f;
        float padX = 1.0f;
        auto barRect = juce::Rectangle<float>(b.getX() + padX, y, std::max(6.0f, b.getWidth() - (padX * 2.0f)), barH);

        const juce::Colour amberPrimary(0xFFD97706);
        const juce::Colour amberHover(0xFFF59E0B);
        const juce::Colour amberDown(0xFFB45309);

        // Subtle warm glow across the clickable bottom zone
        if (isDown)
        {
            g.setColour(amberDown.withAlpha(0.20f));
            g.fillRoundedRectangle(b.reduced(0.5f), 2.0f);
        }
        else if (isOver)
        {
            g.setColour(amberHover.withAlpha(0.12f));
            g.fillRoundedRectangle(b.reduced(0.5f), 2.0f);
        }

        juce::Colour barCol;
        if (isActive)
            barCol = amberPrimary;
        else if (isDown)
            barCol = amberDown;
        else if (isOver)
            barCol = amberHover;
        else
            barCol = amberPrimary.withAlpha(0.85f);

        g.setColour(barCol);
        g.fillRoundedRectangle(barRect, barH * 0.5f);

        if (isOver || isActive)
        {
            g.setColour(amberHover.withAlpha(0.60f));
            g.drawRoundedRectangle(barRect.expanded(0.5f), barH * 0.5f, 0.8f);
        }
    }

    void setActive(bool active)
    {
        if (isActive != active)
        {
            isActive = active;
            repaint();
        }
    }

private:
    bool isActive = false;
};

namespace
{
    const juce::Colour swatchMint     (0x9986EFAC); // Pastel Mint
    const juce::Colour swatchSky      (0x9993C5FD); // Pastel Sky
    const juce::Colour swatchPeach    (0x99FED7AA); // Pastel Peach
    const juce::Colour swatchLavender (0x99DDD6FE); // Pastel Lavender
    const juce::Colour swatchButter   (0x99FEF08A); // Pastel Butter
    const juce::Colour swatchRose     (0x99FECDD3); // Pastel Rose
    const juce::Colour swatchSlate    (0x99CBD5E1); // Slate Gray
}

class CustomColorCallout : public juce::Component,
                           public juce::ChangeListener
{
public:
    CustomColorCallout(SyllableCellComponent& ownerComp, LyricDocument& doc, juce::Colour initialColor)
        : owner(ownerComp), document(doc)
    {
        selector = std::make_unique<juce::ColourSelector>(
            juce::ColourSelector::showColourAtTop |
            juce::ColourSelector::showSliders |
            juce::ColourSelector::showColourspace);
        selector->setCurrentColour(initialColor.withAlpha(0.65f));
        selector->addChangeListener(this);
        addAndMakeVisible(selector.get());
        setSize(280, 260);
    }

    ~CustomColorCallout() override
    {
        if (selector != nullptr)
            selector->removeChangeListener(this);
    }

    void resized() override
    {
        if (selector != nullptr)
            selector->setBounds(getLocalBounds());
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (source == selector.get())
        {
            auto col = selector->getCurrentColour();
            if (col.getAlpha() > 210)
                col = col.withAlpha(0.65f);
            else if (col.getAlpha() < 40)
                col = col.withAlpha(0.50f);

            if (document.isCellSelected(owner.getBarIndex(), owner.getGlobalSyllableIndex()) &&
                document.getSelectedCells().size() > 1)
            {
                document.setSelectionCustomColor(col);
            }
            else
            {
                document.setCustomCellColor(owner.getBarIndex(), owner.getGlobalSyllableIndex(), col);
            }
            owner.updateContent();
        }
    }

private:
    SyllableCellComponent& owner;
    LyricDocument& document;
    std::unique_ptr<juce::ColourSelector> selector;
};

} // namespace CompassCadence

namespace CompassCadence
{

SyllableCellComponent::SyllableCellComponent(LyricDocument& doc, int bar, int pulse, int sylInPulse, int globalSyl)
    : document(doc), barIndex(bar), pulseIndex(pulse),
      syllableInPulse(sylInPulse), globalSyllableIndex(globalSyl)
{
    setWantsKeyboardFocus(true);

    auto applyAlign = [this](LyricDocument::CellAlignment align)
    {
        if (document.isCellSelected(barIndex, globalSyllableIndex) && document.getSelectedCells().size() > 1)
            document.setSelectionAlignment(align);
        else
            setAlignment(align);
        updateAlignButtonStates();
    };

    alignLeftBtn = std::make_unique<AlignArrowButton>(AlignArrowButton::DirectionLeft, "Align Left (\xe2\x86\x90)");
    alignLeftBtn->onClick = [this, applyAlign] {
        applyAlign(LyricDocument::AlignLeft);
    };
    addChildComponent(alignLeftBtn.get());
    alignLeftBtn->setVisible(false);

    alignCenterBtn = std::make_unique<AlignUnderlineButton>("Align Center / Down (\xe2\x86\x93)");
    alignCenterBtn->onClick = [this, applyAlign] {
        applyAlign(LyricDocument::AlignCenter);
    };
    addChildComponent(alignCenterBtn.get());
    alignCenterBtn->setVisible(false);

    alignRightBtn = std::make_unique<AlignArrowButton>(AlignArrowButton::DirectionRight, "Align Right (\xe2\x86\x92)");
    alignRightBtn->onClick = [this, applyAlign] {
        applyAlign(LyricDocument::AlignRight);
    };
    addChildComponent(alignRightBtn.get());
    alignRightBtn->setVisible(false);

    addMouseListener(this, true);

    updateContent();
}

SyllableCellComponent::~SyllableCellComponent()
{
}

void SyllableCellComponent::setPlayheadActive(bool active)
{
    if (isPlayheadActive != active)
    {
        isPlayheadActive = active;
        // Repaint if needed for playhead (active highlight removed in favor of sweeping needle cursor)
    }
}

void SyllableCellComponent::updateContent()
{
    currentText = document.getSyllable(barIndex, globalSyllableIndex);

    auto colorMode = document.getRhymeClassifier().getColorMode();
    if (colorMode == RhymeClassifier::ColorMode::Off)
    {
        rhymeHighlight = juce::Colours::transparentBlack;
    }
    else if (colorMode == RhymeClassifier::ColorMode::Repeats)
    {
        rhymeHighlight = document.getRhymeClassifier().getHighlightForCell(barIndex, globalSyllableIndex, currentText, {}, {});
    }
    else // ColorMode::Rhymes
    {
        if (document.hasCustomCellColor(barIndex, globalSyllableIndex))
        {
            rhymeHighlight = document.getCustomCellColor(barIndex, globalSyllableIndex);
        }
        else
        {
            juce::String prevSyl, nextSyl;
            if (globalSyllableIndex > 0)
                prevSyl = document.getSyllable(barIndex, globalSyllableIndex - 1).trim();
            int totalSyls = document.getNotation(barIndex).getTotalSyllables();
            if (globalSyllableIndex + 1 < totalSyls)
                nextSyl = document.getSyllable(barIndex, globalSyllableIndex + 1).trim();

            rhymeHighlight = document.getRhymeClassifier().getHighlightForCell(barIndex, globalSyllableIndex, currentText, prevSyl, nextSyl);
        }
    }

    if (!isMouseOver(true))
    {
        if (alignLeftBtn != nullptr)   alignLeftBtn->setVisible(false);
        if (alignCenterBtn != nullptr) alignCenterBtn->setVisible(false);
        if (alignRightBtn != nullptr)  alignRightBtn->setVisible(false);
    }
    else
    {
        updateAlignButtonStates();
    }

    repaint();
}

void SyllableCellComponent::startEditing(juce::juce_wchar initialChar)
{
    if (editor == nullptr)
    {
        editor = std::make_unique<SyllableInlineEditor>(*this);
        editor->addListener(this);

        juce::Justification just = juce::Justification::centred;
        auto align = document.getCellAlignment(barIndex, globalSyllableIndex);
        if (align == LyricDocument::AlignLeft)
            just = juce::Justification::centredLeft;
        else if (align == LyricDocument::AlignRight)
            just = juce::Justification::centredRight;
        editor->setJustification(just);

        addAndMakeVisible(editor.get());
        editor->setBounds(getLocalBounds().reduced(2));

        if (initialChar >= 32)
        {
            // Preserve the typed character and place caret after it!
            editor->setText(juce::String::charToString(initialChar), false);
            editor->setCaretPosition(1);
        }
        else
        {
            editor->setText(currentText, false);
            editor->selectAll();
        }

        editor->grabKeyboardFocus();
    }
}

void SyllableCellComponent::stopEditing()
{
    if (editor != nullptr)
    {
        juce::String newText = editor->getText().trim();
        editor.reset();

        currentText = newText;
        document.setSyllable(barIndex, globalSyllableIndex, newText);
        updateContent();
    }
}

void SyllableCellComponent::openCustomColorPicker()
{
    juce::Colour initialCol = document.hasCustomCellColor(barIndex, globalSyllableIndex)
        ? document.getCustomCellColor(barIndex, globalSyllableIndex)
        : rhymeHighlight;

    if (initialCol.isTransparent() || initialCol.getAlpha() == 0)
        initialCol = juce::Colour(0xFF60A5FA);

    auto callout = std::make_unique<CustomColorCallout>(*this, document, initialCol);
    juce::CallOutBox::launchAsynchronously(std::move(callout), getScreenBounds(), nullptr);
}

void SyllableCellComponent::showContextMenu(const juce::MouseEvent&)
{
    if (editor != nullptr)
    {
        commitText(editor->getText().trim(), false);
    }

    if (!document.isCellSelected(barIndex, globalSyllableIndex))
        document.selectCell(barIndex, globalSyllableIndex, false);

    bool isMulti = (document.getSelectedCells().size() > 1);

    juce::PopupMenu menu;

    // 1. Header: Syllable phonetic rhyme sound
    juce::String trimmed = currentText.trim();
    juce::String headerText;
    if (trimmed.isNotEmpty())
    {
        juce::String prevSyl, nextSyl;
        if (globalSyllableIndex > 0)
            prevSyl = document.getSyllable(barIndex, globalSyllableIndex - 1).trim();
        int totalSyls = document.getNotation(barIndex).getTotalSyllables();
        if (globalSyllableIndex + 1 < totalSyls)
            nextSyl = document.getSyllable(barIndex, globalSyllableIndex + 1).trim();

        juce::String rhymeKey = RhymeClassifier::extractRhymeKeyWithContext(trimmed, prevSyl, nextSyl);
        const auto* info = RhymeClassifier::findVowelSound(rhymeKey);
        headerText = "Syllable: \"" + trimmed + "\"";
        if (info != nullptr)
            headerText += " [" + info->label + " - " + info->examples + "]";
        else if (rhymeKey.isNotEmpty())
            headerText += " [" + rhymeKey + "]";
    }
    else
    {
        headerText = "Syllable: (empty)";
    }
    menu.addSectionHeader(headerText);

    // 2. Highlight Color Submenu with 15 Vowel Phonemes & Swatches
    juce::PopupMenu colorSubMenu;
    bool isAuto = !document.hasCustomCellColor(barIndex, globalSyllableIndex);
    colorSubMenu.addItem(100, "Auto (Phonemic Vowel Tint)", true, isAuto);
    colorSubMenu.addSeparator();

    const auto& catalog = RhymeClassifier::getVowelSoundCatalog();
    for (int i = 0; i < (int)catalog.size(); ++i)
    {
        const auto& entry = catalog[i];
        juce::Colour c = document.getVowelSoundColor(entry.key);

        juce::Image img(juce::Image::ARGB, 14, 14, true);
        {
            juce::Graphics g(img);
            g.setColour(c.withAlpha(1.0f));
            g.fillRoundedRectangle(0.0f, 0.0f, 14.0f, 14.0f, 2.5f);
            g.setColour(juce::Colour(0x60000000));
            g.drawRoundedRectangle(0.5f, 0.5f, 13.0f, 13.0f, 2.5f, 1.0f);
        }
        auto d = std::make_unique<juce::DrawableImage>();
        d->setImage(img);

        juce::PopupMenu::Item item;
        item.itemID = 1000 + i;
        item.text = entry.label + " (" + entry.examples + ")";
        item.setImage(std::move(d));
        colorSubMenu.addItem(item);
    }

    colorSubMenu.addSeparator();
    colorSubMenu.addItem(108, "Clear Highlight (None)");
    colorSubMenu.addItem(111, "Clear All Custom Colors in Song");
    colorSubMenu.addItem(109, "Custom Cell Color...");
    colorSubMenu.addItem(110, "Customize Vowel Color Scheme...");
    menu.addSubMenu("Highlight Color", colorSubMenu);

    // 3. Text Transform Submenu
    juce::PopupMenu caseSubMenu;
    caseSubMenu.addItem(201, "UPPERCASE", trimmed.isNotEmpty());
    caseSubMenu.addItem(202, "lowercase", trimmed.isNotEmpty());
    caseSubMenu.addItem(203, "Capitalize Word", trimmed.isNotEmpty());
    menu.addSubMenu("Transform Text", caseSubMenu);

    // 4. Text Alignment Submenu
    juce::PopupMenu alignSubMenu;
    auto curAlign = document.getCellAlignment(barIndex, globalSyllableIndex);
    alignSubMenu.addItem(301, "Align Left (\xe2\x86\x90)", true, curAlign == LyricDocument::AlignLeft);
    alignSubMenu.addItem(302, "Align Center / Down (\xe2\x86\x93)", true, curAlign == LyricDocument::AlignCenter);
    alignSubMenu.addItem(303, "Align Right (\xe2\x86\x92)", true, curAlign == LyricDocument::AlignRight);
    menu.addSubMenu("Alignment", alignSubMenu);

    // 5. Formatting & Flow
    menu.addItem(401, "Bold Emphasis (Ctrl+B)", true, isBold());
    menu.addItem(402, "Duplicate into Next Box", trimmed.isNotEmpty());

    menu.addSeparator();

    // 6. Clipboard Operations
    menu.addItem(501, "Cut (Ctrl+X)", trimmed.isNotEmpty());
    menu.addItem(502, "Copy (Ctrl+C)", trimmed.isNotEmpty());
    menu.addItem(503, "Paste (Ctrl+V)", juce::SystemClipboard::getTextFromClipboard().isNotEmpty());

    menu.addSeparator();

    // 7. Meter & Syllable Insertion
    menu.addItem(701, "Insert Syllable Before (+Meter)");
    menu.addItem(702, "Insert Syllable After (+Meter)");
    bool canDeleteBox = document.getNotation(barIndex).getSyllablesForPulse(pulseIndex) > 1;
    menu.addItem(703, "Delete Syllable Box (-Meter)", canDeleteBox);

    menu.addSeparator();

    // 8. Cell Operations
    menu.addItem(601, "Join Cells (Ctrl+J)");
    menu.addItem(602, "Split Syllables (Ctrl+K)", trimmed.isNotEmpty());
    menu.addItem(603, "Clear Cell (Del)", trimmed.isNotEmpty());

    int bIdx = barIndex;
    int gIdx = globalSyllableIndex;
    auto& docRef = document;

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [this, isMulti, bIdx, gIdx, &docRef](int result) {
        if (result == 0)
            return;

        // Color actions
        if (result == 100) // Auto
        {
            if (isMulti)
                document.clearSelectionCustomColor();
            else
                document.clearCustomCellColor(barIndex, globalSyllableIndex);
            updateContent();
        }
        else if (result >= 1000 && result < 1000 + (int)RhymeClassifier::getVowelSoundCatalog().size())
        {
            int idx = result - 1000;
            const auto& catalog = RhymeClassifier::getVowelSoundCatalog();
            if (idx >= 0 && idx < (int)catalog.size())
            {
                juce::Colour chosen = document.getVowelSoundColor(catalog[(size_t)idx].key);
                if (isMulti)
                    document.setSelectionCustomColor(chosen);
                else
                    document.setCustomCellColor(barIndex, globalSyllableIndex, chosen);
                updateContent();
            }
        }
        else if (result == 108) // Clear Highlight (None)
        {
            if (isMulti)
                document.setSelectionCustomColor(juce::Colours::transparentBlack);
            else
                document.setCustomCellColor(barIndex, globalSyllableIndex, juce::Colours::transparentBlack);
            updateContent();
        }
        else if (result == 111) // Clear All Custom Colors in Song
        {
            document.clearAllCustomCellColors();
            updateContent();
        }
        else if (result == 109) // Custom Color Picker
        {
            openCustomColorPicker();
        }
        else if (result == 110) // Customize Vowel Color Scheme
        {
            VowelColorCustomizerDialog::showDialog(this, document);
        }
        // Case transforms
        else if (result == 201) // Upper
        {
            if (isMulti)
                document.transformSelectionCase(LyricDocument::CaseUpper);
            else
                document.transformCellCase(barIndex, globalSyllableIndex, LyricDocument::CaseUpper);
            updateContent();
        }
        else if (result == 202) // Lower
        {
            if (isMulti)
                document.transformSelectionCase(LyricDocument::CaseLower);
            else
                document.transformCellCase(barIndex, globalSyllableIndex, LyricDocument::CaseLower);
            updateContent();
        }
        else if (result == 203) // Capitalize
        {
            if (isMulti)
                document.transformSelectionCase(LyricDocument::CaseTitle);
            else
                document.transformCellCase(barIndex, globalSyllableIndex, LyricDocument::CaseTitle);
            updateContent();
        }
        // Alignment
        else if (result == 301)
        {
            if (isMulti)
                document.setSelectionAlignment(LyricDocument::AlignLeft);
            else
                setAlignment(LyricDocument::AlignLeft);
        }
        else if (result == 302)
        {
            if (isMulti)
                document.setSelectionAlignment(LyricDocument::AlignCenter);
            else
                setAlignment(LyricDocument::AlignCenter);
        }
        else if (result == 303)
        {
            if (isMulti)
                document.setSelectionAlignment(LyricDocument::AlignRight);
            else
                setAlignment(LyricDocument::AlignRight);
        }
        // Formatting & Flow
        else if (result == 401) // Bold
        {
            if (isMulti)
                document.toggleSelectedBold();
            else
                toggleBold();
        }
        else if (result == 402) // Duplicate into Next Box
        {
            document.duplicateCellToNext(barIndex, globalSyllableIndex);
            updateContent();
        }
        // Clipboard
        else if (result == 501) // Cut
        {
            juce::String textToCopy = isMulti ? document.getSelectedText() : currentText;
            juce::SystemClipboard::copyTextToClipboard(textToCopy);
            if (isMulti)
                document.deleteSelected();
            else
            {
                document.setSyllable(barIndex, globalSyllableIndex, "");
                updateContent();
            }
        }
        else if (result == 502) // Copy
        {
            juce::String textToCopy = isMulti ? document.getSelectedText() : currentText;
            juce::SystemClipboard::copyTextToClipboard(textToCopy);
        }
        else if (result == 503) // Paste
        {
            juce::String clip = juce::SystemClipboard::getTextFromClipboard();
            if (clip.isNotEmpty())
            {
                if (clip.containsAnyOf(" \t\r\n") || SyllableSplitter::splitLineIntoSyllables(clip).size() > 1)
                    handleMultiWordPaste(clip);
                else
                    commitText(clip, false);
            }
        }
        // Meter & Syllable Insertion
        else if (result == 701) // Insert Syllable Before
        {
            docRef.insertSyllableInBar(bIdx, gIdx, false);
            return;
        }
        else if (result == 702) // Insert Syllable After
        {
            docRef.insertSyllableInBar(bIdx, gIdx, true);
            return;
        }
        else if (result == 703) // Delete Syllable Box
        {
            docRef.deleteSyllableInBar(bIdx, gIdx);
            return;
        }
        // Cell manipulation
        else if (result == 601) // Join
        {
            if (isMulti)
                document.joinSelected();
            else
                joinWithNext();
        }
        else if (result == 602) // Split
        {
            if (isMulti)
                document.splitSelected();
            else
                splitCurrent();
        }
        else if (result == 603) // Clear
        {
            if (isMulti)
                document.deleteSelected();
            else
            {
                document.setSyllable(barIndex, globalSyllableIndex, "");
                updateContent();
            }
        }
    });
}

void SyllableCellComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.eventComponent == alignLeftBtn.get() ||
        e.eventComponent == alignCenterBtn.get() ||
        e.eventComponent == alignRightBtn.get())
    {
        return;
    }

    // Direct hit fallback on alignment button bounds
    if (alignCenterBtn != nullptr && alignCenterBtn->isVisible() &&
        alignCenterBtn->getBounds().expanded(1, 2).contains(e.getPosition()))
    {
        alignCenterBtn->triggerClick();
        return;
    }
    if (alignLeftBtn != nullptr && alignLeftBtn->isVisible() &&
        alignLeftBtn->getBounds().expanded(1, 2).contains(e.getPosition()))
    {
        alignLeftBtn->triggerClick();
        return;
    }
    if (alignRightBtn != nullptr && alignRightBtn->isVisible() &&
        alignRightBtn->getBounds().expanded(1, 2).contains(e.getPosition()))
    {
        alignRightBtn->triggerClick();
        return;
    }

    if (e.mods.isPopupMenu())
    {
        showContextMenu(e);
        return;
    }

    if (e.mods.isShiftDown())
    {
        auto sel = document.getSelectedCells();
        std::pair<int, int> anchor = sel.empty() ? std::make_pair(barIndex, globalSyllableIndex) : *sel.begin();
        document.selectRange(anchor, { barIndex, globalSyllableIndex });
    }
    else if (e.mods.isCommandDown() || e.mods.isCtrlDown())
    {
        bool wasSel = document.isCellSelected(barIndex, globalSyllableIndex);
        auto sel = document.getSelectedCells();
        if (wasSel)
            sel.erase({ barIndex, globalSyllableIndex });
        else
            sel.insert({ barIndex, globalSyllableIndex });
        document.setSelectedCells(sel);
    }
    else
    {
        bool wasSelected = document.isCellSelected(barIndex, globalSyllableIndex);
        if (!wasSelected || document.getSelectedCells().size() > 1)
        {
            document.selectCell(barIndex, globalSyllableIndex, false);
        }
        else
        {
            startEditing();
        }
    }
}

void SyllableCellComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.eventComponent == alignLeftBtn.get() ||
        e.eventComponent == alignCenterBtn.get() ||
        e.eventComponent == alignRightBtn.get())
    {
        return;
    }

    document.selectCell(barIndex, globalSyllableIndex, false);
    startEditing();
}

void SyllableCellComponent::mouseEnter(const juce::MouseEvent&)
{
    updateAlignButtonStates();
    if (alignLeftBtn != nullptr)   { alignLeftBtn->setVisible(true);   alignLeftBtn->toFront(false); }
    if (alignCenterBtn != nullptr) { alignCenterBtn->setVisible(true); alignCenterBtn->toFront(false); }
    if (alignRightBtn != nullptr)  { alignRightBtn->setVisible(true);  alignRightBtn->toFront(false); }
}

void SyllableCellComponent::mouseExit(const juce::MouseEvent&)
{
    if (!isMouseOver(true))
    {
        if (alignLeftBtn != nullptr)   alignLeftBtn->setVisible(false);
        if (alignCenterBtn != nullptr) alignCenterBtn->setVisible(false);
        if (alignRightBtn != nullptr)  alignRightBtn->setVisible(false);
    }
}

void SyllableCellComponent::mouseMove(const juce::MouseEvent&)
{
    if (alignCenterBtn != nullptr && !alignCenterBtn->isVisible())
    {
        updateAlignButtonStates();
        if (alignLeftBtn != nullptr)   { alignLeftBtn->setVisible(true);   alignLeftBtn->toFront(false); }
        if (alignCenterBtn != nullptr) { alignCenterBtn->setVisible(true); alignCenterBtn->toFront(false); }
        if (alignRightBtn != nullptr)  { alignRightBtn->setVisible(true);  alignRightBtn->toFront(false); }
    }
}

void SyllableCellComponent::updateAlignButtonStates()
{
    auto cur = document.getCellAlignment(barIndex, globalSyllableIndex);
    if (auto* b = dynamic_cast<AlignArrowButton*>(alignLeftBtn.get()))
        b->setActive(cur == LyricDocument::AlignLeft);
    if (auto* b = dynamic_cast<AlignUnderlineButton*>(alignCenterBtn.get()))
        b->setActive(cur == LyricDocument::AlignCenter);
    if (auto* b = dynamic_cast<AlignArrowButton*>(alignRightBtn.get()))
        b->setActive(cur == LyricDocument::AlignRight);
}

void SyllableCellComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (editor != nullptr ||
        e.eventComponent == alignLeftBtn.get() ||
        e.eventComponent == alignCenterBtn.get() ||
        e.eventComponent == alignRightBtn.get())
    {
        return;
    }

    // Range drag selection across cells
    if (auto* content = findParentComponentOfClass<NotebookPageContent>())
    {
        auto ptInContent = e.getEventRelativeTo(content).getPosition();
        if (auto* hitComp = content->getComponentAt(ptInContent))
        {
            auto* targetCell = dynamic_cast<SyllableCellComponent*>(hitComp);
            if (targetCell == nullptr)
                targetCell = hitComp->findParentComponentOfClass<SyllableCellComponent>();

            if (targetCell != nullptr)
            {
                document.selectRange({ barIndex, globalSyllableIndex },
                                     { targetCell->getBarIndex(), targetCell->getGlobalSyllableIndex() });
            }
        }
    }
}

bool SyllableCellComponent::keyPressed(const juce::KeyPress& key)
{
    // Delete or Backspace: Clear selected cells
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        document.deleteSelected();
        return true;
    }

    // Ctrl+Z: Undo
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        document.undo();
        return true;
    }

    // Redo: Ctrl+Alt+Z (FL Studio), Ctrl+Y, Ctrl+Shift+Z
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::altModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::altModifier, 0) ||
        key == juce::KeyPress('y', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('y', juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        document.redo();
        return true;
    }

    // Ctrl+J: Join selected cells or join with next
    if (key == juce::KeyPress('j', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('j', juce::ModifierKeys::commandModifier, 0))
    {
        document.joinSelected();
        return true;
    }

    // Ctrl+K or Ctrl+Shift+S: Split selected cells
    if (key == juce::KeyPress('k', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress('s', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0) ||
        key == juce::KeyPress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        document.splitSelected();
        return true;
    }

    // Ctrl+B: Toggle bold emphasis for selected cells
    if (key == juce::KeyPress('b', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('b', juce::ModifierKeys::commandModifier, 0))
    {
        document.toggleSelectedBold();
        return true;
    }

    // Ctrl+Left / Cmd+Left: Align Left
    if (key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier, 0))
    {
        document.setSelectionAlignment(LyricDocument::AlignLeft);
        return true;
    }

    // Ctrl+Right / Cmd+Right: Align Right
    if (key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier, 0))
    {
        document.setSelectionAlignment(LyricDocument::AlignRight);
        return true;
    }

    // Ctrl+Up / Ctrl+Down: Align Center
    if (key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier, 0))
    {
        document.setSelectionAlignment(LyricDocument::AlignCenter);
        return true;
    }

    // Escape: Clear selection
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        document.clearSelection();
        return true;
    }

    // Tab / Shift+Tab
    if (key.isKeyCode(juce::KeyPress::tabKey))
    {
        advanceFocus(!key.getModifiers().isShiftDown());
        return true;
    }

    // Enter / Return: start editing or next bar
    if (key.isKeyCode(juce::KeyPress::returnKey))
    {
        jumpToNextBar();
        return true;
    }

    // Printable character: start editing and forward typed character immediately!
    if (key.getTextCharacter() >= 32 && !key.getModifiers().isCtrlDown() && !key.getModifiers().isAltDown())
    {
        startEditing(key.getTextCharacter());
        return true;
    }

    return false;
}

void SyllableCellComponent::undoDocument()
{
    document.undo();
}

void SyllableCellComponent::redoDocument()
{
    document.redo();
}

void SyllableCellComponent::joinWithNext()
{
    document.joinWithNext(barIndex, globalSyllableIndex);
    updateContent();
}

void SyllableCellComponent::joinWithPrevious()
{
    if (globalSyllableIndex > 0)
    {
        document.joinCells({ { barIndex, globalSyllableIndex - 1 }, { barIndex, globalSyllableIndex } });
        advanceFocus(false);
    }
    else if (barIndex > 0)
    {
        int totalSyls = document.getNotation(barIndex - 1).getTotalSyllables();
        document.joinCells({ { barIndex - 1, totalSyls - 1 }, { barIndex, globalSyllableIndex } });
        advanceFocus(false);
    }
}

void SyllableCellComponent::splitCurrent()
{
    document.splitCell(barIndex, globalSyllableIndex);
    updateContent();
}

void SyllableCellComponent::toggleBold()
{
    document.toggleCellBold(barIndex, globalSyllableIndex);
    if (editor != nullptr)
    {
        if (auto* ed = dynamic_cast<SyllableInlineEditor*>(editor.get()))
            ed->updateEditorFont(isBold());
    }
    repaint();
}

bool SyllableCellComponent::isBold() const
{
    return document.isCellBold(barIndex, globalSyllableIndex);
}

void SyllableCellComponent::setAlignment(LyricDocument::CellAlignment align)
{
    document.setCellAlignment(barIndex, globalSyllableIndex, align);
    if (editor != nullptr)
    {
        juce::Justification just = juce::Justification::centred;
        if (align == LyricDocument::AlignLeft)
            just = juce::Justification::centredLeft;
        else if (align == LyricDocument::AlignRight)
            just = juce::Justification::centredRight;
        editor->setJustification(just);
    }
    repaint();
}

LyricDocument::CellAlignment SyllableCellComponent::getAlignment() const
{
    return document.getCellAlignment(barIndex, globalSyllableIndex);
}

void SyllableCellComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Rhyme Scheme / Custom Color Highlighter Tint (if any)
    if (!rhymeHighlight.isTransparent() && document.getRhymeClassifier().isEnabled())
    {
        g.setColour(rhymeHighlight);
        g.fillRoundedRectangle(bounds.reduced(1.0f), 2.0f);
    }

    // 2. Multi-box Selection Highlight
    if (document.isCellSelected(barIndex, globalSyllableIndex))
    {
        // Soft stationery denim/blue wash
        g.setColour(juce::Colour(0x352563EB));
        g.fillRoundedRectangle(bounds.reduced(0.5f), 2.0f);

        // Pencil selection border
        g.setColour(juce::Colour(0x992563EB));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 2.0f, 1.5f);
    }

    // 3. Syllable text (when not actively editing with TextEditor)
    if (editor == nullptr)
    {
        if (currentText.isNotEmpty())
        {
            g.setColour(NotebookLookAndFeel::getGraphiteColour());
            g.setFont(juce::Font(juce::FontOptions("Segoe UI", 14.0f, isBold() ? juce::Font::bold : juce::Font::plain)));

            juce::Justification just = juce::Justification::centred;
            auto align = document.getCellAlignment(barIndex, globalSyllableIndex);
            if (align == LyricDocument::AlignLeft)
                just = juce::Justification::centredLeft;
            else if (align == LyricDocument::AlignRight)
                just = juce::Justification::centredRight;

            g.drawFittedText(currentText, getLocalBounds().reduced(3, 1), just, 1);
        }
        else
        {
            // Faint dotted baseline for empty syllable slot
            g.setColour(NotebookLookAndFeel::getLightGraphiteColour().withAlpha(0.3f));
            float y = bounds.getBottom() - 4.0f;
            const float dashLengths[] = { 2.0f, 2.0f };
            g.drawDashedLine(juce::Line<float>(bounds.getX() + 3.0f, y, bounds.getRight() - 3.0f, y),
                             dashLengths, 2, 1.0f);
        }
    }
}

void SyllableCellComponent::resized()
{
    auto b = getLocalBounds();
    if (editor != nullptr)
        editor->setBounds(b.reduced(2));

    int cellW = b.getWidth();
    int cellH = b.getHeight();

    // Arrow buttons tucked at the bottom-left and bottom-right corners
    int btnW = std::clamp(cellW / 4, 8, 14);
    int btnH = std::clamp(cellH / 2, 10, 14);
    int btnY = cellH - btnH - 1;

    if (alignLeftBtn != nullptr)
        alignLeftBtn->setBounds(2, btnY, btnW, btnH);
    if (alignRightBtn != nullptr)
        alignRightBtn->setBounds(cellW - btnW - 2, btnY, btnW, btnH);

    // Thick underline all the way beneath the word (between left and right arrows)
    // Generous height (10-13px) for effortless, reliable clicking
    int underlineX = (alignLeftBtn != nullptr ? alignLeftBtn->getRight() : btnW + 2) + 1;
    int underlineRight = (alignRightBtn != nullptr ? alignRightBtn->getX() : cellW - btnW - 2) - 1;
    int underlineW = std::max(6, underlineRight - underlineX);
    int underlineH = std::clamp(cellH / 2, 10, 13);
    int underlineY = cellH - underlineH - 1;

    if (alignCenterBtn != nullptr)
        alignCenterBtn->setBounds(underlineX, underlineY, underlineW, underlineH);
}

void SyllableCellComponent::commitText(const juce::String& text, bool advanceFocus)
{
    juce::String trimmed = text.trim();
    currentText = trimmed;
    document.setSyllable(barIndex, globalSyllableIndex, trimmed);
    updateContent();

    if (advanceFocus && navListener != nullptr)
        navListener->onCellAdvance(barIndex, globalSyllableIndex, true);
}

void SyllableCellComponent::advanceFocus(bool forward)
{
    stopEditing();
    if (navListener != nullptr)
        navListener->onCellAdvance(barIndex, globalSyllableIndex, forward);
}

void SyllableCellComponent::jumpToNextBar()
{
    stopEditing();
    if (navListener != nullptr)
        navListener->onCellEnterNextBar(barIndex);
}

void SyllableCellComponent::handleMultiWordPaste(const juce::String& text)
{
    stopEditing();
    auto nextTarget = document.insertTextFlow(barIndex, globalSyllableIndex, text);
    if (navListener != nullptr)
        navListener->onCellJumpTo(nextTarget.first, nextTarget.second);
}

void SyllableCellComponent::textEditorTextChanged(juce::TextEditor& ed)
{
    // Auto-advance if text ends with space or hyphen while typing
    juce::String t = ed.getText();
    if (t.endsWith(" ") || t.endsWith("-"))
    {
        juce::String clean = t.trim();
        if (t.endsWith("-") && !clean.isEmpty())
            clean += "-";

        commitText(clean, true);
    }
}

void SyllableCellComponent::textEditorReturnKeyPressed(juce::TextEditor&)
{
    stopEditing();
    jumpToNextBar();
}

void SyllableCellComponent::textEditorEscapeKeyPressed(juce::TextEditor&)
{
    stopEditing();
}

void SyllableCellComponent::textEditorFocusLost(juce::TextEditor&)
{
    stopEditing();
}

} // namespace CompassCadence
