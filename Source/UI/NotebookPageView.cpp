#include "NotebookPageView.h"
#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

NotebookPageContent::NotebookPageContent(LyricDocument& doc, float marginLineX)
    : document(doc), marginX(marginLineX)
{
    rebuildBars();
}

NotebookPageContent::~NotebookPageContent()
{
}

void NotebookPageContent::rebuildBars()
{
    barLines.clear();
    barYPositions.clear();

    int startBar = document.getVisibleStartBar();
    int endBar = document.getVisibleEndBar();

    for (int b = startBar; b < endBar; ++b)
    {
        auto line = std::make_unique<BarLineComponent>(document, b, marginX);
        line->setBarNavListener(this);
        addAndMakeVisible(line.get());
        barLines.push_back(std::move(line));
    }

    resized();
    repaint();
}

void NotebookPageContent::appendBars(int newEndBar)
{
    int currentCount = (int)barLines.size();
    int startBar = barLines.empty() ? 0 : barLines.front()->getBarIndex();
    int currentEndBar = startBar + currentCount;

    for (int b = currentEndBar; b < newEndBar; ++b)
    {
        auto line = std::make_unique<BarLineComponent>(document, b, marginX);
        line->setBarNavListener(this);
        addAndMakeVisible(line.get());
        barLines.push_back(std::move(line));
    }

    resized();
    repaint();
}

void NotebookPageContent::updateContent()
{
    repaint();
    for (auto& bl : barLines)
    {
        bl->updateContent();
        bl->repaint();
    }
}

void NotebookPageContent::updateSelection()
{
    repaint();
    for (auto& bl : barLines)
    {
        bl->updateSelection();
        bl->repaint();
    }
}

void NotebookPageContent::updatePlayhead(const PlayheadLocation& loc, bool isPlaying)
{
    for (auto& bl : barLines)
    {
        int bIdx = bl->getBarIndex();
        if (bIdx == loc.bar)
        {
            bl->setPlayheadActive(true, loc.barProgress, isPlaying);
        }
        else
        {
            bl->setPlayheadActive(false, 0.0, false);
        }
    }
}

int NotebookPageContent::getBarY(int barIndex) const
{
    auto it = barYPositions.find(barIndex);
    if (it != barYPositions.end())
        return it->second;
    return 0;
}

void NotebookPageContent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Base notebook paper background
    g.setColour(NotebookLookAndFeel::getPaperColour());
    g.fillRect(bounds);

    // 2. Alternating Line Variable Shading & Stanza Breaks
    for (const auto& pair : barYPositions)
    {
        int barIndex = pair.first;
        float y = (float)pair.second;
        float h = (float)barHeight;

        // Even numbered bars (1-based Bar 2, Bar 4, Bar 6...) shaded darker than odd
        if ((barIndex + 1) % 2 == 0)
        {
            g.setColour(NotebookLookAndFeel::getEvenLineColour());
            g.fillRect(0.0f, y, bounds.getRight(), h);
        }

        // Stanza breaks between stanzas shaded a different shade from either
        if (document.shouldAddSpacingAfterBar(barIndex))
        {
            float extraY = y + h;
            float extraH = (float)blankLineGap;
            g.setColour(NotebookLookAndFeel::getStanzaBreakColour());
            g.fillRect(0.0f, extraY, bounds.getRight(), extraH);
        }
    }

    // 3. Ruled horizontal faint lines
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    for (const auto& pair : barYPositions)
    {
        float y = (float)(pair.second + barHeight);
        g.drawLine(0.0f, y, bounds.getRight(), y, 1.0f);

        // If extra line spacing exists after this bar, draw a second blank rule line
        if (document.shouldAddSpacingAfterBar(pair.first))
        {
            float extraY = y + (float)blankLineGap;
            g.drawLine(0.0f, extraY, bounds.getRight(), extraY, 1.0f);
        }
    }

    // 4. Vertical Red Margin Rule
    g.setColour(NotebookLookAndFeel::getMarginRedColour());
    g.drawLine(marginX, 0.0f, marginX, bounds.getBottom(), 1.5f);

    // 5. Spiral Binder Wire Rings along the far left edge (seamless continuation from header)
    NotebookLookAndFeel::drawSpiralRings(g, 24.4f, bounds.getHeight(), 18.0f, 28.8f);
}

void NotebookPageContent::resized()
{
    barYPositions.clear();
    int currentY = 10;
    int w = getWidth();

    for (auto& bl : barLines)
    {
        int bIdx = bl->getBarIndex();
        barYPositions[bIdx] = currentY;

        bl->setBounds(0, currentY, w, barHeight);
        currentY += barHeight;

        // Check if extra line spacing requested
        if (document.shouldAddSpacingAfterBar(bIdx))
        {
            currentY += blankLineGap;
        }
    }

    int totalHeight = std::max(currentY + 20, getParentComponent() ? getParentComponent()->getHeight() : 600);
    if (!isUpdatingSize && getHeight() != totalHeight)
    {
        isUpdatingSize = true;
        setSize(w, totalHeight);
        isUpdatingSize = false;
    }
}

