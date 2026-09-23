#include "VowelColorCustomizerDialog.h"
#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

class ColorPickerCallout : public juce::Component,
                           public juce::ChangeListener
{
public:
    ColorPickerCallout(std::function<void(juce::Colour)> onColorChange, juce::Colour initialCol)
        : callback(onColorChange)
    {
        selector = std::make_unique<juce::ColourSelector>(
            juce::ColourSelector::showColourAtTop |
            juce::ColourSelector::showSliders |
            juce::ColourSelector::showColourspace);
        selector->setCurrentColour(initialCol.withAlpha(0.70f));
        selector->addChangeListener(this);
        addAndMakeVisible(selector.get());
        setSize(320, 290);
    }

    ~ColorPickerCallout() override
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
        if (source == selector.get() && callback)
        {
            auto col = selector->getCurrentColour();
            if (col.getAlpha() > 210)
                col = col.withAlpha(0.72f);
            else if (col.getAlpha() < 50)
                col = col.withAlpha(0.55f);
            callback(col);
        }
    }

private:
    std::function<void(juce::Colour)> callback;
    std::unique_ptr<juce::ColourSelector> selector;
};

class SwatchButton : public juce::Button
{
public:
    SwatchButton(std::function<juce::Colour()> colGetter)
        : juce::Button("swatch"), getColor(colGetter)
    {
    }

    void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        juce::ignoreUnused(shouldDrawButtonAsDown);
        auto b = getLocalBounds().toFloat().reduced(1.5f);
        juce::Colour c = getColor ? getColor() : juce::Colours::transparentBlack;

        g.setColour(c);
        g.fillRoundedRectangle(b, 4.0f);

        g.setColour(shouldDrawButtonAsHighlighted
                    ? (NotebookLookAndFeel::isDarkMode() ? juce::Colour(0xFFF59E0B) : juce::Colour(0xFFD97706))
                    : (NotebookLookAndFeel::isDarkMode() ? juce::Colour(0xFF52525B) : juce::Colour(0xFFA1A1AA)));
        g.drawRoundedRectangle(b, 4.0f, shouldDrawButtonAsHighlighted ? 1.8f : 1.0f);
    }

private:
    std::function<juce::Colour()> getColor;
};

class DialogContentComponent : public juce::Component
{
public:
    DialogContentComponent(const std::vector<VowelColorCustomizerDialog::RowItem>& r)
        : rows(r) {}

    void paint(juce::Graphics& g) override
    {
        bool dark = NotebookLookAndFeel::isDarkMode();
        int rowH = 34;
        int y = 4;
        for (const auto& r : rows)
        {
            // Subtle row separator
            g.setColour(dark ? juce::Colour(0x18FFFFFF) : juce::Colour(0x10000000));
            g.drawHorizontalLine(y + rowH - 1, 8.0f, (float)getWidth() - 8.0f);

            // Label
            g.setColour(dark ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF1E293B));
            g.setFont(juce::Font(juce::FontOptions("Segoe UI", 14.0f, juce::Font::bold)));
            g.drawText(r.label, 12, y, 110, rowH, juce::Justification::centredLeft);

            // Examples
            g.setColour(dark ? juce::Colour(0xFF94A3B8) : juce::Colour(0xFF64748B));
            g.setFont(juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::italic)));
            g.drawText("(" + r.examples + ")", 126, y, getWidth() - 126 - 72, rowH, juce::Justification::centredLeft);

            y += rowH;
        }
    }

private:
    const std::vector<VowelColorCustomizerDialog::RowItem>& rows;
};

