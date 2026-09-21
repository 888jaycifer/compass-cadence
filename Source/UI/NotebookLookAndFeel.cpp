#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

static NotebookLookAndFeel* gActiveNotebookLookAndFeel = nullptr;

NotebookLookAndFeel::NotebookLookAndFeel()
{
    gActiveNotebookLookAndFeel = this;
    updateColours();
}

NotebookLookAndFeel::~NotebookLookAndFeel()
{
    if (gActiveNotebookLookAndFeel == this)
        gActiveNotebookLookAndFeel = nullptr;
}

void NotebookLookAndFeel::setDarkMode(bool dark) noexcept
{
    darkMode = dark;
    if (gActiveNotebookLookAndFeel != nullptr)
        gActiveNotebookLookAndFeel->updateColours();
}

void NotebookLookAndFeel::updateColours()
{
    setColour(juce::ResizableWindow::backgroundColourId, getPaperColour());
    setColour(juce::Label::textColourId, getGraphiteColour());
    setColour(juce::Label::textWhenEditingColourId, getGraphiteColour());
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);

    setColour(juce::TextEditor::backgroundColourId, isDarkMode() ? juce::Colour(0xFF27272A) : juce::Colour(0xFFFFFFFF));
    setColour(juce::TextEditor::textColourId, getGraphiteColour());
    setColour(juce::TextEditor::highlightColourId, isDarkMode() ? juce::Colour(0x603B82F6) : juce::Colour(0x60FFF59D));
    setColour(juce::TextEditor::highlightedTextColourId, getGraphiteColour());
    setColour(juce::TextEditor::outlineColourId, isDarkMode() ? juce::Colour(0xFF52525B) : juce::Colour(0xFFCBD5E1));
    setColour(juce::TextEditor::focusedOutlineColourId, isDarkMode() ? juce::Colour(0xFFF59E0B) : juce::Colour(0xFFD97706));

    setColour(juce::ComboBox::backgroundColourId, isDarkMode() ? juce::Colour(0xFF27272A) : juce::Colour(0xFFF3EFE6));
    setColour(juce::ComboBox::textColourId, getGraphiteColour());
    setColour(juce::ComboBox::outlineColourId, getLightGraphiteColour().withAlpha(0.6f));
    setColour(juce::ComboBox::arrowColourId, getGraphiteColour());

    setColour(juce::PopupMenu::backgroundColourId, isDarkMode() ? juce::Colour(0xFF27272A) : getPaperColour());
    setColour(juce::PopupMenu::textColourId, getGraphiteColour());
    setColour(juce::PopupMenu::highlightedBackgroundColourId, isDarkMode() ? juce::Colour(0xFF3F3F46) : juce::Colour(0x60FFF59D));
    setColour(juce::PopupMenu::highlightedTextColourId, getGraphiteColour());
}

void NotebookLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(backgroundColour);
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    juce::Colour base;
    if (button.getToggleState())
    {
        // Toggled ON: Soft stationery highlighter wash in light, warm amber glow in dark
        base = darkMode ? juce::Colour(0xFFB45309) : juce::Colour(0xFFFFF176);
    }
    else
    {
        // Normal state: warm stationery paper tab tone in light, dark tile in dark
        base = darkMode ? juce::Colour(0xFF27272A) : juce::Colour(0xFFF2EFE7);
    }

    if (shouldDrawButtonAsDown)
        base = base.darker(0.12f);
    else if (shouldDrawButtonAsHighlighted)
        base = base.brighter(0.06f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 3.0f);

    // Fine pencil graphite border
    juce::Colour borderCol = button.getToggleState()
        ? (darkMode ? juce::Colour(0xFFF59E0B) : juce::Colour(0xFFD97706))
        : (darkMode ? juce::Colour(0xFF52525B) : getLightGraphiteColour().withAlpha(0.65f));

    g.setColour(borderCol);
    g.drawRoundedRectangle(bounds, 3.0f, button.getToggleState() ? 1.4f : 1.0f);
}

void NotebookLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                        bool shouldDrawButtonAsHighlighted,
                                        bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused(shouldDrawButtonAsHighlighted);
    juce::Font font = getTextButtonFont(button, button.getHeight());
    g.setFont(font);

    // Crisp lettering
    juce::Colour textCol = (button.getToggleState() && darkMode) ? juce::Colours::white : getGraphiteColour();
    if (shouldDrawButtonAsDown)
        textCol = textCol.withAlpha(0.7f);

    g.setColour(textCol);
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(4, 2),
                     juce::Justification::centred, 1);
}

