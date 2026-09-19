#include "NotebookTabBarComponent.h"
#include "NotebookLookAndFeel.h"
#include "../Model/SongManager.h"

namespace CompassCadence
{

NotebookTabBarComponent::NotebookTabBarComponent(CompassCadenceAudioProcessor& proc)
    : processor(proc)
{
    newTabBtn.setTooltip("Add a new blank tab, duplicate current tab (unsynced), or open a song file into a new tab.");
    newTabBtn.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem(1, "New Blank Song / Tab");
        menu.addItem(2, "Duplicate Current Tab (Unsynced)");
        menu.addSeparator();
        menu.addItem(3, "Open Song File in New Tab...");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&newTabBtn), [this](int result)
        {
            if (result == 1)
            {
                processor.addBlankTab();
            }
            else if (result == 2)
            {
                processor.duplicateTab(processor.getActiveTabIndex());
            }
            else if (result == 3)
            {
                promptOpenSongInNewTab();
            }
        });
    };
    addAndMakeVisible(newTabBtn);

    processor.onTabsChanged = [this]
    {
        refreshTabs();
    };

    processor.onActiveTabChanged = [this](int)
    {
        refreshTabs();
    };
}

NotebookTabBarComponent::~NotebookTabBarComponent()
{
}

void NotebookTabBarComponent::refreshTabs()
{
    resized();
    repaint();
}

void NotebookTabBarComponent::resized()
{
    tabLayouts.clear();

    const int totalTabs = processor.getNumTabs();

    // Start tabs just right of the vertical red margin line
    int startX = (int)MARGIN_X + 6;
    int tabY = 4;
    int tabH = getHeight() - tabY; // Flush with the bottom of this bar component

    juce::Font font(juce::FontOptions("Segoe UI", 11.5f, juce::Font::bold));

    int curX = startX;
    for (int i = 0; i < totalTabs; ++i)
    {
        juce::String name = processor.getTabName(i);
        int textW = (int)std::ceil(font.getStringWidth(name));
        int closeBtnW = (totalTabs > 1) ? 18 : 0;
        int tabW = std::clamp(textW + closeBtnW + 24, 75, 200);

        juce::Rectangle<int> tabRect(curX, tabY, tabW, tabH);
        juce::Rectangle<int> closeRect;
        if (totalTabs > 1)
        {
            closeRect = juce::Rectangle<int>(curX + tabW - 18, tabY + (tabH - 14) / 2, 14, 14);
        }

        tabLayouts.push_back({ i, tabRect, closeRect });
        curX += tabW + 4;
    }

    newTabBtn.setBounds(curX + 2, tabY + (tabH - 20) / 2, 22, 20);
}

void NotebookTabBarComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Header/tab shelf background
    g.setColour(NotebookLookAndFeel::getEvenLineColour());
    g.fillRect(bounds);

    // 2. Ruled horizontal line along bottom
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    g.drawLine(0.0f, bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f, 1.0f);

    // 3. Spiral Binder Wire Rings along the far left edge
    NotebookLookAndFeel::drawSpiralRings(g, 0.0f, bounds.getHeight(), 18.0f, 28.8f);

    // 4. Vertical Red Margin Rule
    g.setColour(NotebookLookAndFeel::getMarginRedColour());
    g.drawLine(MARGIN_X, 0.0f, MARGIN_X, bounds.getBottom(), 1.5f);

    // 5. Draw the tabs
    const int activeIdx = processor.getActiveTabIndex();
    const int totalTabs = processor.getNumTabs();

    for (const auto& layout : tabLayouts)
    {
        bool isActive = (layout.index == activeIdx);
        bool isHovered = (layout.index == hoveredTabIndex);
        auto r = layout.bounds.toFloat();

        // Shape of tab: rounded top corners, flat bottom
        juce::Path tabPath;
        tabPath.addRoundedRectangle(r.getX(), r.getY(), r.getWidth(), r.getHeight() + 2.0f,
                                    3.5f, 3.5f, true, true, false, false);

        if (isActive)
        {
            // Active tab has the exact paper color of the page canvas and merges seamlessly downwards
            g.setColour(NotebookLookAndFeel::getPaperColour());
            g.fillPath(tabPath);

            // Border on top, left, right
            g.setColour(NotebookLookAndFeel::getPulseBoxBorderColour());
            g.strokePath(tabPath, juce::PathStrokeType(1.2f));

            // Overwrite the bottom line under the active tab with paper color so it is completely open
            g.setColour(NotebookLookAndFeel::getPaperColour());
            g.drawLine(r.getX() + 0.5f, bounds.getBottom() - 1.0f, r.getRight() - 0.5f, bounds.getBottom() - 1.0f, 2.0f);
        }
        else
        {
            // Inactive tab
            juce::Colour fillCol = isHovered ? NotebookLookAndFeel::getPaperColour().withAlpha(0.6f)
                                             : NotebookLookAndFeel::getEvenLineColour();
            g.setColour(fillCol);
            g.fillPath(tabPath);

            g.setColour(NotebookLookAndFeel::getPulseBoxBorderColour().withAlpha(0.6f));
            g.strokePath(tabPath, juce::PathStrokeType(1.0f));
        }

        // Tab Title Text
        juce::String title = processor.getTabName(layout.index);
        g.setColour(isActive ? NotebookLookAndFeel::getGraphiteColour()
                             : NotebookLookAndFeel::getLightGraphiteColour());
        g.setFont(juce::Font(juce::FontOptions("Segoe UI", 11.5f, isActive ? juce::Font::bold : juce::Font::plain)));

        int textMarginRight = (totalTabs > 1) ? 20 : 8;
        juce::Rectangle<int> textRect(layout.bounds.getX() + 8, layout.bounds.getY(),
                                      layout.bounds.getWidth() - textMarginRight - 8, layout.bounds.getHeight());
        g.drawFittedText(title, textRect, juce::Justification::centredLeft, 1);

        // Close button '×'
        if (totalTabs > 1 && (isActive || isHovered))
        {
            bool closeHovered = (layout.index == hoveredCloseTabIndex);
            auto cr = layout.closeBounds.toFloat();

            if (closeHovered)
            {
                g.setColour(juce::Colour(0x22EF4444)); // Faint red hover circle
                g.fillEllipse(cr);
            }

            g.setColour(closeHovered ? juce::Colour(0xFFDC2626)
                                     : NotebookLookAndFeel::getLightGraphiteColour());
            g.setFont(juce::Font(juce::FontOptions("Segoe UI", 11.0f, juce::Font::bold)));
            g.drawFittedText("x", layout.closeBounds, juce::Justification::centred, 1);
        }
    }
}

void NotebookTabBarComponent::mouseDown(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();

    for (const auto& layout : tabLayouts)
    {
        if (layout.bounds.contains(pos))
        {
            if (e.mods.isPopupMenu())
            {
                showTabMenu(layout.index, e.getScreenPosition());
                return;
            }

            // Check if clicked close '×'
            if (processor.getNumTabs() > 1 && layout.closeBounds.contains(pos))
            {
                processor.closeTab(layout.index);
                return;
            }

            // Otherwise activate tab
            processor.setActiveTabIndex(layout.index);
            return;
        }
    }

    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "New Blank Song / Tab");
        menu.addItem(2, "Duplicate Current Tab (Unsynced)");
        menu.addSeparator();
        menu.addItem(3, "Open Song File in New Tab...");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ e.getScreenPosition(), e.getScreenPosition() }), [this](int result)
        {
            if (result == 1) processor.addBlankTab();
            else if (result == 2) processor.duplicateTab(processor.getActiveTabIndex());
            else if (result == 3) promptOpenSongInNewTab();
        });
    }
}

void NotebookTabBarComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    for (const auto& layout : tabLayouts)
    {
        if (layout.bounds.contains(pos))
        {
            if (!layout.closeBounds.contains(pos))
            {
                promptRenameTab(layout.index);
                return;
            }
        }
    }
}

void NotebookTabBarComponent::mouseMove(const juce::MouseEvent& e)
{
    auto pos = e.getPosition();
    int newHovered = -1;
    int newCloseHovered = -1;

    for (const auto& layout : tabLayouts)
    {
        if (layout.bounds.contains(pos))
        {
            newHovered = layout.index;
            if (layout.closeBounds.contains(pos))
                newCloseHovered = layout.index;
            break;
        }
    }

    if (newHovered != hoveredTabIndex || newCloseHovered != hoveredCloseTabIndex)
    {
        hoveredTabIndex = newHovered;
        hoveredCloseTabIndex = newCloseHovered;
        repaint();
    }
}

