#include "NotebookPageView.h"
#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

NotebookPageContent::NotebookPageContent(LyricDocument& doc, float marginLineX)
    : document(doc), marginX(marginLineX)
{
    barHeight = document.getBarHeight();
    rebuildBars();
}

NotebookPageContent::~NotebookPageContent()
{
}

void NotebookPageContent::setBarHeight(int h)
{
    int clamped = std::clamp(h, 32, 100);
    if (barHeight != clamped)
    {
        barHeight = clamped;
        resized();
        repaint();
    }
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

void NotebookPageContent::mouseExit(const juce::MouseEvent& e)
{
    if (!getLocalBounds().contains(e.getPosition()))
    {
        SyllableCellComponent::clearGlobalHoveredCell();
    }
}

void NotebookPageContent::barNotationChanged(int barIndex)
{
    for (auto& bl : barLines)
    {
        if (bl->getBarIndex() == barIndex)
        {
            bl->rebuildPulses();
            const auto& sel = document.getSelectedCells();
            if (sel.size() == 1 && sel.begin()->first == barIndex)
            {
                bl->focusSyllable(sel.begin()->second);
            }
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
// NotebookVerticalScrollBar
// -----------------------------------------------------------------------------

NotebookVerticalScrollBar::NotebookVerticalScrollBar()
    : juce::ScrollBar(true)
{
}

NotebookVerticalScrollBar::~NotebookVerticalScrollBar()
{
}

juce::Rectangle<int> NotebookVerticalScrollBar::getThumbBounds() const
{
    auto rangeLen = getMaximumRangeLimit() - getMinimumRangeLimit();
    auto viewStart = getCurrentRangeStart() - getMinimumRangeLimit();
    auto viewLength = getCurrentRange().getLength();
    int H = getHeight();

    if (rangeLen <= 0.0 || viewLength >= rangeLen || H <= 20)
        return {};

    auto& lf = getLookAndFeel();
    int minThumb = lf.getMinimumScrollbarThumbSize(const_cast<NotebookVerticalScrollBar&>(*this));
    int thumbH = juce::roundToInt((viewLength * H) / rangeLen);
    thumbH = std::clamp(thumbH, minThumb, H);

    int thumbTop = 0;
    if (rangeLen > viewLength)
    {
        thumbTop = juce::roundToInt((viewStart * (H - thumbH)) / (rangeLen - viewLength));
    }
    thumbTop = std::clamp(thumbTop, 0, H - thumbH);

    return { 0, thumbTop, getWidth(), thumbH };
}

NotebookVerticalScrollBar::ResizeEdge NotebookVerticalScrollBar::getResizeEdgeAt(int mouseY) const
{
    auto thumb = getThumbBounds();
    if (thumb.isEmpty())
        return ResizeEdge::None;

    int thumbTop = thumb.getY();
    int thumbBottom = thumb.getBottom();
    int thumbH = thumb.getHeight();

    int grabZone = std::clamp(thumbH / 4, 3, 7);

    if (std::abs(mouseY - thumbTop) <= grabZone)
        return ResizeEdge::Top;
    if (std::abs(mouseY - thumbBottom) <= grabZone)
        return ResizeEdge::Bottom;

    return ResizeEdge::None;
}

void NotebookVerticalScrollBar::mouseMove(const juce::MouseEvent& e)
{
    if (getResizeEdgeAt(e.y) != ResizeEdge::None)
    {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }
    else
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }

    juce::ScrollBar::mouseMove(e);
}

void NotebookVerticalScrollBar::mouseExit(const juce::MouseEvent& e)
{
    if (!isResizing)
        setMouseCursor(juce::MouseCursor::NormalCursor);

    juce::ScrollBar::mouseExit(e);
}

void NotebookVerticalScrollBar::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown())
    {
        auto edge = getResizeEdgeAt(e.y);
        if (edge != ResizeEdge::None)
        {
            activeEdge = edge;
            isResizing = true;
            dragStartY = (float)e.y;

            if (onResizeStarted)
                onResizeStarted();

            // CRITICAL: Return immediately without calling juce::ScrollBar::mouseDown(e).
            // This completely freezes JUCE's internal scrollbar scrolling mechanism!
            return;
        }
    }

    activeEdge = ResizeEdge::None;
    isResizing = false;
    juce::ScrollBar::mouseDown(e);
}

void NotebookVerticalScrollBar::mouseDrag(const juce::MouseEvent& e)
{
    if (isResizing)
    {
        float deltaY = (float)e.y - dragStartY;
        if (onResizeDragged)
            onResizeDragged(activeEdge, deltaY);

        // CRITICAL: Return immediately without calling juce::ScrollBar::mouseDrag(e).
        // This keeps the scrollbar frozen vertically while resizing row heights!
        return;
    }

    juce::ScrollBar::mouseDrag(e);
}

void NotebookVerticalScrollBar::mouseUp(const juce::MouseEvent& e)
{
    if (isResizing)
    {
        isResizing = false;
        activeEdge = ResizeEdge::None;
        setMouseCursor(juce::MouseCursor::NormalCursor);

        if (onResizeEnded)
            onResizeEnded();

        return;
    }

    juce::ScrollBar::mouseUp(e);
}

void NotebookVerticalScrollBar::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCtrlDown() || e.mods.isAltDown() || e.mods.isCommandDown())
    {
        if (onZoomWheel)
            onZoomWheel(wheel.deltaY > 0 ? 4 : -4);
        return;
    }

    juce::ScrollBar::mouseWheelMove(e, wheel);
}