VowelColorCustomizerDialog::VowelColorCustomizerDialog(LyricDocument& doc)
    : document(doc)
{
    const auto& catalog = RhymeClassifier::getVowelSoundCatalog();
    for (const auto& item : catalog)
    {
        RowItem r;
        r.key = item.key;
        r.label = item.label;
        r.examples = item.examples;

        juce::String key = item.key;
        auto btn = std::make_unique<SwatchButton>([this, key] {
            return document.getVowelSoundColor(key);
        });
        btn->setTooltip("Click to customize color for \"" + item.label + "\"");
        btn->onClick = [this, key, btnPtr = btn.get()] {
            openColorPicker(key, btnPtr);
        };
        r.swatchButton = std::move(btn);

        rows.push_back(std::move(r));
    }

    contentComp = std::make_unique<DialogContentComponent>(rows);
    for (auto& r : rows)
    {
        if (r.swatchButton != nullptr)
            contentComp->addAndMakeVisible(r.swatchButton.get());
    }

    viewport.setViewedComponent(contentComp.get(), false);
    viewport.setScrollBarsShown(true, false);
    viewport.setScrollBarThickness(10);
    addAndMakeVisible(viewport);

    repeatLabel.setFont(juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::bold)));
    repeatLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    addAndMakeVisible(repeatLabel);

    repeat2Btn.setClickingTogglesState(false);
    repeat2Btn.setTooltip("Detect and highlight exact matches of 2 or more syllables.");
    repeat2Btn.onClick = [this] {
        document.setMinRepeatLength(2);
        updateRepeatButtons();
    };
    addAndMakeVisible(repeat2Btn);

    repeat3Btn.setClickingTogglesState(false);
    repeat3Btn.setTooltip("Detect and highlight exact matches of 3 or more syllables.");
    repeat3Btn.onClick = [this] {
        document.setMinRepeatLength(3);
        updateRepeatButtons();
    };
    addAndMakeVisible(repeat3Btn);

    repeat4Btn.setClickingTogglesState(false);
    repeat4Btn.setTooltip("Detect and highlight exact matches of 4 or more syllables.");
    repeat4Btn.onClick = [this] {
        document.setMinRepeatLength(4);
        updateRepeatButtons();
    };
    addAndMakeVisible(repeat4Btn);

    updateRepeatButtons();

    repeatDistLabel.setFont(juce::Font(juce::FontOptions("Segoe UI", 13.0f, juce::Font::bold)));
    repeatDistLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    repeatDistLabel.setTooltip("Maximum lines apart for a repeated sequence match (0 = same line, 1 = couplet, 24 = default).");
    addAndMakeVisible(repeatDistLabel);

    repeatDistSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    repeatDistSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    repeatDistSlider.setRange(0.0, 64.0, 1.0);
    repeatDistSlider.setTextValueSuffix(" lines");
    repeatDistSlider.setValue(document.getMaxRepeatLineDistance(), juce::dontSendNotification);
    repeatDistSlider.setTooltip("Maximum distance in lines/bars between repeated sequences. 0 = same line only, 1 = couplets, 24 = default baseline.");
    repeatDistSlider.onValueChange = [this] {
        document.setMaxRepeatLineDistance((int)repeatDistSlider.getValue());
    };
    addAndMakeVisible(repeatDistSlider);

    closeBtn.setTooltip("Close");
    closeBtn.onClick = [this] {
        if (onClose) onClose();
    };
    addAndMakeVisible(closeBtn);

    resetBtn.setTooltip("Restore all 15 vowel sounds to their default stationery pastel colors.");
    resetBtn.onClick = [this] {
        document.resetVowelSoundColorsToDefaults();
        repaint();
        if (contentComp != nullptr)
            contentComp->repaint();
    };
    addAndMakeVisible(resetBtn);

    doneBtn.onClick = [this] {
        if (onClose) onClose();
    };
    addAndMakeVisible(doneBtn);
}

VowelColorCustomizerDialog::~VowelColorCustomizerDialog()
{
    viewport.setViewedComponent(nullptr, false);
}

void VowelColorCustomizerDialog::updateRepeatButtons()
{
    int minLen = document.getMinRepeatLength();
    repeat2Btn.setToggleState(minLen == 2, juce::dontSendNotification);
    repeat3Btn.setToggleState(minLen == 3, juce::dontSendNotification);
    repeat4Btn.setToggleState(minLen == 4, juce::dontSendNotification);
}

void VowelColorCustomizerDialog::openColorPicker(const juce::String& vowelKey, juce::Button* targetButton)
{
    juce::Colour curCol = document.getVowelSoundColor(vowelKey);
    auto picker = std::make_unique<ColorPickerCallout>([this, vowelKey](juce::Colour newCol) {
        document.setVowelSoundColor(vowelKey, newCol);
        if (contentComp != nullptr)
            contentComp->repaint();
    }, curCol);

    auto* overlay = findParentComponentOfClass<VowelColorCustomizerOverlay>();
    juce::CallOutBox::launchAsynchronously(std::move(picker), targetButton->getScreenBounds(),
                                           overlay != nullptr ? (juce::Component*)overlay : (juce::Component*)this);
}

