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
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

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
}

void PulseGroupComponent::resized()
{
    auto bounds = getLocalBounds().reduced(2);
    const int numCells = (int)cells.size();
    if (numCells == 0)
        return;

    int cellWidth = bounds.getWidth() / numCells;
    int x = bounds.getX();

    for (int i = 0; i < numCells; ++i)
    {
        int w = (i == numCells - 1) ? (bounds.getRight() - x) : cellWidth;
        cells[i]->setBounds(x, bounds.getY(), w, bounds.getHeight());
        x += w;
    }
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
