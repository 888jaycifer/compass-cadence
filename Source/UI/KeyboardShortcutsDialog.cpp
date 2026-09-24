#include "KeyboardShortcutsDialog.h"
#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

KeyboardShortcutsDialog::KeyboardShortcutsDialog()
{
    buildDefaultShortcuts();

    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    closeBtn.setColour(juce::TextButton::textColourOffId, NotebookLookAndFeel::getGraphiteColour());
    closeBtn.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible(closeBtn);

    doneBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFFD97706));
    doneBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    doneBtn.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible(doneBtn);

    resetBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    resetBtn.setColour(juce::TextButton::textColourOffId, NotebookLookAndFeel::getLightGraphiteColour());
    resetBtn.onClick = [this] {
        buildDefaultShortcuts();
        statusLabel.setText("Restored factory default bindings", juce::dontSendNotification);
        juce::Timer::callAfterDelay(2000, [this] {
            statusLabel.setText("Default Layout Active", juce::dontSendNotification);
        });
        resized();
        repaint();
    };
    addAndMakeVisible(resetBtn);

    statusLabel.setText("Default Layout Active", juce::dontSendNotification);
    statusLabel.setFont(juce::Font(juce::FontOptions("Calibri", 11.0f, juce::Font::italic)));
    statusLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getLightGraphiteColour());
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    contentComp = std::make_unique<juce::Component>();
    viewport.setViewedComponent(contentComp.get(), false);
    viewport.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport);
}

void KeyboardShortcutsDialog::buildDefaultShortcuts()
{
    shortcuts = {
        // Navigation & Flow
        { "Navigation", "Space / -", "Commit syllable & advance to next cell", "Splits words naturally as you type lyrics" },
        { "Navigation", "Tab / Shift+Tab", "Focus next / previous syllable cell", "Preserves and commits entered text" },
        { "Navigation", "Enter / Return", "Jump to first syllable of next bar", "Allows rapid line-by-line lyric writing" },
        { "Navigation", "Escape", "Cancel cell edit / Clear selection", "Returns keyboard focus to canvas" },
        { "Navigation", "Arrow Keys", "Navigate adjacent syllable cells", "In edit mode, navigates caret within box" },

        // Syllable Box Insertion & Deletion (Meter Alteration)
        { "Meter", "Alt+= / Alt+Ins", "Insert syllable box AFTER current cell", "Alters bar meter subdivision (+1)" },
        { "Navigation", "Alt+Shift+=", "Insert syllable box BEFORE current cell", "Alters bar meter subdivision (+1)" },
        { "Meter", "Alt+- / Alt+Del", "Remove syllable box from bar meter", "Alters bar meter subdivision (-1)" },
        { "Meter", "Hover '+' Corner", "Quick-insert syllable before or after", "Positioned at mouse-nearest bottom corner" },
        { "Meter", "Hover '-' Gutter", "Quick-remove cell from bar meter", "Positioned below bottom border to prevent misclicks" },

        // Cell Alignment
        { "Alignment", "Alt+Left / Ctrl+Left", "Align text to Left edge of box", "Hugs previous syllable box" },
        { "Alignment", "Alt+Down / Ctrl+Down", "Align text to Center of box", "Default balanced syllable position" },
        { "Alignment", "Alt+Right / Ctrl+Right", "Align text to Right edge of box", "Hugs following syllable box" },
        { "Alignment", "Header 'Align' Toggle", "Show concurrent alignment buttons", "Keeps boxes clean when toggled OFF" },

        // Text & Cell Manipulation
        { "Editing", "Ctrl+B", "Toggle bold emphasis on cell(s)", "Accents stressed beats & syncopation" },
        { "Editing", "Ctrl+J", "Join current cell with next cell", "Merges multi-box text into single cell" },
        { "Editing", "Ctrl+K", "Split cell text into two syllables", "Distributes words across metric boxes" },
        { "Editing", "Backspace on Empty", "Step back into previous syllable box", "Streamlined lyric revision" },
        { "Editing", "Backspace at Start", "Join cell text into previous cell", "Merges syllables seamlessly" },

        // Clipboard
        { "Clipboard", "Ctrl+X", "Cut cell text to clipboard & clear box", "Fully removes text from box" },
        { "Clipboard", "Ctrl+C", "Copy cell / selection text to clipboard", "Copies words without altering boxes" },
        { "Clipboard", "Ctrl+V", "Paste text with auto-syllable spread", "Spreads words/syllables across boxes" },
        { "Clipboard", "Delete / Backspace", "Clear text in selected cell(s)", "Empties cell text without deleting box" },

        // History
        { "History", "Ctrl+Z", "Undo last lyric or metric change", "100-level multi-step undo buffer" },
        { "History", "Ctrl+Y / Ctrl+Shift+Z", "Redo reverted lyric change", "Also supports FL Studio Ctrl+Alt+Z" },

        // Zoom & Sizing
        { "View", "Ctrl/Alt + Wheel", "Smoothly resize line height (32-100px)", "Vertically freezes scroll position" },
        { "View", "Scrollbar Drag Edge", "DAW-style vertical line height scaling", "Top/bottom thumb edges resize rows" }
    };
}