void VowelColorCustomizerDialog::paint(juce::Graphics& g)
{
    bool dark = NotebookLookAndFeel::isDarkMode();
    auto b = getLocalBounds().toFloat();

    // Card background
    g.setColour(dark ? juce::Colour(0xFF18181B) : juce::Colour(0xFFFAF8F2));
    g.fillRoundedRectangle(b, 8.0f);

    // Card border
    g.setColour(dark ? juce::Colour(0xFF52525B) : juce::Colour(0xFFCBD5E1));
    g.drawRoundedRectangle(b.reduced(0.5f), 8.0f, 1.5f);

    // Title header
    g.setColour(dark ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF1E293B));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 16.0f, juce::Font::bold)));
    g.drawText("Customize Color Palette & Repetition Rules", 18, 12, getWidth() - 60, 22, juce::Justification::centredLeft);

    g.setColour(dark ? juce::Colour(0xFF94A3B8) : juce::Colour(0xFF64748B));
    g.setFont(juce::Font(juce::FontOptions("Segoe UI", 12.0f, juce::Font::plain)));
    g.drawText("Click any color swatch to customize vowel sounds, or adjust match rules", 18, 34, getWidth() - 60, 18, juce::Justification::centredLeft);

    // Subtle rule line below header & repeat controls
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    g.drawLine(14.0f, 126.0f, (float)getWidth() - 14.0f, 126.0f, 1.0f);
}

void VowelColorCustomizerDialog::resized()
{
    auto b = getLocalBounds();
    int footerH = 50;
    auto footerArea = b.removeFromBottom(footerH);

    int headerH = 132;
    b.removeFromTop(headerH);

    closeBtn.setBounds(getWidth() - 36, 12, 24, 24);

    repeatLabel.setBounds(18, 58, 180, 26);
    int rBtnW = 90, rBtnH = 26;
    repeat2Btn.setBounds(202, 58, rBtnW, rBtnH);
    repeat3Btn.setBounds(202 + rBtnW + 4, 58, rBtnW, rBtnH);
    repeat4Btn.setBounds(202 + (rBtnW + 4) * 2, 58, rBtnW, rBtnH);

    repeatDistLabel.setBounds(18, 90, 180, 26);
    repeatDistSlider.setBounds(202, 90, getWidth() - 202 - 18, 26);

    viewport.setBounds(b.reduced(6, 0));

    int rowH = 34;
    int totalContentH = (int)rows.size() * rowH + 8;
    int contentW = viewport.getWidth() - viewport.getScrollBarThickness();
    contentComp->setBounds(0, 0, contentW, totalContentH);

    int curY = 4;
    for (auto& r : rows)
    {
        if (r.swatchButton != nullptr)
        {
            r.swatchButton->setBounds(contentW - 60, curY + 4, 52, 26);
        }
        curY += rowH;
    }

    int btnH = 30;
    resetBtn.setBounds(18, footerArea.getY() + 10, 145, btnH);
    doneBtn.setBounds(footerArea.getRight() - 98, footerArea.getY() + 10, 80, btnH);
}

// ============================================================================
// Overlay Wrapper (Centers modal dialog inside top-level window with dimming)
// ============================================================================

VowelColorCustomizerOverlay::VowelColorCustomizerOverlay(LyricDocument& doc)
    : dialog(doc)
{
    addAndMakeVisible(dialog);
    dialog.onClose = [this] {
        if (auto* p = getParentComponent())
            p->removeChildComponent(this);
        delete this;
    };
}

void VowelColorCustomizerOverlay::paint(juce::Graphics& g)
{
    // Dimmed background
    g.fillAll(juce::Colour(0x75000000));
}

void VowelColorCustomizerOverlay::mouseDown(const juce::MouseEvent& e)
{
    // Clicking outside the dialog card closes it
    if (!dialog.getBounds().contains(e.getPosition()))
    {
        if (dialog.onClose)
            dialog.onClose();
    }
}

void VowelColorCustomizerOverlay::resized()
{
    int w = std::clamp(getWidth() - 48, 380, 530);
    int h = std::clamp(getHeight() - 48, 420, 620);
    dialog.centreWithSize(w, h);
}

void VowelColorCustomizerDialog::showDialog(juce::Component* parent, LyricDocument& doc)
{
    if (parent == nullptr)
        return;

    auto* top = parent->getTopLevelComponent();
    if (top == nullptr)
        return;

    if (auto* existing = top->findChildWithID("VowelColorCustomizerOverlay"))
    {
        existing->toFront(true);
        return;
    }

    auto overlay = std::make_unique<VowelColorCustomizerOverlay>(doc);
    overlay->setComponentID("VowelColorCustomizerOverlay");
    overlay->setBounds(top->getLocalBounds());
    top->addAndMakeVisible(overlay.release());
}

} // namespace CompassCadence
