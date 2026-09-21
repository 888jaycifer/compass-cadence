#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace CompassCadence
{

class NotebookLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NotebookLookAndFeel();
    ~NotebookLookAndFeel() override;

    // Custom notebook colors
    static bool isDarkMode() noexcept { return darkMode; }
    static void setDarkMode(bool dark) noexcept;

    static juce::Colour getPaperColour() noexcept
    {
        return darkMode ? juce::Colour(0xFF18181B) : juce::Colour(0xFFFAF8F2);
    }
    static juce::Colour getEvenLineColour() noexcept
    {
        return darkMode ? juce::Colour(0xFF222227) : juce::Colour(0xFFEFE9DC);
    }
    static juce::Colour getStanzaBreakColour() noexcept
    {
        return darkMode ? juce::Colour(0xFF0E0E10) : juce::Colour(0xFFE2D9C8);
    }
    static juce::Colour getRuleLineColour() noexcept
    {
        return darkMode ? juce::Colour(0x3571717A) : juce::Colour(0x60B0C4DE);
    }
    static juce::Colour getMarginRedColour() noexcept
    {
        return darkMode ? juce::Colour(0xCCEF4444) : juce::Colour(0xCCEF5350);
    }
    static juce::Colour getGraphiteColour() noexcept
    {
        return darkMode ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF262626);
    }
    static juce::Colour getLightGraphiteColour() noexcept
    {
        return darkMode ? juce::Colour(0xFF94A3B8) : juce::Colour(0xFF6B7280);
    }
    static juce::Colour getPulseBoxBorderColour() noexcept
    {
        return darkMode ? juce::Colour(0xFF52525B) : juce::Colour(0xFF3E3E3E);
    }
    static juce::Colour getPulseBoxBackground() noexcept
    {
        return darkMode ? juce::Colour(0xFF27272A) : juce::Colours::transparentBlack;
    }
    static juce::Colour getActivePlayheadColour() noexcept
    {
        return juce::Colour(0xFFFFD54F);
    }
    static juce::Colour getActivePlayheadBorder() noexcept
    {
        return juce::Colour(0xFFFF8F00);
    }

    void updateColours();

    // Renders realistic double-wire spiral binder rings with hole punches and metallic highlights
    static void drawSpiralRings(juce::Graphics& g, float startY, float endY,
                                float spiralX = 18.0f, float ringPitch = 28.8f);

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& scrollbar,
                       int x, int y, int width, int height,
                       bool isScrollbarVertical, int thumbStartPosition,
                       int thumbSize, bool isMouseOver, bool isMouseDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;

    void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override;

    void drawPopupMenuItem(juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted,
                            bool isTicked, bool hasSubMenu,
                            const juce::String& text, const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColour) override;

    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;

private:
    static inline bool darkMode = false;
};

} // namespace CompassCadence