void KeyboardShortcutsDialog::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();

    // Stationery paper dialog background
    g.setColour(NotebookLookAndFeel::getPaperColour());
    g.fillRoundedRectangle(b, 8.0f);

    // Subtle drop border
    g.setColour(NotebookLookAndFeel::getPulseBoxBorderColour().withAlpha(0.6f));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.5f);

    // Header divider
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    g.drawLine(14.0f, 44.0f, b.getRight() - 14.0f, 44.0f, 1.0f);

    // Title
    g.setColour(NotebookLookAndFeel::getGraphiteColour());
    g.setFont(juce::Font(juce::FontOptions("Calibri", 16.0f, juce::Font::bold)));
    g.drawText("Keyboard Shortcuts & Meter Controls", 18, 10, 360, 26, juce::Justification::centredLeft);
}

void KeyboardShortcutsDialog::resized()
{
    auto b = getLocalBounds();

    closeBtn.setBounds(b.getWidth() - 34, 10, 24, 24);

    int footerY = b.getHeight() - 42;
    resetBtn.setBounds(18, footerY, 130, 28);
    statusLabel.setBounds(156, footerY, 200, 28);
    doneBtn.setBounds(b.getWidth() - 94, footerY, 76, 28);

    int contentW = b.getWidth() - 36;
    int rowH = 26;
    int totalH = (int)shortcuts.size() * rowH + 20;

    contentComp->setSize(contentW, totalH);
    viewport.setBounds(18, 48, contentW, footerY - 54);

    contentComp->removeAllChildren();

    // Layout custom table rows inside contentComp
    class TableContentComponent : public juce::Component
    {
    public:
        TableContentComponent(const std::vector<ShortcutRow>& list, int rowHeight)
            : items(list), rH(rowHeight)
        {
        }

        void paint(juce::Graphics& g) override
        {
            int y = 4;
            auto textCol = NotebookLookAndFeel::getGraphiteColour();
            auto dimCol = NotebookLookAndFeel::getLightGraphiteColour();
            auto accentCol = juce::Colour(0xFFD97706);

            for (size_t i = 0; i < items.size(); ++i)
            {
                const auto& item = items[i];

                if (i % 2 == 1)
                {
                    g.setColour(NotebookLookAndFeel::getEvenLineColour().withAlpha(0.5f));
                    g.fillRect(0, y, getWidth(), rH);
                }

                // Category pill
                g.setColour(dimCol.withAlpha(0.15f));
                g.fillRoundedRectangle(4.0f, (float)y + 3.0f, 68.0f, (float)rH - 6.0f, 3.0f);
                g.setColour(dimCol);
                g.setFont(juce::Font(juce::FontOptions("Calibri", 10.5f, juce::Font::bold)));
                g.drawText(item.category, 6, y, 64, rH, juce::Justification::centred);

                // Shortcut keys badge
                g.setColour(accentCol);
                g.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::bold)));
                g.drawText(item.keys, 80, y, 165, rH, juce::Justification::centredLeft);

                // Action description
                g.setColour(textCol);
                g.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::plain)));
                g.drawText(item.action, 250, y, 240, rH, juce::Justification::centredLeft);

                // Notes / details
                g.setColour(dimCol);
                g.setFont(juce::Font(juce::FontOptions("Calibri", 11.0f, juce::Font::italic)));
                g.drawText(item.notes, 495, y, getWidth() - 500, rH, juce::Justification::centredLeft);

                y += rH;
            }
        }

    private:
        const std::vector<ShortcutRow>& items;
        int rH;
    };

    auto* tableComp = new TableContentComponent(shortcuts, rowH);
    tableComp->setBounds(0, 0, contentW, totalH);
    contentComp->addAndMakeVisible(tableComp);
}

void KeyboardShortcutsDialog::showDialog(juce::Component* parent)
{
    if (parent == nullptr)
        return;

    auto* top = parent->getTopLevelComponent();
    if (top == nullptr)
        top = parent;

    auto overlay = std::make_unique<KeyboardShortcutsOverlay>();
    overlay->setBounds(top->getLocalBounds());
    top->addAndMakeVisible(overlay.release());
}

// -----------------------------------------------------------------------------
// KeyboardShortcutsOverlay
// -----------------------------------------------------------------------------
KeyboardShortcutsOverlay::KeyboardShortcutsOverlay()
{
    dialog.onClose = [this] {
        if (auto* p = getParentComponent())
            p->removeChildComponent(this);
    };
    addAndMakeVisible(dialog);
}

void KeyboardShortcutsOverlay::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black.withAlpha(0.45f));
}

void KeyboardShortcutsOverlay::mouseDown(const juce::MouseEvent& e)
{
    if (!dialog.getBounds().contains(e.getPosition()))
    {
        if (auto* p = getParentComponent())
            p->removeChildComponent(this);
    }
}

void KeyboardShortcutsOverlay::resized()
{
    int dw = std::min(780, getWidth() - 40);
    int dh = std::min(520, getHeight() - 40);
    dialog.setBounds((getWidth() - dw) / 2, (getHeight() - dh) / 2, dw, dh);
}

} // namespace CompassCadence