void NotebookVerticalScrollBar::paint(juce::Graphics& g)
{
    juce::ScrollBar::paint(g);

    // Subtle DAW-style resize grip indicators on top & bottom thumb edges
    auto thumb = getThumbBounds();
    if (!thumb.isEmpty() && thumb.getHeight() >= 24)
    {
        g.setColour(juce::Colour(0x50000000));
        int cx = thumb.getCentreX();
        // Top grip bar
        g.fillRect(cx - 3, thumb.getY() + 3, 6, 1);
        // Bottom grip bar
        g.fillRect(cx - 3, thumb.getBottom() - 4, 6, 1);
    }
}

// -----------------------------------------------------------------------------
// NotebookViewport
// -----------------------------------------------------------------------------

NotebookViewport::NotebookViewport()
{
    recreateScrollbars();
}

NotebookViewport::~NotebookViewport()
{
}

juce::ScrollBar* NotebookViewport::createScrollBarComponent(bool isVertical)
{
    if (isVertical)
        return new NotebookVerticalScrollBar();

    return juce::Viewport::createScrollBarComponent(false);
}

void NotebookViewport::scrollBarMoved(juce::ScrollBar* sb, double newRangeStart)
{
    // If scroll is frozen during resizing, ignore all scroll movement requests
    if (scrollFrozen)
        return;

    juce::Viewport::scrollBarMoved(sb, newRangeStart);
}

void NotebookViewport::visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea)
{
    juce::Viewport::visibleAreaChanged(newVisibleArea);
    if (onVisibleAreaChanged)
        onVisibleAreaChanged();
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
    viewport.setScrollBarThickness(14);
    viewport.onVisibleAreaChanged = [this] {
        checkInfiniteScroll();
    };

    if (auto* vsb = dynamic_cast<NotebookVerticalScrollBar*>(&viewport.getVerticalScrollBar()))
    {
        vsb->onResizeStarted = [this] { handleResizeStarted(); };
        vsb->onResizeDragged = [this] (NotebookVerticalScrollBar::ResizeEdge edge, float deltaY) {
            handleResizeDragged(edge, deltaY);
        };
        vsb->onResizeEnded   = [this] { handleResizeEnded(); };
        vsb->onZoomWheel     = [this] (int delta) { handleZoomWheel(delta); };
    }

    timelineRuler = std::make_unique<DAWTimelineRulerComponent>(*document, MARGIN_X);
    addAndMakeVisible(timelineRuler.get());
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
    timelineRuler = std::make_unique<DAWTimelineRulerComponent>(*document, MARGIN_X);
    addAndMakeVisible(timelineRuler.get());

    resized();
    repaint();
}

void NotebookPageView::handleResizeStarted()
{
    viewport.setScrollFrozen(true);
    resizeStartBarHeight = document ? document->getBarHeight() : 50;

    resizeAnchorBarTop = 0;
    resizeAnchorOffsetTop = 0;
    resizeAnchorBarBottom = 0;
    resizeAnchorOffsetBottom = 0;

    if (pageContent != nullptr && document != nullptr)
    {
        int curY = viewport.getViewPositionY();
        int curBottomY = curY + viewport.getHeight();
        int startB = document->getVisibleStartBar();
        int endB = document->getVisibleEndBar();

        for (int b = startB; b < endB; ++b)
        {
            int bY = pageContent->getBarY(b);
            if (bY <= curY)
            {
                resizeAnchorBarTop = b;
                resizeAnchorOffsetTop = curY - bY;
            }
            if (bY <= curBottomY)
            {
                resizeAnchorBarBottom = b;
                resizeAnchorOffsetBottom = curBottomY - bY;
            }
        }
    }
}