void NotebookLookAndFeel::drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                                       int x, int y, int width, int height,
                                       bool isScrollbarVertical, int thumbStartPosition,
                                       int thumbSize, bool isMouseOver, bool isMouseDown)
{
    juce::ignoreUnused(scrollbar, isScrollbarVertical);

    // Clean minimal stationery scrollbar
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
    g.setColour(juce::Colour(0x20000000));
    g.fillRect(bounds);

    juce::Rectangle<float> thumbBounds;
    if (isScrollbarVertical)
    {
        thumbBounds = juce::Rectangle<float>((float)x + 2.0f,
                                             (float)thumbStartPosition,
                                             (float)width - 4.0f,
                                             (float)thumbSize);
    }
    else
    {
        thumbBounds = juce::Rectangle<float>((float)thumbStartPosition,
                                             (float)y + 2.0f,
                                             (float)thumbSize,
                                             (float)height - 4.0f);
    }

    juce::Colour thumbCol = isMouseDown ? juce::Colour(0xFF555555)
                          : isMouseOver ? juce::Colour(0xFF777777)
                          : juce::Colour(0xFFAAAAAA);

    g.setColour(thumbCol);
    g.fillRoundedRectangle(thumbBounds, 3.0f);

    if (isScrollbarVertical && thumbSize >= 22)
    {
        // Top and bottom DAW-style thumb resize indicators
        float ribW = std::clamp(thumbBounds.getWidth() * 0.60f, 4.0f, 10.0f);
        float ribX = thumbBounds.getCentreX() - (ribW * 0.5f);
        juce::Colour ribCol = isMouseDown || isMouseOver
            ? (darkMode ? juce::Colour(0xFFF59E0B) : juce::Colour(0xFFD97706))
            : (darkMode ? juce::Colour(0x80F1F5F9) : juce::Colour(0x60000000));
        g.setColour(ribCol);

        // Top edge ribs
        float topY1 = (float)thumbStartPosition + 3.5f;
        float topY2 = (float)thumbStartPosition + 5.5f;
        g.drawLine(ribX, topY1, ribX + ribW, topY1, 1.0f);
        g.drawLine(ribX, topY2, ribX + ribW, topY2, 1.0f);

        // Bottom edge ribs
        float botY1 = (float)thumbStartPosition + (float)thumbSize - 6.5f;
        float botY2 = (float)thumbStartPosition + (float)thumbSize - 4.5f;
        g.drawLine(ribX, botY1, ribX + ribW, botY1, 1.0f);
        g.drawLine(ribX, botY2, ribX + ribW, botY2, 1.0f);
    }
}

void NotebookLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                                      int buttonX, int buttonY, int buttonW, int buttonH,
                                      juce::ComboBox& box)
{
    juce::ignoreUnused(buttonX, buttonY, buttonW, buttonH, isButtonDown);
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat().reduced(1.0f);

    g.setColour(darkMode ? juce::Colour(0xFF27272A) : box.findColour(juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(darkMode ? juce::Colour(0xFF52525B) : box.findColour(juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);

    // Draw little pencil arrow
    juce::Path p;
    float arrowX = (float)width - 14.0f;
    float arrowY = (float)height * 0.5f - 2.0f;
    p.startNewSubPath(arrowX, arrowY);
    p.lineTo(arrowX + 8.0f, arrowY);
    p.lineTo(arrowX + 4.0f, arrowY + 5.0f);
    p.closeSubPath();

    g.setColour(darkMode ? juce::Colour(0xFFE4E4E7) : box.findColour(juce::ComboBox::arrowColourId));
    g.fillPath(p);
}

void NotebookLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
    g.setColour(darkMode ? juce::Colour(0xFF202024) : getPaperColour());
    g.fillRect(bounds);

    g.setColour(darkMode ? juce::Colour(0xFF52525B) : getLightGraphiteColour());
    g.drawRect(bounds, 1.0f);
}

void NotebookLookAndFeel::drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                                            bool isSeparator, bool isActive, bool isHighlighted,
                                            bool isTicked, bool hasSubMenu,
                                            const juce::String& text, const juce::String& shortcutKeyText,
                                            const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    juce::ignoreUnused(textColourToUse);

    if (isSeparator)
    {
        auto r = area.reduced(5, 0);
        r.removeFromTop(juce::roundToInt(((float)r.getHeight() * 0.5f) - 0.5f));
        g.setColour(darkMode ? juce::Colour(0xFF3F3F46) : getRuleLineColour());
        g.fillRect(r.removeFromTop(1));
    }
    else
    {
        auto r = area.reduced(1);

        if (isHighlighted && isActive)
        {
            // Amber highlighter tone in dark, soft paper wash in light
            g.setColour(darkMode ? juce::Colour(0xFFB45309) : juce::Colour(0x60FFF59D));
            g.fillRect(r);
            g.setColour(darkMode ? juce::Colours::white : juce::Colour(0xFF1E293B));
        }
        else
        {
            // High-contrast text color
            juce::Colour normalText = darkMode ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF262626);
            g.setColour(normalText.withMultipliedAlpha(isActive ? 1.0f : 0.45f));
        }

        r.reduce(std::min(6, area.getWidth() / 20), 0);

        auto font = juce::Font(juce::FontOptions("Segoe UI", 13.5f, juce::Font::plain));
        float maxFontHeight = (float)r.getHeight() / 1.3f;
        if (font.getHeight() > maxFontHeight)
            font.setHeight(maxFontHeight);
        g.setFont(font);

        auto iconArea = r.removeFromLeft(juce::roundToInt(maxFontHeight)).toFloat();

        if (icon != nullptr)
        {
            icon->drawWithin(g, iconArea, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
            r.removeFromLeft(juce::roundToInt(maxFontHeight * 0.5f));
        }
        else if (isTicked)
        {
            // Crisp checkmark in playhead gold
            g.setColour(darkMode ? juce::Colour(0xFFFBBF24) : juce::Colour(0xFFB45309));
            auto tick = getTickShape(1.0f);
            g.fillPath(tick, tick.getTransformToScaleToFit(iconArea.reduced(iconArea.getWidth() / 5, 0).toFloat(), true));
            if (isHighlighted && isActive)
                g.setColour(darkMode ? juce::Colours::white : juce::Colour(0xFF1E293B));
            else
                g.setColour(darkMode ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF262626));
        }

        if (hasSubMenu)
        {
            auto arrowH = 0.6f * font.getAscent();
            auto x = (float)r.removeFromRight((int)arrowH).getX();
            auto halfH = (float)r.getCentreY();

            juce::Path path;
            path.startNewSubPath(x, halfH - arrowH * 0.5f);
            path.lineTo(x + arrowH * 0.6f, halfH);
            path.lineTo(x, halfH + arrowH * 0.5f);
            g.strokePath(path, juce::PathStrokeType(1.5f));
        }

        r.removeFromRight(3);
        g.drawFittedText(text, r, juce::Justification::centredLeft, 1);

        if (shortcutKeyText.isNotEmpty())
        {
            g.setColour(getLightGraphiteColour());
            g.drawFittedText(shortcutKeyText, r, juce::Justification::centredRight, 1);
        }
    }
}

juce::Font NotebookLookAndFeel::getTextButtonFont(juce::TextButton&, int buttonHeight)
{
    return juce::Font(juce::FontOptions("Segoe UI", (float)buttonHeight * 0.5f, juce::Font::bold));
}

void NotebookLookAndFeel::drawSpiralRings(juce::Graphics& g, float startY, float endY,
                                         float spiralX, float ringPitch)
{
    int numRings = (int)((endY - startY) / ringPitch) + 2;

    for (int i = 0; i < numRings; ++i)
    {
        float ringY = startY + (float)i * ringPitch;
        if (ringY > endY + 8.0f)
            break;

        // Hole punch with inner shadow
        g.setColour(darkMode ? juce::Colour(0xFF09090B) : juce::Colour(0x50000000));
        g.fillEllipse(spiralX - 4.0f, ringY - 4.0f, 10.0f, 10.0f);

        g.setColour(darkMode ? juce::Colour(0xFF18181B) : juce::Colour(0xFF333333));
        g.fillEllipse(spiralX - 3.0f, ringY - 3.0f, 8.0f, 8.0f);

        // Double wire metallic loop
        juce::Path ringPath;
        ringPath.startNewSubPath(0.0f, ringY - 3.0f);
        ringPath.cubicTo(spiralX * 0.7f, ringY - 6.0f,
                         spiralX + 5.0f, ringY - 6.0f,
                         spiralX + 1.0f, ringY + 1.0f);

        // Metallic chrome silver highlight
        g.setColour(juce::Colour(0xFFD4D4D8));
        g.strokePath(ringPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Wire graphite shadow
        ringPath.clear();
        ringPath.startNewSubPath(0.0f, ringY - 1.0f);
        ringPath.cubicTo(spiralX * 0.7f, ringY - 4.0f,
                         spiralX + 5.0f, ringY - 4.0f,
                         spiralX + 1.0f, ringY + 3.0f);
        g.setColour(juce::Colour(0xFF71717A));
        g.strokePath(ringPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

} // namespace CompassCadence

