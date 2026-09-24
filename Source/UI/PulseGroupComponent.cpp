#include "PulseGroupComponent.h"
#include "NotebookLookAndFeel.h"

namespace CompassCadence
{

PulseGroupComponent::PulseGroupComponent(LyricDocument& doc, int bar, int pulse, int count, int startGlobal)
    : document(doc), barIndex(bar), pulseIndex(pulse), startGlobalIndex(startGlobal)
{
    count = std::max(1, count);

    for (int s = 0; s < count; ++s)
    {
        auto cell = std::make_unique<SyllableCellComponent>(document, barIndex, pulseIndex, s, startGlobalIndex + s);
        cell->setNavigationListener(this);
        addAndMakeVisible(cell.get());
        cells.push_back(std::move(cell));
    }

    setPaintingIsUnclipped(true);
}

PulseGroupComponent::~PulseGroupComponent()
{
}

void PulseGroupComponent::updateContent()
{
    repaint();
    for (auto& c : cells)
        c->updateContent();
}

void PulseGroupComponent::setPlayheadActive(int activeSyllableInPulse)
{
    for (int s = 0; s < (int)cells.size(); ++s)
    {
        cells[s]->setPlayheadActive(s == activeSyllableInPulse);
    }
}

SyllableCellComponent* PulseGroupComponent::getCell(int index)
{
    if (index >= 0 && index < (int)cells.size())
        return cells[index].get();
    return nullptr;
}

void PulseGroupComponent::paint(juce::Graphics& g)
{
    float boxY = 8.0f;
    float boxH = std::max(18.0f, (float)getHeight() - boxY - 14.0f);
    auto bounds = juce::Rectangle<float>(1.0f, boxY, (float)getWidth() - 2.0f, boxH);

    // Thick graphite border defining the pulse subdivision group (box interior is transparent so alternating line shading flows through)
    g.setColour(NotebookLookAndFeel::getPulseBoxBorderColour());
    g.drawRoundedRectangle(bounds, 3.0f, 2.2f); // Extra width border (2.2px)

    // Inner thin dividers between individual syllable cells
    const int numCells = (int)cells.size();
    if (numCells > 1)
    {
        g.setColour(NotebookLookAndFeel::getLightGraphiteColour().withAlpha(0.45f));
        float cellWidth = bounds.getWidth() / (float)numCells;

        const float dashLengths[] = { 3.0f, 2.0f };
        for (int i = 1; i < numCells; ++i)
        {
            float x = bounds.getX() + (float)i * cellWidth;
            g.drawDashedLine(juce::Line<float>(x, bounds.getY() + 3.0f, x, bounds.getBottom() - 3.0f),
                             dashLengths, 2, 1.0f);
        }
    }

    // Tuplet Brackets (State Machine: ALL_ON, ALL_OFF, ACTIVE_LINE)
    auto mode = document.getTupletBracketMode();
    if (mode == LyricDocument::BracketAllOff)
        return;

    if (mode == LyricDocument::BracketActiveLine)
    {
        bool isCurrentBarActive = false;
        for (const auto& cell : document.getSelectedCells())
        {
            if (cell.first == barIndex)
            {
                isCurrentBarActive = true;
                break;
            }
        }
        if (!isCurrentBarActive)
            return;
    }

    if (getHeight() < 24)
        return;

    const juce::Colour tupletCol = NotebookLookAndFeel::getAccentColour();
    g.setColour(tupletCol);

    juce::String numStr = juce::String(numCells);

    // Tuplet Bracket Renders ON TOP of the Pulse Group Box
    float bracketY = 3.0f;
    float tickLen = 3.5f;
    float xLeft = bounds.getX() + 2.0f;
    float xRight = bounds.getRight() - 2.0f;
    float midX = (xLeft + xRight) * 0.5f;

    bool showNumeral = (getHeight() >= 34);

    if (showNumeral)
    {
        juce::Font numFont(juce::FontOptions("Calibri", 10.0f, juce::Font::bold));
        g.setFont(numFont);
        float numW = (float)numFont.getStringWidth(numStr) + 6.0f;
        float halfW = numW * 0.5f;

        // Left bracket segment & downward tick pointing towards the box
        g.drawLine(xLeft, bracketY, xLeft, bracketY + tickLen, 1.5f);
        if (midX - halfW > xLeft)
            g.drawLine(xLeft, bracketY, midX - halfW, bracketY, 1.5f);

        // Right bracket segment & downward tick pointing towards the box
        g.drawLine(xRight, bracketY, xRight, bracketY + tickLen, 1.5f);
        if (xRight > midX + halfW)
            g.drawLine(midX + halfW, bracketY, xRight, bracketY, 1.5f);

        // Centered numeral
        juce::Rectangle<float> numRect(midX - halfW, bracketY - 2.0f, numW, 10.0f);
        g.setColour(NotebookLookAndFeel::getPaperColour());
        g.fillRect(numRect);
        g.setColour(tupletCol);
        g.drawFittedText(numStr, numRect.toNearestInt(), juce::Justification::centred, 1);
    }
    else
    {
        // Compact: continuous bracket with left/right downward ticks, no numeral
        g.drawLine(xLeft, bracketY, xLeft, bracketY + tickLen, 1.5f);
        g.drawLine(xRight, bracketY, xRight, bracketY + tickLen, 1.5f);
        g.drawLine(xLeft, bracketY, xRight, bracketY, 1.5f);
    }
}

void PulseGroupComponent::resized()
{
    const int numCells = (int)cells.size();
    if (numCells == 0)
        return;

    int totalW = getWidth();
    int boxY = 8;
    int cellH = getHeight() - boxY;
    int cellWidth = totalW / numCells;
    int x = 0;

    for (int i = 0; i < numCells; ++i)
    {
        int w = (i == numCells - 1) ? (totalW - x) : cellWidth;
        cells[i]->setBounds(x, boxY, w, cellH);
        x += w;
    }
}

bool PulseGroupComponent::hitTest(int x, int y)
{
    if (juce::Component::hitTest(x, y))
        return true;
    for (auto& c : cells)
    {
        if (c != nullptr && c->isVisible())
        {
            if (c->hitTest(x - c->getX(), y - c->getY()))
                return true;
        }
    }
    return false;
}

void PulseGroupComponent::onCellAdvance(int barIdx, int globalSylIdx, bool forward)
{
    if (pulseNavListener != nullptr)
        pulseNavListener->onSyllableAdvance(barIdx, globalSylIdx, forward);
}

void PulseGroupComponent::onCellJumpTo(int targetBarIdx, int targetGlobalSylIdx)
{
    if (pulseNavListener != nullptr)
        pulseNavListener->onSyllableJumpTo(targetBarIdx, targetGlobalSylIdx);
}

void PulseGroupComponent::onCellEnterNextBar(int barIdx)
{
    if (pulseNavListener != nullptr)
        pulseNavListener->onSyllableEnterNextBar(barIdx);
}

} // namespace CompassCadence