void NotebookPageView::handleResizeDragged(NotebookVerticalScrollBar::ResizeEdge edge, float deltaY)
{
    if (document == nullptr || pageContent == nullptr)
        return;

    float scale = 0.35f;
    int newHeight = resizeStartBarHeight;

    if (edge == NotebookVerticalScrollBar::ResizeEdge::Bottom)
    {
        // Dragging thumb bottom down expands thumb -> decreases content row height
        // Dragging thumb bottom up shrinks thumb -> increases content row height
        newHeight = resizeStartBarHeight - juce::roundToInt(deltaY * scale);
    }
    else if (edge == NotebookVerticalScrollBar::ResizeEdge::Top)
    {
        // Dragging thumb top up expands thumb -> decreases content row height
        // Dragging thumb top down shrinks thumb -> increases content row height
        newHeight = resizeStartBarHeight + juce::roundToInt(deltaY * scale);
    }

    newHeight = std::clamp(newHeight, 32, 100);

    if (document->getBarHeight() != newHeight)
    {
        document->setBarHeight(newHeight);
        pageContent->setBarHeight(newHeight);

        // Re-anchor the visible position so the content and the scrollbar remain frozen vertically!
        int newY = 0;
        int maxScroll = std::max(0, pageContent->getHeight() - viewport.getHeight());

        if (edge == NotebookVerticalScrollBar::ResizeEdge::Bottom)
        {
            // Pinned to top visible bar
            newY = pageContent->getBarY(resizeAnchorBarTop) + resizeAnchorOffsetTop;
        }
        else
        {
            // Pinned to bottom visible bar
            int newBottomY = pageContent->getBarY(resizeAnchorBarBottom) + resizeAnchorOffsetBottom;
            newY = newBottomY - viewport.getHeight();
        }

        newY = std::clamp(newY, 0, maxScroll);
        viewport.setViewPosition(0, newY);
    }
}

void NotebookPageView::handleResizeEnded()
{
    viewport.setScrollFrozen(false);
}

void NotebookPageView::handleZoomWheel(int delta)
{
    if (document == nullptr || pageContent == nullptr)
        return;

    int curH = document->getBarHeight();
    int newH = std::clamp(curH + delta, 32, 100);
    if (curH != newH)
    {
        viewport.setScrollFrozen(true);

        int curY = viewport.getViewPositionY();
        int anchorB = 0;
        int anchorOff = 0;
        int startB = document->getVisibleStartBar();
        int endB = document->getVisibleEndBar();
        for (int b = startB; b < endB; ++b)
        {
            int bY = pageContent->getBarY(b);
            if (bY <= curY)
            {
                anchorB = b;
                anchorOff = curY - bY;
            }
            else break;
        }

        document->setBarHeight(newH);
        pageContent->setBarHeight(newH);

        int newY = pageContent->getBarY(anchorB) + anchorOff;
        int maxScroll = std::max(0, pageContent->getHeight() - viewport.getHeight());
        viewport.setViewPosition(0, std::clamp(newY, 0, maxScroll));

        viewport.setScrollFrozen(false);
    }
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
    if (viewport.isScrollFrozen())
        return;

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
    int rulerH = 24;
    if (timelineRuler != nullptr)
    {
        timelineRuler->setBounds(0, 0, getWidth(), rulerH);
    }
    viewport.setBounds(0, rulerH, getWidth(), getHeight() - rulerH);
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

void NotebookPageView::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    if (e.mods.isCtrlDown() || e.mods.isAltDown() || e.mods.isCommandDown())
    {
        handleZoomWheel(wheel.deltaY > 0 ? 4 : -4);
        return;
    }
    Component::mouseWheelMove(e, wheel);
}