void NotebookPageContent::barNotationChanged(int barIndex)
{
    for (auto& bl : barLines)
    {
        if (bl->getBarIndex() == barIndex)
        {
            bl->rebuildPulses();
            break;
        }
    }
}

void NotebookPageContent::onBarJumpToCell(int targetBar, int targetSyl)
{
    int totalSyls = document.getNotation(targetBar).getTotalSyllables();
    while (targetSyl >= totalSyls)
    {
        targetBar++;
        targetSyl -= totalSyls;
        totalSyls = document.getNotation(targetBar).getTotalSyllables();
    }

    if (targetSyl < 0)
    {
        while (targetSyl < 0 && targetBar > 0)
        {
            targetBar--;
            totalSyls = document.getNotation(targetBar).getTotalSyllables();
            targetSyl += totalSyls;
        }
        if (targetSyl < 0) targetSyl = 0;
    }

    if (document.getViewMode() == LyricDocument::ModeScroll)
    {
        if (targetBar >= document.getBarCount())
        {
            document.ensureBarCount(targetBar + 16);
        }
        if (auto* pv = findParentComponentOfClass<NotebookPageView>())
        {
            pv->scrollToBar(targetBar);
        }
    }
    else
    {
        // Check if target is on current page
        int startBar = document.getFirstBarOfCurrentPage();
        int endBar = document.getLastBarOfCurrentPage();

        if (targetBar < startBar || targetBar >= endBar)
        {
            // Switch page
            int newPage = targetBar / document.getBarsPerPage();
            document.setCurrentPage(newPage);
        }
    }

    for (auto& bl : barLines)
    {
        if (bl->getBarIndex() == targetBar)
        {
            bl->focusSyllable(targetSyl);
            break;
        }
    }
}

void NotebookPageContent::onBarNavigateSyllable(int barIdx, int globalSylIdx, bool forward)
{
    int targetBar = barIdx;
    int targetSyl = forward ? (globalSylIdx + 1) : (globalSylIdx - 1);
    onBarJumpToCell(targetBar, targetSyl);
}

void NotebookPageContent::onBarEnterNext(int barIdx)
{
    int totalSyls = document.getNotation(barIdx).getTotalSyllables();
    onBarNavigateSyllable(barIdx, totalSyls - 1, true);
}

// -----------------------------------------------------------------------------
// NotebookPageView
// -----------------------------------------------------------------------------

NotebookPageView::NotebookPageView(CompassCadenceAudioProcessor& proc, LyricDocument& doc)
    : processor(proc), document(&doc)
{
    document->addListener(this);

    pageContent = std::make_unique<NotebookPageContent>(*document, MARGIN_X);
    viewport.setViewedComponent(pageContent.get(), false);
    viewport.setScrollBarsShown(true, false); // Vertical scroll only
    viewport.setScrollBarThickness(10);
    viewport.onVisibleAreaChanged = [this] {
        checkInfiniteScroll();
    };
    addAndMakeVisible(viewport);
}

NotebookPageView::~NotebookPageView()
{
    if (document != nullptr)
        document->removeListener(this);
    viewport.setViewedComponent(nullptr, false);
}

void NotebookPageView::setDocument(LyricDocument& newDoc)
{
    if (document == &newDoc)
        return;

    if (document != nullptr)
        document->removeListener(this);

    document = &newDoc;
    document->addListener(this);

    pageContent = std::make_unique<NotebookPageContent>(*document, MARGIN_X);
    viewport.setViewedComponent(pageContent.get(), false);
    resized();
    repaint();
}

void NotebookPageView::updatePlayhead(const PlayheadLocation& loc, bool isPlaying, bool followDAW)
{
    if (document == nullptr)
        return;

    if (pageContent != nullptr)
    {
        pageContent->updatePlayhead(loc, isPlaying);

        if (followDAW && isPlaying)
        {
            if (document->getViewMode() == LyricDocument::ModePages)
            {
                // Auto-switch page if playhead moved outside current page during playback
                int barPage = loc.bar / document->getBarsPerPage();
                if (barPage != document->getCurrentPage())
                {
                    document->setCurrentPage(barPage);
                    if (auto* param = dynamic_cast<juce::AudioParameterInt*>(processor.getAPVTS().getParameter("page")))
                    {
                        *param = barPage;
                    }
                }
            }

            // Scroll to keep active bar visible in viewport
            scrollToBar(loc.bar);
        }

        if (document->getViewMode() == LyricDocument::ModeScroll)
        {
            checkInfiniteScroll();
        }
    }
}

void NotebookPageView::scrollToBar(int barIndex)
{
    if (pageContent == nullptr)
        return;

    int barY = pageContent->getBarY(barIndex);
    int barH = pageContent->getBarHeight();
    int viewH = viewport.getHeight();

    int targetY = barY - (viewH / 2) + (barH / 2);
    if (targetY < 0) targetY = 0;
    int maxY = std::max(0, pageContent->getHeight() - viewH);
    if (targetY > maxY) targetY = maxY;

    int curY = viewport.getViewPositionY();
    // Smooth dampening towards target
    int diff = targetY - curY;
    if (std::abs(diff) > 2)
    {
        viewport.setViewPosition(0, curY + (diff / 3));
    }
}

