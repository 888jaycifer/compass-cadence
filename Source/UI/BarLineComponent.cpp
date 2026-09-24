#include "BarLineComponent.h"
#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

BarLineComponent::BarLineComponent(LyricDocument& doc, int bar, float marginLineX)
    : document(doc), barIndex(bar), marginX(marginLineX)
{
    // Per-line editable metric notation field
    metricLabel.setEditable(true, true, false);
    metricLabel.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::bold)));
    metricLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    metricLabel.setColour(juce::Label::textWhenEditingColourId, NotebookLookAndFeel::getGraphiteColour());
    metricLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    metricLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    metricLabel.setJustificationType(juce::Justification::centredLeft);
    metricLabel.setTooltip("Metric cross-rhythm for this bar (e.g. [4444]/4:4, [333222]/6:4). Click to customize this bar's flow.");
    metricLabel.addListener(this);
    addAndMakeVisible(metricLabel);

    // Syllable counter controls (actual spoken syllables vs. metric grid)
    decSylBtn.setTooltip("Decrease actual spoken syllable count (e.g. for slurs/elisions)");
    decSylBtn.onClick = [this]
    {
        int current = document.getActualSpokenSyllableCount(barIndex);
        document.setActualSpokenSyllableCount(barIndex, std::max(0, current - 1));
        updateSyllableCounter();
    };
    addAndMakeVisible(decSylBtn);

    incSylBtn.setTooltip("Increase actual spoken syllable count (e.g. for ghost/extra syllables)");
    incSylBtn.onClick = [this]
    {
        int current = document.getActualSpokenSyllableCount(barIndex);
        document.setActualSpokenSyllableCount(barIndex, current + 1);
        updateSyllableCounter();
    };
    addAndMakeVisible(incSylBtn);

    sylCountLabel.setFont(juce::Font(juce::FontOptions("Calibri", 11.0f, juce::Font::bold)));
    sylCountLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    sylCountLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    sylCountLabel.setJustificationType(juce::Justification::centred);
    sylCountLabel.addMouseListener(this, false);
    addAndMakeVisible(sylCountLabel);

    // Row Options Button "+" shown on row hover
    addLineBtn.setButtonText("+");
    addLineBtn.setTooltip("Row options: Add new line (w same meter) or toggle stanza break");
    addLineBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    addLineBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0x20000000));
    addLineBtn.setColour(juce::TextButton::textColourOffId, NotebookLookAndFeel::getLightGraphiteColour());
    addLineBtn.setColour(juce::TextButton::textColourOnId, NotebookLookAndFeel::getGraphiteColour());
    addLineBtn.onClick = [this]
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Add new line (w same meter)");
        bool hasBreak = document.shouldAddSpacingAfterBar(barIndex);
        menu.addItem(2, hasBreak ? "Remove stanza break" : "Add stanza break");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&addLineBtn), [this](int result)
        {
            if (result == 1)
            {
                document.insertBar(barIndex);
            }
            else if (result == 2)
            {
                document.toggleStanzaBreak(barIndex);
            }
        });
    };
    addChildComponent(addLineBtn);
    addLineBtn.setVisible(false);

    setPaintingIsUnclipped(true);

    rebuildPulses();
}

BarLineComponent::~BarLineComponent()
{
}

void BarLineComponent::updateMetricLabel()
{
    const auto& notat = document.getNotation(barIndex);
    juce::String str = notat.toNotationString();
    if (metricLabel.getText() != str)
        metricLabel.setText(str, juce::dontSendNotification);
}

void BarLineComponent::updateSyllableCounter()
{
    int actual = document.getActualSpokenSyllableCount(barIndex);
    int gridTotal = document.getNotation(barIndex).getTotalSyllables();
    bool isCustom = document.hasCustomSpokenSyllableCount(barIndex);

    juce::String text = juce::String(actual) + "/" + juce::String(gridTotal);
    if (isCustom)
    {
        // High-contrast deep theme accent indicates customized count
        sylCountLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getAccentDarkColour());
        sylCountLabel.setTooltip("Custom spoken syllable count: " + juce::String(actual) + " (Grid: " + juce::String(gridTotal) + "). Double-click or right-click to reset.");
    }
    else
    {
        sylCountLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getLightGraphiteColour());
        sylCountLabel.setTooltip("Syllable count: " + juce::String(actual) + " spoken / " + juce::String(gridTotal) + " grid subdivisions. Use +/- to adjust.");
    }

    if (sylCountLabel.getText() != text)
        sylCountLabel.setText(text, juce::dontSendNotification);
}