void NotebookTabBarComponent::mouseExit(const juce::MouseEvent&)
{
    if (hoveredTabIndex != -1 || hoveredCloseTabIndex != -1)
    {
        hoveredTabIndex = -1;
        hoveredCloseTabIndex = -1;
        repaint();
    }
}

void NotebookTabBarComponent::showTabMenu(int tabIndex, juce::Point<int> screenPos)
{
    juce::PopupMenu menu;
    menu.addItem(1, "Duplicate Tab (Unsynced)");
    menu.addItem(2, "Rename Tab...");
    menu.addSeparator();
    menu.addItem(3, "Load Song File into This Tab...");
    menu.addItem(4, "Save This Tab to Song File...");
    menu.addSeparator();
    menu.addItem(5, "Close Tab", processor.getNumTabs() > 1);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ screenPos, screenPos }), [this, tabIndex](int result)
    {
        if (result == 1)
        {
            processor.duplicateTab(tabIndex);
        }
        else if (result == 2)
        {
            promptRenameTab(tabIndex);
        }
        else if (result == 3)
        {
            promptLoadSongIntoTab(tabIndex);
        }
        else if (result == 4)
        {
            promptSaveTabToSong(tabIndex);
        }
        else if (result == 5)
        {
            processor.closeTab(tabIndex);
        }
    });
}

void NotebookTabBarComponent::promptRenameTab(int tabIndex)
{
    juce::String currentName = processor.getTabName(tabIndex);
    auto* aw = new juce::AlertWindow("Rename Tab", "Enter new name for tab:", juce::AlertWindow::NoIcon);
    aw->addTextEditor("name", currentName, "Tab name:");
    aw->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, tabIndex, aw](int result)
    {
        if (result == 1)
        {
            juce::String newName = aw->getTextEditorContents("name").trim();
            if (newName.isNotEmpty())
            {
                processor.setTabName(tabIndex, newName);
            }
        }
        delete aw;
    }));
}

void NotebookTabBarComponent::promptSaveTabToSong(int tabIndex)
{
    juce::String defaultName = processor.getTabName(tabIndex);
    auto* aw = new juce::AlertWindow("Save Tab to Song", "Save this tab as a .cadence song file:", juce::AlertWindow::NoIcon);
    aw->addTextEditor("name", defaultName, "Song name:");
    aw->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, tabIndex, aw](int result)
    {
        if (result == 1)
        {
            juce::String songName = aw->getTextEditorContents("name").trim();
            if (songName.isNotEmpty())
            {
                SongManager sm;
                sm.saveSong(songName, processor.getLyricDocument());
                processor.setTabName(tabIndex, songName);
            }
        }
        delete aw;
    }));
}

void NotebookTabBarComponent::promptLoadSongIntoTab(int tabIndex)
{
    SongManager sm;
    auto names = sm.getSavedSongNames();
    if (names.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "No Saved Songs",
                                              "No saved .cadence songs found in:\n" + sm.getSongsDirectory().getFullPathName());
        return;
    }

    juce::PopupMenu menu;
    for (int i = 0; i < (int)names.size(); ++i)
    {
        menu.addItem(i + 1, names[i]);
    }

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, tabIndex, names, sm](int result)
    {
        if (result > 0 && result <= (int)names.size())
        {
            auto file = sm.getSongsDirectory().getChildFile(names[result - 1] + ".cadence");
            processor.loadSongIntoTab(tabIndex, file);
        }
    });
}

void NotebookTabBarComponent::promptOpenSongInNewTab()
{
    SongManager sm;
    auto names = sm.getSavedSongNames();
    if (names.empty())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "No Saved Songs",
                                              "No saved .cadence songs found in:\n" + sm.getSongsDirectory().getFullPathName());
        return;
    }

    juce::PopupMenu menu;
    for (int i = 0; i < (int)names.size(); ++i)
    {
        menu.addItem(i + 1, names[i]);
    }

    menu.showMenuAsync(juce::PopupMenu::Options(), [this, names, sm](int result)
    {
        if (result > 0 && result <= (int)names.size())
        {
            auto file = sm.getSongsDirectory().getChildFile(names[result - 1] + ".cadence");
            processor.openSongInNewTab(file);
        }
    });
}

} // namespace CompassCadence