void NotebookPageView::lyricDocumentChanged()
{
    if (document == nullptr)
        return;

    if (pageContent != nullptr)
    {
        if (pageContent->getBarHeight() != document->getBarHeight())
            pageContent->setBarHeight(document->getBarHeight());

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
    if (timelineRuler != nullptr)
    {
        timelineRuler->repaint();
    }
    if (pageContent != nullptr)
    {
        pageContent->rebuildBars();
    }
}

void NotebookPageView::barNotationChanged(int barIndex, const MetricNotation&)
{
    if (timelineRuler != nullptr)
    {
        timelineRuler->repaint();
    }
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

// -----------------------------------------------------------------------------
// DAWTimelineRulerComponent
// -----------------------------------------------------------------------------

DAWTimelineRulerComponent::DAWTimelineRulerComponent(LyricDocument& doc, float marginLineX)
    : document(doc), marginX(marginLineX)
{
}

void DAWTimelineRulerComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Header paper background
    g.setColour(NotebookLookAndFeel::isDarkMode() ? juce::Colour(0xFF141416) : juce::Colour(0xFFF2EFE7));
    g.fillRect(bounds);

    // 2. Ruled horizontal bottom line
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    g.drawLine(0.0f, bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f, 1.5f);

    // 3. Continuous vertical Red Margin Rule at MARGIN_X
    g.setColour(NotebookLookAndFeel::getMarginRedColour());
    g.drawLine(marginX, 0.0f, marginX, bounds.getBottom(), 1.5f);

    // 4. Spiral Binder Wire Rings along the far left edge
    NotebookLookAndFeel::drawSpiralRings(g, 2.0f, bounds.getHeight(), 18.0f, 28.8f);

    // 5. Left Label "BEAT DIVISION"
    g.setColour(NotebookLookAndFeel::getLightGraphiteColour());
    g.setFont(juce::Font(juce::FontOptions("Calibri", 11.0f, juce::Font::bold)));
    g.drawFittedText("BEAT DIVISION", (int)marginX + 6, 0, 96, (int)bounds.getHeight(), juce::Justification::centredLeft, 1);

    // Vertical Divider before pulse grid
    int pulseStartX = (int)marginX + 6 + 96 + 8;
    g.setColour(NotebookLookAndFeel::getLightGraphiteColour().withAlpha(0.25f));
    g.drawLine((float)pulseStartX - 5.0f, 2.0f, (float)pulseStartX - 5.0f, bounds.getBottom() - 2.0f, 1.0f);

    // Vertical Divider after pulse grid
    int counterW = 88;
    int counterMarginRight = 24;
    int counterX = (int)bounds.getWidth() - counterW - counterMarginRight;
    int pulseEndX = counterX - 8;
    g.drawLine((float)pulseEndX + 4.0f, 2.0f, (float)pulseEndX + 4.0f, bounds.getBottom() - 2.0f, 1.0f);

    // 6. Numerical Indicators for each Beat (demarcated at top of display)
    const auto& notation = document.getNotation();
    int bpb = notation.getBeatsPerBar();
    if (bpb <= 0) bpb = 4;

    if (pulseEndX > pulseStartX)
    {
        float totalSpan = (float)(pulseEndX - pulseStartX);
        juce::Font badgeFont(juce::FontOptions("Calibri", 10.5f, juce::Font::bold));
        g.setFont(badgeFont);

        for (int b = 0; b < bpb; ++b)
        {
            float lineX = (float)pulseStartX + totalSpan * ((float)b / (float)bpb);
            juce::String text = "Beat " + juce::String(b + 1);
            float textW = (float)badgeFont.getStringWidth(text) + 10.0f;
            float badgeH = 15.0f;
            float badgeY = 2.0f;
            float badgeX = (b == 0) ? lineX : (lineX - (textW * 0.5f));

            juce::Rectangle<float> badgeRect(badgeX, badgeY, textW, badgeH);

            // Badge Background
            g.setColour(NotebookLookAndFeel::getPaperColour());
            g.fillRoundedRectangle(badgeRect, 3.0f);

            // Badge Border in Theme Accent
            g.setColour(NotebookLookAndFeel::getAccentColour());
            g.drawRoundedRectangle(badgeRect, 3.0f, 1.0f);

            // Badge Text
            g.setColour(NotebookLookAndFeel::isDarkMode() ? NotebookLookAndFeel::getAccentHoverColour() : NotebookLookAndFeel::getAccentDarkColour());
            g.drawFittedText(text, badgeRect.toNearestInt(), juce::Justification::centred, 1);

            // Downward Tick pointing to the grid line
            g.setColour(NotebookLookAndFeel::getAccentColour());
            g.drawLine(lineX, badgeY + badgeH, lineX, bounds.getBottom() - 1.0f, 1.5f);
        }
    }
}

} // namespace CompassCadence