void NotebookPageView::scrollToNormalized(float normY)
{
    if (pageContent == nullptr)
        return;

    int maxScroll = std::max(0, pageContent->getHeight() - viewport.getHeight());
    int targetY = (int)(std::clamp(normY, 0.0f, 1.0f) * (float)maxScroll);
    viewport.setViewPosition(0, targetY);
}

void NotebookPageView::scrollByBars(int numBars)
{
    if (pageContent == nullptr) return;
    int curY = viewport.getViewPositionY();
    int deltaY = numBars * pageContent->getBarHeight();
    viewport.setViewPosition(0, std::max(0, curY + deltaY));
}

void NotebookPageView::checkInfiniteScroll()
{
    if (pageContent == nullptr || document == nullptr || document->getViewMode() != LyricDocument::ModeScroll)
        return;

    int curY = viewport.getViewPositionY();
    int viewH = viewport.getHeight();
    int contentH = pageContent->getHeight();

    // When scrolling within 120px of the bottom, append next 16 bars
    if (contentH > 0 && curY + viewH >= contentH - 120 && document->getBarCount() < 1024)
    {
        document->ensureBarCount(document->getBarCount() + 16);
    }
}

void NotebookPageView::paint(juce::Graphics& g)
{
    g.fillAll(NotebookLookAndFeel::getPaperColour());
}

void NotebookPageView::resized()
{
    viewport.setBounds(getLocalBounds());
    if (pageContent != nullptr)
    {
        pageContent->setSize(viewport.getWidth() - viewport.getScrollBarThickness(), pageContent->getHeight());
        pageContent->resized();
    }
}

bool NotebookPageView::keyPressed(const juce::KeyPress& key)
{
    if (document == nullptr)
        return false;

    // Ctrl+Z: Document Undo
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        document->undo();
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
        document->redo();
        return true;
    }

    // Ctrl+J: Join selected cells
    if (key == juce::KeyPress('j', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('j', juce::ModifierKeys::commandModifier, 0))
    {
        document->joinSelected();
        return true;
    }

    // Ctrl+C: Copy selected / highlighted text to system clipboard
    if (key == juce::KeyPress('c', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
    {
        juce::String text = document->getSelectedText();
        if (text.isEmpty())
            text = document->getAllText();
        if (text.isNotEmpty())
            juce::SystemClipboard::copyTextToClipboard(text);
        return true;
    }

    // Ctrl+Left / Cmd+Left: Align Left
    if (key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier, 0))
    {
        document->setSelectionAlignment(LyricDocument::AlignLeft);
        return true;
    }

    // Ctrl+Right / Cmd+Right: Align Right
    if (key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier, 0))
    {
        document->setSelectionAlignment(LyricDocument::AlignRight);
        return true;
    }

    // Ctrl+Up / Ctrl+Down: Align Center
    if (key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier, 0))
    {
        document->setSelectionAlignment(LyricDocument::AlignCenter);
        return true;
    }

    // Ctrl+K or Ctrl+Shift+S: Split selected cells
    if (key == juce::KeyPress('k', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress('s', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0) ||
        key == juce::KeyPress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        document->splitSelected();
        return true;
    }

    // Delete or Backspace: Clear selected cells
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        document->deleteSelected();
        return true;
    }

    // Escape: Clear selection
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        document->clearSelection();
        return true;
    }

    return false;
}

void NotebookPageView::lyricDocumentChanged()
{
    if (document == nullptr)
        return;

    if (pageContent != nullptr)
    {
        int startBar = document->getVisibleStartBar();
        int endBar = document->getVisibleEndBar();
        int expectedCount = endBar - startBar;

        // Check if page range has changed or count has changed
        if (pageContent->getDisplayedStartBar() != startBar ||
            pageContent->getDisplayedBarCount() != expectedCount)
        {
            int savedScrollY = viewport.getViewPositionY();

            // Seamless infinite expansion in Scroll mode: append newly added bars without rebuilding or losing focus
            if (document->getViewMode() == LyricDocument::ModeScroll &&
                pageContent->getDisplayedStartBar() == startBar &&
                pageContent->getDisplayedBarCount() < expectedCount)
            {
                pageContent->appendBars(endBar);
            }
            else
            {
                pageContent->rebuildBars();
                if (document->getViewMode() == LyricDocument::ModeScroll)
                    viewport.setViewPosition(0, savedScrollY);
                else
                    viewport.setViewPosition(0, 0); // Reset scroll to top of new page
            }
        }
        else
        {
            pageContent->updateContent();
        }

        pageContent->repaint();
    }

    repaint();
}

void NotebookPageView::metricNotationChanged(const MetricNotation&)
{
    if (pageContent != nullptr)
    {
        pageContent->rebuildBars();
    }
}

void NotebookPageView::barNotationChanged(int barIndex, const MetricNotation&)
{
    if (pageContent != nullptr)
    {
        pageContent->barNotationChanged(barIndex);
    }
}

void NotebookPageView::selectionChanged()
{
    if (pageContent != nullptr)
    {
        pageContent->updateSelection();
    }
}

} // namespace CompassCadence