void BarLineComponent::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (e.eventComponent == &sylCountLabel)
    {
        document.resetActualSpokenSyllableCount(barIndex);
        updateSyllableCounter();
    }
}

void BarLineComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.eventComponent == &sylCountLabel && e.mods.isPopupMenu())
    {
        int calc = document.getCalculatedSyllableCount(barIndex);
        juce::PopupMenu menu;
        menu.addItem(1, "Reset to Calculated Count (" + juce::String(calc) + ")", document.hasCustomSpokenSyllableCount(barIndex));
        menu.showMenuAsync(juce::PopupMenu::Options(), [this](int result)
        {
            if (result == 1)
            {
                document.resetActualSpokenSyllableCount(barIndex);
                updateSyllableCounter();
            }
        });
    }
}

void BarLineComponent::rebuildPulses()
{
    pulseGroups.clear();

    const auto& notation = document.getNotation(barIndex);
    const int numPulses = notation.getPulseCount();
    int runningGlobal = 0;

    for (int p = 0; p < numPulses; ++p)
    {
        int sylCount = notation.getSyllablesForPulse(p);
        auto pulseBox = std::make_unique<PulseGroupComponent>(document, barIndex, p, sylCount, runningGlobal);
        pulseBox->setPulseNavListener(this);
        addAndMakeVisible(pulseBox.get());
        pulseGroups.push_back(std::move(pulseBox));

        runningGlobal += sylCount;
    }

    updateMetricLabel();
    updateSyllableCounter();
    addLineBtn.toFront(false);
    if (!isMouseOver(true))
        addLineBtn.setVisible(false);
    resized();
    repaint();
}

void BarLineComponent::updateContent()
{
    updateMetricLabel();
    updateSyllableCounter();
    if (!isMouseOver(true))
        addLineBtn.setVisible(false);
    for (auto& p : pulseGroups)
        p->updateContent();
}

void BarLineComponent::updateSelection()
{
    for (auto& p : pulseGroups)
        p->updateContent();
}

void BarLineComponent::setPlayheadActive(bool isBarActive, double progress, bool isPlaying)
{
    bool changed = (isBarActiveInDAW != isBarActive || isDAWPlaying != isPlaying || std::abs(barProgress - progress) > 0.001);
    isBarActiveInDAW = isBarActive;
    barProgress = progress;
    isDAWPlaying = isPlaying;

    if (changed)
        repaint();
}

void BarLineComponent::focusSyllable(int globalSyllableIndex)
{
    const auto& notation = document.getNotation(barIndex);
    auto [pulseIdx, sylInPulse] = notation.getPulseAndSyllableFromGlobal(globalSyllableIndex);

    if (pulseIdx >= 0 && pulseIdx < (int)pulseGroups.size())
    {
        if (auto* cell = pulseGroups[pulseIdx]->getCell(sylInPulse))
        {
            cell->startEditing();
        }
    }
}

void BarLineComponent::mouseEnter(const juce::MouseEvent&)
{
    addLineBtn.setVisible(true);
}

void BarLineComponent::mouseMove(const juce::MouseEvent&)
{
    if (!addLineBtn.isVisible())
    {
        addLineBtn.setVisible(true);
    }
}

void BarLineComponent::mouseExit(const juce::MouseEvent& e)
{
    if (!getLocalBounds().contains(e.getPosition()))
    {
        addLineBtn.setVisible(false);
        SyllableCellComponent::clearGlobalHoveredCell();
    }
}

void BarLineComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Notebook ruling line along bottom of this bar
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    g.drawLine(0.0f, bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f, 1.0f);

    // 2. Bar Number in left margin
    juce::String barNumStr = juce::String::formatted("%02d", barIndex + 1);
    juce::Rectangle<float> gutterBounds(10.0f, 0.0f, marginX - 18.0f, bounds.getHeight());

    if (isBarActiveInDAW)
    {
        // High-contrast deep theme accent marker when active
        g.setColour(NotebookLookAndFeel::getAccentDarkColour());
        g.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::bold)));
    }
    else
    {
        g.setColour(NotebookLookAndFeel::getLightGraphiteColour());
        g.setFont(juce::Font(juce::FontOptions("Calibri", 11.0f, juce::Font::plain)));
    }

    g.drawFittedText(barNumStr, gutterBounds.toNearestInt(), juce::Justification::centredRight, 1);

    // 3. Stationary Left-Margin Playhead: Crisp right-pointing arrow ▶ in gutter
    if (isBarActiveInDAW && isDAWPlaying)
    {
        float tipX = marginX - 5.0f;
        float baseX = marginX - 13.0f;
        float centerY = bounds.getCentreY();
        float halfH = 5.0f;

        juce::Path arrow;
        arrow.startNewSubPath(baseX, centerY - halfH);
        arrow.lineTo(tipX, centerY);
        arrow.lineTo(baseX, centerY + halfH);
        arrow.closeSubPath();

        g.setColour(NotebookLookAndFeel::getAccentColour());
        g.fillPath(arrow);

        // Accent rule line along marginX for this active line
        g.drawLine(marginX, 0.0f, marginX, bounds.getHeight(), 2.0f);
    }

    // 4. Crisp vertical divider between the per-line metric label and the pulse cells
    int pulseStartX = (int)marginX + 6 + 96 + 8;
    g.setColour(NotebookLookAndFeel::getLightGraphiteColour().withAlpha(0.22f));
    g.drawLine((float)pulseStartX - 5.0f, 4.0f, (float)pulseStartX - 5.0f, bounds.getBottom() - 4.0f, 1.0f);

    // 5. Crisp vertical divider between pulse cells and the syllable counter
    int counterW = 88;
    int counterMarginRight = 24;
    int counterX = (int)bounds.getWidth() - counterW - counterMarginRight;
    int pulseEndX = counterX - 8;
    g.drawLine((float)pulseEndX + 4.0f, 4.0f, (float)pulseEndX + 4.0f, bounds.getBottom() - 4.0f, 1.0f);
}

void BarLineComponent::paintOverChildren(juce::Graphics& g)
{
    // Static DAW structural beat grid lines drawn over children so they are always visible across syllable cell bars
    const auto& notation = document.getNotation(barIndex);
    const int bpb = notation.getBeatsPerBar();
    auto bounds = getLocalBounds();
    int metricX = (int)marginX + 6;
    int metricW = 96;
    int pulseStartX = metricX + metricW + 8;
    int counterW = 88;
    int counterMarginRight = 24;
    int counterX = (int)bounds.getWidth() - counterW - counterMarginRight;
    int pulseEndX = counterX - 8;
    int availableWidth = pulseEndX - pulseStartX;

    if (bpb > 1 && availableWidth > 0)
    {
        juce::Colour dawBeatCol = document.isDarkMode()
            ? juce::Colour(0x55FFFFFF)
            : juce::Colour(0x5552525B);
        juce::Colour dawBeatAccent = document.isDarkMode()
            ? NotebookLookAndFeel::getAccentHoverColour().withAlpha(0.70f)
            : NotebookLookAndFeel::getAccentColour().withAlpha(0.60f);

        const float beatDash[] = { 4.0f, 3.0f };

        for (int b = 0; b < bpb; ++b)
        {
            float lineX = MetricNotation::getBeatScreenX(notation, (double)b, pulseStartX, availableWidth);
            g.setColour(dawBeatCol);
            g.drawLine(lineX, 0.0f, lineX, (float)bounds.getBottom(), 1.0f);

            g.setColour(dawBeatAccent);
            g.drawDashedLine(juce::Line<float>(lineX, 0.0f, lineX, (float)bounds.getBottom()), beatDash, 2, 1.0f);
        }
    }
}

void BarLineComponent::resized()
{
    auto bounds = getLocalBounds();
    const int numPulses = (int)pulseGroups.size();

    int metricX = (int)marginX + 6;
    int metricW = 96;
    int h = bounds.getHeight() - 4;
    int y = 2;

    int boxH = std::max(18, h - 14);
    metricLabel.setBounds(metricX, y + 1, metricW, boxH - 2);

    // Syllable counter controls: [-] [16/16] [+]
    int counterW = 88;
    int counterMarginRight = 24;
    int counterX = bounds.getWidth() - counterW - counterMarginRight;
    int btnW = 18;
    int labelW = counterW - (btnW * 2) - 4;
    int btnH = 20;
    int btnY = y + (boxH - btnH) / 2;

    decSylBtn.setBounds(counterX, btnY, btnW, btnH);
    sylCountLabel.setBounds(counterX + btnW + 2, btnY, labelW, btnH);
    incSylBtn.setBounds(counterX + btnW + 2 + labelW + 2, btnY, btnW, btnH);

    // Row Options "+" button tucked into the bottom right corner of the line (not in line with syllable count)
    int addBtnW = 16;
    int addBtnH = 15;
    int addBtnX = bounds.getWidth() - addBtnW - 3;
    int addBtnY = bounds.getHeight() - addBtnH - 2;
    addLineBtn.setBounds(addBtnX, addBtnY, addBtnW, addBtnH);

    if (numPulses == 0)
        return;

    // Pulse boxes start to the right of the metric label column and end before the counter column
    int pulseStartX = metricX + metricW + 8;
    int pulseEndX = counterX - 8;
    int availableWidth = pulseEndX - pulseStartX;
    if (availableWidth <= 0)
        return;

    int gap = 10; // Spacing gap between pulse group boxes (Requirement 4: 10px gap)
    int totalGaps = (numPulses - 1) * gap;
    int netWidth = availableWidth - totalGaps;
    if (netWidth <= 0) netWidth = availableWidth;

    const auto& notation = document.getNotation(barIndex);
    const double totalDuration = notation.getTotalDuration();

    int curX = pulseStartX;
    for (int p = 0; p < numPulses; ++p)
    {
        double pulseWeight = notation.getPulseDuration(p);
        double ratio = (totalDuration > 0.0) ? (pulseWeight / totalDuration) : (1.0 / (double)numPulses);
        int w = (p == numPulses - 1) ? (pulseStartX + availableWidth - curX) : (int)std::round(netWidth * ratio);
        pulseGroups[p]->setBounds(curX, y, w, h);
        curX += w + gap;
    }
}

void BarLineComponent::labelTextChanged(juce::Label* labelThatHasChanged)
{
    if (labelThatHasChanged == &metricLabel)
    {
        juce::String text = metricLabel.getText().trim();
        if (text.isNotEmpty())
        {
            int currentBeats = document.getNotation(barIndex).getBeatsPerBar();
            MetricNotation parsed = MetricNotation::fromNotationString(text, currentBeats);
            document.setBarNotation(barIndex, parsed);
        }
        else
        {
            document.clearBarNotation(barIndex);
        }
        updateMetricLabel();
        rebuildPulses();
    }
}

void BarLineComponent::editorShown(juce::Label* label, juce::TextEditor& editor)
{
    if (label == &metricLabel)
    {
        // High-contrast contrast in both Dark and Light modes (no light-gray on white!)
        bool dark = NotebookLookAndFeel::isDarkMode();
        editor.setColour(juce::TextEditor::textColourId, dark ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF262626));
        editor.setColour(juce::TextEditor::backgroundColourId, dark ? juce::Colour(0xFF27272A) : juce::Colour(0xFFFFFFFF));
        editor.setColour(juce::TextEditor::outlineColourId, NotebookLookAndFeel::getPulseBoxBorderColour());
        editor.setColour(juce::TextEditor::focusedOutlineColourId, dark ? NotebookLookAndFeel::getAccentHoverColour() : NotebookLookAndFeel::getAccentColour());
        editor.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::bold)));
        editor.applyColourToAllText(dark ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF262626), true);
    }
}

void BarLineComponent::editorHidden(juce::Label*, juce::TextEditor&)
{
}

void BarLineComponent::onSyllableAdvance(int barIdx, int globalSylIdx, bool forward)
{
    if (navListener != nullptr)
        navListener->onBarNavigateSyllable(barIdx, globalSylIdx, forward);
}

void BarLineComponent::onSyllableJumpTo(int targetBarIdx, int targetGlobalSylIdx)
{
    if (navListener != nullptr)
        navListener->onBarJumpToCell(targetBarIdx, targetGlobalSylIdx);
}

void BarLineComponent::onSyllableEnterNextBar(int barIdx)
{
    if (navListener != nullptr)
        navListener->onBarEnterNext(barIdx);
}

bool BarLineComponent::hitTest(int x, int y)
{
    if (juce::Component::hitTest(x, y))
        return true;
    for (auto& p : pulseGroups)
    {
        if (p != nullptr && p->isVisible())
        {
            if (p->hitTest(x - p->getX(), y - p->getY()))
                return true;
        }
    }
    return false;
}

} // namespace CompassCadence
