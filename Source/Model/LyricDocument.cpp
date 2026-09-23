#include "LyricDocument.h"
#include "../UI/NotebookLookAndFeel.h"

namespace CompassCadence
{

LyricDocument::LyricDocument()
    : defaultNotation(4, { 4, 4, 4, 4 })
{
}

const MetricNotation& LyricDocument::getNotation(int barIndex) const noexcept
{
    if (barIndex >= 0)
    {
        auto it = barNotations.find(barIndex);
        if (it != barNotations.end())
            return it->second;
    }
    return defaultNotation;
}

void LyricDocument::setNotation(const MetricNotation& newNotation)
{
    if (defaultNotation != newNotation)
    {
        pushUndoSnapshot();
        defaultNotation = newNotation;
        int newTotal = defaultNotation.getTotalSyllables();

        for (auto& pair : barData)
        {
            int b = pair.first;
            if (!hasBarNotationOverride(b))
            {
                if ((int)pair.second.size() > newTotal)
                    pair.second.resize(newTotal);

                for (auto bit = boldCells.begin(); bit != boldCells.end(); )
                {
                    if (bit->first == b && bit->second >= newTotal)
                        bit = boldCells.erase(bit);
                    else
                        ++bit;
                }
                for (auto ait = cellAlignments.begin(); ait != cellAlignments.end(); )
                {
                    if (ait->first.first == b && ait->first.second >= newTotal)
                        ait = cellAlignments.erase(ait);
                    else
                        ++ait;
                }
                for (auto cit = customCellColors.begin(); cit != customCellColors.end(); )
                {
                    if (cit->first.first == b && cit->first.second >= newTotal)
                        cit = customCellColors.erase(cit);
                    else
                        ++cit;
                }
                auto spIt = customSpokenSyllableCounts.find(b);
                if (spIt != customSpokenSyllableCounts.end() && spIt->second > newTotal)
                {
                    spIt->second = newTotal;
                }
            }
        }

        refreshRhymes();
        notifyNotationChanged();
        notifyChanged();
    }
}

void LyricDocument::setBarNotation(int barIndex, const MetricNotation& newNotation)
{
    if (barIndex < 0)
        return;

    auto it = barNotations.find(barIndex);
    if (it != barNotations.end() && it->second == newNotation)
        return;

    pushUndoSnapshot();
    barNotations[barIndex] = newNotation;
    ensureBarCount(barIndex + 1);

    // Truncate any syllables that extend past the new division count
    int newTotal = newNotation.getTotalSyllables();
    auto dataIt = barData.find(barIndex);
    if (dataIt != barData.end() && (int)dataIt->second.size() > newTotal)
    {
        dataIt->second.resize(newTotal);
    }
    for (auto bit = boldCells.begin(); bit != boldCells.end(); )
    {
        if (bit->first == barIndex && bit->second >= newTotal)
            bit = boldCells.erase(bit);
        else
            ++bit;
    }
    for (auto ait = cellAlignments.begin(); ait != cellAlignments.end(); )
    {
        if (ait->first.first == barIndex && ait->first.second >= newTotal)
            ait = cellAlignments.erase(ait);
        else
            ++ait;
    }
    for (auto cit = customCellColors.begin(); cit != customCellColors.end(); )
    {
        if (cit->first.first == barIndex && cit->first.second >= newTotal)
            cit = customCellColors.erase(cit);
        else
            ++cit;
    }
    auto spIt = customSpokenSyllableCounts.find(barIndex);
    if (spIt != customSpokenSyllableCounts.end() && spIt->second > newTotal)
    {
        spIt->second = newTotal;
    }

    refreshRhymes();
    notifyBarNotationChanged(barIndex);
    notifyChanged();
}

void LyricDocument::clearBarNotation(int barIndex)
{
    auto it = barNotations.find(barIndex);
    if (it != barNotations.end())
    {
        pushUndoSnapshot();
        barNotations.erase(it);

        int newTotal = defaultNotation.getTotalSyllables();
        auto dataIt = barData.find(barIndex);
        if (dataIt != barData.end() && (int)dataIt->second.size() > newTotal)
        {
            dataIt->second.resize(newTotal);
        }
        for (auto bit = boldCells.begin(); bit != boldCells.end(); )
        {
            if (bit->first == barIndex && bit->second >= newTotal)
                bit = boldCells.erase(bit);
            else
                ++bit;
        }
        for (auto ait = cellAlignments.begin(); ait != cellAlignments.end(); )
        {
            if (ait->first.first == barIndex && ait->first.second >= newTotal)
                ait = cellAlignments.erase(ait);
            else
                ++ait;
        }
        for (auto cit = customCellColors.begin(); cit != customCellColors.end(); )
        {
            if (cit->first.first == barIndex && cit->first.second >= newTotal)
                cit = customCellColors.erase(cit);
            else
                ++cit;
        }
        auto spIt = customSpokenSyllableCounts.find(barIndex);
        if (spIt != customSpokenSyllableCounts.end() && spIt->second > newTotal)
        {
            spIt->second = newTotal;
        }

        refreshRhymes();
        notifyBarNotationChanged(barIndex);
        notifyChanged();
    }
}

bool LyricDocument::hasBarNotationOverride(int barIndex) const noexcept
{
    return barNotations.find(barIndex) != barNotations.end();
}

void LyricDocument::notifyBarNotationChanged(int barIndex)
{
    listeners.call([this, barIndex](Listener& l) {
        l.barNotationChanged(barIndex, getNotation(barIndex));
    });
}

void LyricDocument::ensureBarCount(int count)
{
    if (count > totalBars)
    {
        int pagesNeeded = (count + barsPerPage - 1) / barsPerPage;
        totalBars = std::max(pagesNeeded * barsPerPage, count);
        notifyChanged();
    }
}

juce::String LyricDocument::getSyllable(int barIndex, int globalSyllableIndex) const
{
    auto it = barData.find(barIndex);
    if (it != barData.end() && globalSyllableIndex >= 0 && globalSyllableIndex < (int)it->second.size())
    {
        return it->second[globalSyllableIndex];
    }
    return {};
}

void LyricDocument::setSyllable(int barIndex, int globalSyllableIndex, const juce::String& text, bool notify)
{
    if (barIndex < 0 || globalSyllableIndex < 0)
        return;

    ensureBarCount(barIndex + 1);

    auto& vec = barData[barIndex];
    int totalNeeded = std::max(getNotation(barIndex).getTotalSyllables(), globalSyllableIndex + 1);
    if ((int)vec.size() < totalNeeded)
        vec.resize(totalNeeded);

    if (vec[globalSyllableIndex] == text)
        return;

    if (notify)
        pushUndoSnapshot();

    vec[globalSyllableIndex] = text;

    if (notify)
    {
        refreshRhymes();
        notifyChanged();
    }
}

std::vector<juce::String> LyricDocument::getBarSyllables(int barIndex) const
{
    auto it = barData.find(barIndex);
    int total = getNotation(barIndex).getTotalSyllables();
    if (it != barData.end())
    {
        std::vector<juce::String> res = it->second;
        if ((int)res.size() < total)
            res.resize(total);
        return res;
    }
    return std::vector<juce::String>(total);
}

void LyricDocument::setBarSyllables(int barIndex, const std::vector<juce::String>& syllables)
{
    pushUndoSnapshot();
    ensureBarCount(barIndex + 1);
    barData[barIndex] = syllables;
    refreshRhymes();
    notifyChanged();
}

std::pair<int, int> LyricDocument::insertTextFlow(int barIndex, int startSyllableIndex, const juce::String& text)
{
    if (text.isEmpty())
        return { barIndex, startSyllableIndex };

    auto split = SyllableSplitter::splitLineIntoSyllables(text);
    if (split.empty())
        return { barIndex, startSyllableIndex };

    pushUndoSnapshot();

    int curBar = barIndex;
    int curSyl = startSyllableIndex;

    std::vector<std::pair<int, int>> placedTokens;

    for (const auto& token : split)
    {
        int totalSylsPerBar = getNotation(curBar).getTotalSyllables();
        if (curSyl >= totalSylsPerBar)
        {
            curBar++;
            curSyl = 0;
            totalSylsPerBar = getNotation(curBar).getTotalSyllables();
        }

        ensureBarCount(curBar + 1);
        auto& vec = barData[curBar];
        int totalNeeded = std::max(totalSylsPerBar, curSyl + 1);
        if ((int)vec.size() < totalNeeded)
            vec.resize(totalNeeded);
        vec[curSyl] = token;

        placedTokens.push_back({ curBar, curSyl });
        curSyl++;
    }

    // Auto-align multi-syllable split words together across box gaps
    size_t tokIdx = 0;
    while (tokIdx < split.size())
    {
        size_t start = tokIdx;
        while (tokIdx < split.size() && split[tokIdx].endsWith("-"))
        {
            tokIdx++;
        }
        size_t end = (tokIdx < split.size()) ? tokIdx : (split.size() - 1);
        size_t wordLen = end - start + 1;

        if (wordLen >= 2)
        {
            // First syllable aligns right (hugs next box)
            cellAlignments[placedTokens[start]] = AlignRight;
            // Last syllable aligns left (hugs previous box)
            cellAlignments[placedTokens[end]] = AlignLeft;
            // Intermediate syllables align right
            for (size_t m = start + 1; m < end; ++m)
            {
                cellAlignments[placedTokens[m]] = AlignRight;
            }
        }
        else
        {
            cellAlignments[placedTokens[start]] = AlignCenter;
        }

        tokIdx = end + 1;
    }

    refreshRhymes();
    notifyChanged();

    return { curBar, curSyl };
}

void LyricDocument::clearBar(int barIndex)
{
    if (barData.find(barIndex) != barData.end() || !boldCells.empty() || !cellAlignments.empty() || !customCellColors.empty())
    {
        pushUndoSnapshot();
        barData.erase(barIndex);
        for (auto it = boldCells.begin(); it != boldCells.end(); )
        {
            if (it->first == barIndex)
                it = boldCells.erase(it);
            else
                ++it;
        }
        for (auto it = cellAlignments.begin(); it != cellAlignments.end(); )
        {
            if (it->first.first == barIndex)
                it = cellAlignments.erase(it);
            else
                ++it;
        }
        for (auto it = customCellColors.begin(); it != customCellColors.end(); )
        {
            if (it->first.first == barIndex)
                it = customCellColors.erase(it);
            else
                ++it;
        }
        refreshRhymes();
        notifyChanged();
    }
}

void LyricDocument::clearAll()
{
    if (!barData.empty() || !boldCells.empty() || !cellAlignments.empty() || !customCellColors.empty())
    {
        pushUndoSnapshot();
        barData.clear();
        boldCells.clear();
        cellAlignments.clear();
        customCellColors.clear();
        refreshRhymes();
        notifyChanged();
    }
}

void LyricDocument::pushUndoSnapshot()
{
    DocumentSnapshot snap;
    snap.barData = barData;
    snap.defaultNotation = defaultNotation;
    snap.barNotations = barNotations;
    snap.boldCells = boldCells;
    snap.cellAlignments = cellAlignments;
    snap.showAlignmentControls = showAlignmentControls;
    snap.customCellColors = customCellColors;
    snap.hiddenColorCells = hiddenColorCells;
    snap.minRepeatLength = rhymeClassifier.getMinRepeatLength();
    snap.customSpokenSyllableCounts = customSpokenSyllableCounts;
    snap.totalBars = totalBars;
    snap.barHeight = barHeight;
    snap.viewMode = viewMode;
    snap.darkMode = darkMode;
    snap.customStanzaBreaksActive = customStanzaBreaksActive;
    snap.stanzaBreaks = stanzaBreaks;

    undoStack.push_back(std::move(snap));
    if (undoStack.size() > 100)
        undoStack.erase(undoStack.begin());

    redoStack.clear();
}

void LyricDocument::undo()
{
    if (undoStack.empty())
        return;

    DocumentSnapshot current;
    current.barData = barData;
    current.defaultNotation = defaultNotation;
    current.barNotations = barNotations;
    current.boldCells = boldCells;
    current.cellAlignments = cellAlignments;
    current.showAlignmentControls = showAlignmentControls;
    current.customCellColors = customCellColors;
    current.hiddenColorCells = hiddenColorCells;
    current.minRepeatLength = rhymeClassifier.getMinRepeatLength();
    current.customSpokenSyllableCounts = customSpokenSyllableCounts;
    current.totalBars = totalBars;
    current.barHeight = barHeight;
    current.viewMode = viewMode;
    current.darkMode = darkMode;
    current.customStanzaBreaksActive = customStanzaBreaksActive;
    current.stanzaBreaks = stanzaBreaks;
    redoStack.push_back(std::move(current));

    DocumentSnapshot prev = std::move(undoStack.back());
    undoStack.pop_back();

    barData = std::move(prev.barData);
    boldCells = std::move(prev.boldCells);
    cellAlignments = std::move(prev.cellAlignments);
    showAlignmentControls = prev.showAlignmentControls;
    customCellColors = std::move(prev.customCellColors);
    hiddenColorCells = std::move(prev.hiddenColorCells);
    rhymeClassifier.setMinRepeatLength(prev.minRepeatLength);
    customSpokenSyllableCounts = std::move(prev.customSpokenSyllableCounts);
    totalBars = prev.totalBars;
    barHeight = prev.barHeight;
    viewMode = prev.viewMode;
    darkMode = prev.darkMode;
    customStanzaBreaksActive = prev.customStanzaBreaksActive;
    stanzaBreaks = std::move(prev.stanzaBreaks);
    NotebookLookAndFeel::setDarkMode(darkMode);
    bool notationChanged = (defaultNotation != prev.defaultNotation || barNotations != prev.barNotations);
    defaultNotation = prev.defaultNotation;
    barNotations = std::move(prev.barNotations);

    refreshRhymes();
    if (notationChanged)
        notifyNotationChanged();
    notifyChanged();
}

void LyricDocument::redo()
{
    if (redoStack.empty())
        return;

    DocumentSnapshot current;
    current.barData = barData;
    current.defaultNotation = defaultNotation;
    current.barNotations = barNotations;
    current.boldCells = boldCells;
    current.cellAlignments = cellAlignments;
    current.showAlignmentControls = showAlignmentControls;
    current.customCellColors = customCellColors;
    current.hiddenColorCells = hiddenColorCells;
    current.minRepeatLength = rhymeClassifier.getMinRepeatLength();
    current.customSpokenSyllableCounts = customSpokenSyllableCounts;
    current.totalBars = totalBars;
    current.barHeight = barHeight;
    current.viewMode = viewMode;
    current.darkMode = darkMode;
    current.customStanzaBreaksActive = customStanzaBreaksActive;
    current.stanzaBreaks = stanzaBreaks;
    undoStack.push_back(std::move(current));

    DocumentSnapshot next = std::move(redoStack.back());
    redoStack.pop_back();

    barData = std::move(next.barData);
    boldCells = std::move(next.boldCells);
    cellAlignments = std::move(next.cellAlignments);
    showAlignmentControls = next.showAlignmentControls;
    customCellColors = std::move(next.customCellColors);
    hiddenColorCells = std::move(next.hiddenColorCells);
    rhymeClassifier.setMinRepeatLength(next.minRepeatLength);
    customSpokenSyllableCounts = std::move(next.customSpokenSyllableCounts);
    totalBars = next.totalBars;
    barHeight = next.barHeight;
    viewMode = next.viewMode;
    darkMode = next.darkMode;
    customStanzaBreaksActive = next.customStanzaBreaksActive;
    stanzaBreaks = std::move(next.stanzaBreaks);
    NotebookLookAndFeel::setDarkMode(darkMode);
    bool notationChanged = (defaultNotation != next.defaultNotation || barNotations != next.barNotations);
    defaultNotation = next.defaultNotation;
    barNotations = std::move(next.barNotations);

    refreshRhymes();
    if (notationChanged)
        notifyNotationChanged();
    notifyChanged();
}

void LyricDocument::setBarHeight(int h)
{
    int clamped = std::clamp(h, 32, 100);
    if (barHeight != clamped)
    {
        pushUndoSnapshot();
        barHeight = clamped;
        notifyChanged();
    }
}

void LyricDocument::clearCells(const std::vector<std::pair<int, int>>& cells)
{
    if (cells.empty())
        return;

    pushUndoSnapshot();

    for (const auto& cell : cells)
    {
        int b = cell.first;
        int s = cell.second;
        auto it = barData.find(b);
        if (it != barData.end() && s >= 0 && s < (int)it->second.size())
        {
            it->second[s] = "";
        }
    }

    refreshRhymes();
    notifyChanged();
}

void LyricDocument::joinCells(const std::vector<std::pair<int, int>>& cells)
{
    if (cells.size() < 2)
        return;

    std::vector<std::pair<int, int>> sorted = cells;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second < b.second;
    });

    pushUndoSnapshot();

    juce::String combined;
    for (size_t i = 0; i < sorted.size(); ++i)
    {
        juce::String syl = getSyllable(sorted[i].first, sorted[i].second).trim();
        if (syl.isNotEmpty())
        {
            if (combined.isNotEmpty())
            {
                if (combined.endsWith("-"))
                    combined += syl;
                else
                    combined += " " + syl;
            }
            else
            {
                combined = syl;
            }
        }
    }

    // Set first cell to combined text
    auto first = sorted[0];
    ensureBarCount(first.first + 1);
    auto& vec = barData[first.first];
    int totalNeeded = std::max(getNotation(first.first).getTotalSyllables(), first.second + 1);
    if ((int)vec.size() < totalNeeded)
        vec.resize(totalNeeded);
    vec[first.second] = combined;

    // Clear remaining cells
    for (size_t i = 1; i < sorted.size(); ++i)
    {
        auto other = sorted[i];
        auto it = barData.find(other.first);
        if (it != barData.end() && other.second >= 0 && other.second < (int)it->second.size())
        {
            it->second[other.second] = "";
        }
    }

    refreshRhymes();
    notifyChanged();
}

void LyricDocument::joinWithNext(int barIndex, int globalSyllableIndex)
{
    int nextBar = barIndex;
    int nextSyl = globalSyllableIndex + 1;
    int totalSyls = getNotation(barIndex).getTotalSyllables();

    if (nextSyl >= totalSyls)
    {
        nextBar++;
        nextSyl = 0;
    }

    joinCells({ { barIndex, globalSyllableIndex }, { nextBar, nextSyl } });
}

void LyricDocument::splitCell(int barIndex, int globalSyllableIndex)
{
    juce::String text = getSyllable(barIndex, globalSyllableIndex).trim();
    if (text.isEmpty())
        return;

    auto syls = SyllableSplitter::splitLineIntoSyllables(text);
    if (syls.size() <= 1)
        return;

    insertTextFlow(barIndex, globalSyllableIndex, text);
}

bool LyricDocument::isCellBold(int barIndex, int globalSyllableIndex) const
{
    return boldCells.find({ barIndex, globalSyllableIndex }) != boldCells.end();
}

void LyricDocument::setCellBold(int barIndex, int globalSyllableIndex, bool bold, bool notify)
{
    bool wasBold = isCellBold(barIndex, globalSyllableIndex);
    if (wasBold != bold)
    {
        pushUndoSnapshot();
        if (bold)
            boldCells.insert({ barIndex, globalSyllableIndex });
        else
            boldCells.erase({ barIndex, globalSyllableIndex });

        if (notify)
            notifyChanged();
    }
}

void LyricDocument::toggleCellBold(int barIndex, int globalSyllableIndex)
{
    setCellBold(barIndex, globalSyllableIndex, !isCellBold(barIndex, globalSyllableIndex), true);
}

void LyricDocument::toggleSelectedBold()
{
    if (selectedCells.empty())
        return;

    pushUndoSnapshot();

    bool allBold = true;
    for (const auto& cell : selectedCells)
    {
        if (!isCellBold(cell.first, cell.second))
        {
            allBold = false;
            break;
        }
    }

    for (const auto& cell : selectedCells)
    {
        if (allBold)
            boldCells.erase(cell);
        else
            boldCells.insert(cell);
    }

    notifyChanged();
}

LyricDocument::CellAlignment LyricDocument::getCellAlignment(int barIndex, int globalSyllableIndex) const
{
    auto it = cellAlignments.find({ barIndex, globalSyllableIndex });
    if (it != cellAlignments.end())
        return it->second;
    return AlignCenter;
}

void LyricDocument::setCellAlignment(int barIndex, int globalSyllableIndex, CellAlignment align, bool notify)
{
    if (barIndex < 0 || globalSyllableIndex < 0)
        return;

    auto key = std::make_pair(barIndex, globalSyllableIndex);
    if (getCellAlignment(barIndex, globalSyllableIndex) == align)
        return;

    if (notify)
        pushUndoSnapshot();

    if (align == AlignCenter)
        cellAlignments.erase(key);
    else
        cellAlignments[key] = align;

    if (notify)
        notifyChanged();
}

void LyricDocument::setSelectionAlignment(CellAlignment align)
{
    if (selectedCells.empty())
        return;

    pushUndoSnapshot();
    for (const auto& cell : selectedCells)
    {
        if (align == AlignCenter)
            cellAlignments.erase(cell);
        else
            cellAlignments[cell] = align;
    }
    notifyChanged();
}

void LyricDocument::setShowAlignmentControls(bool show, bool notify)
{
    if (showAlignmentControls != show)
    {
        if (notify)
            pushUndoSnapshot();

        showAlignmentControls = show;

        if (notify)
            notifyChanged();
    }
}

bool LyricDocument::hasCustomCellColor(int barIndex, int globalSyllableIndex) const
{
    return customCellColors.find({ barIndex, globalSyllableIndex }) != customCellColors.end();
}

juce::Colour LyricDocument::getCustomCellColor(int barIndex, int globalSyllableIndex) const
{
    auto it = customCellColors.find({ barIndex, globalSyllableIndex });
    if (it != customCellColors.end())
        return it->second;
    return juce::Colours::transparentBlack;
}

void LyricDocument::setCustomCellColor(int barIndex, int globalSyllableIndex, const juce::Colour& color, bool notify)
{
    if (barIndex < 0 || globalSyllableIndex < 0)
        return;

    auto key = std::make_pair(barIndex, globalSyllableIndex);
    auto it = customCellColors.find(key);
    if (it != customCellColors.end() && it->second == color)
        return;

    if (notify)
        pushUndoSnapshot();

    customCellColors[key] = color;

    if (notify)
        notifyChanged();
}

void LyricDocument::clearCustomCellColor(int barIndex, int globalSyllableIndex, bool notify)
{
    auto key = std::make_pair(barIndex, globalSyllableIndex);
    auto it = customCellColors.find(key);
    if (it != customCellColors.end())
    {
        if (notify)
            pushUndoSnapshot();

        customCellColors.erase(it);

        if (notify)
            notifyChanged();
    }
}

void LyricDocument::setSelectionCustomColor(const juce::Colour& color)
{
    if (selectedCells.empty())
        return;

    pushUndoSnapshot();
    for (const auto& cell : selectedCells)
    {
        customCellColors[cell] = color;
    }
    notifyChanged();
}

void LyricDocument::clearSelectionCustomColor()
{
    if (selectedCells.empty())
        return;

    pushUndoSnapshot();
    for (const auto& cell : selectedCells)
    {
        customCellColors.erase(cell);
    }
    notifyChanged();
}

void LyricDocument::clearAllCustomCellColors(bool notify)
{
    if (customCellColors.empty())
        return;

    if (notify)
        pushUndoSnapshot();

    customCellColors.clear();

    if (notify)
        notifyChanged();
}

void LyricDocument::insertSyllableInBar(int barIndex, int globalSyllableIndex, bool insertAfter)
{
    if (barIndex < 0)
        return;

    ensureBarCount(barIndex + 1);

    MetricNotation notat = getNotation(barIndex);
    auto [pulseIdx, sInPulse] = notat.getPulseAndSyllableFromGlobal(globalSyllableIndex);
    if (pulseIdx < 0 || pulseIdx >= notat.getPulseCount())
        return;

    pushUndoSnapshot();

    int insertPos = insertAfter ? (globalSyllableIndex + 1) : globalSyllableIndex;

    // 1. Increment subdivision count of this pulse group in this bar's notation
    int curSubdivs = notat.getSyllablesForPulse(pulseIdx);
    notat.setSyllablesForPulse(pulseIdx, curSubdivs + 1);
    barNotations[barIndex] = notat;

    // 2. Shift syllables in barData
    auto& syllables = barData[barIndex];
    if (insertPos < (int)syllables.size())
    {
        syllables.insert(syllables.begin() + insertPos, juce::String());
    }
    else
    {
        syllables.resize(insertPos + 1, juce::String());
    }

    // 3. Shift boldCells
    std::vector<int> bShift;
    for (auto it = boldCells.begin(); it != boldCells.end(); )
    {
        if (it->first == barIndex && it->second >= insertPos)
        {
            bShift.push_back(it->second);
            it = boldCells.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (int idx : bShift)
        boldCells.insert({ barIndex, idx + 1 });

    // 4. Shift cellAlignments
    std::vector<std::pair<int, CellAlignment>> aShift;
    for (auto it = cellAlignments.begin(); it != cellAlignments.end(); )
    {
        if (it->first.first == barIndex && it->first.second >= insertPos)
        {
            aShift.push_back({ it->first.second, it->second });
            it = cellAlignments.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (const auto& p : aShift)
        cellAlignments[{ barIndex, p.first + 1 }] = p.second;

    // 5. Shift customCellColors
    std::vector<std::pair<int, juce::Colour>> cShift;
    for (auto it = customCellColors.begin(); it != customCellColors.end(); )
    {
        if (it->first.first == barIndex && it->first.second >= insertPos)
        {
            cShift.push_back({ it->first.second, it->second });
            it = customCellColors.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (const auto& p : cShift)
        customCellColors[{ barIndex, p.first + 1 }] = p.second;

    // 6. Shift hiddenColorCells
    std::vector<int> hShift;
    for (auto it = hiddenColorCells.begin(); it != hiddenColorCells.end(); )
    {
        if (it->first == barIndex && it->second >= insertPos)
        {
            hShift.push_back(it->second);
            it = hiddenColorCells.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (int sIdx : hShift)
        hiddenColorCells.insert({ barIndex, sIdx + 1 });

    // 7. Shift selectedCells
    selectedCells.clear();
    selectedCells.insert({ barIndex, insertPos });

    // 8. Refresh & Notify
    refreshRhymes();
    notifyBarNotationChanged(barIndex);
    notifyChanged();
    notifySelectionChanged();
}

void LyricDocument::deleteSyllableInBar(int barIndex, int globalSyllableIndex)
{
    if (barIndex < 0)
        return;

    MetricNotation notat = getNotation(barIndex);
    auto [pulseIdx, sInPulse] = notat.getPulseAndSyllableFromGlobal(globalSyllableIndex);
    if (pulseIdx < 0 || pulseIdx >= notat.getPulseCount())
        return;

    int curSubdivs = notat.getSyllablesForPulse(pulseIdx);
    if (curSubdivs <= 1)
        return; // Don't reduce pulse subdivisions below 1

    pushUndoSnapshot();

    // 1. Decrement subdivision count in notation
    notat.setSyllablesForPulse(pulseIdx, curSubdivs - 1);
    barNotations[barIndex] = notat;

    // 2. Erase syllable at globalSyllableIndex and shift left
    auto it = barData.find(barIndex);
    if (it != barData.end() && globalSyllableIndex < (int)it->second.size())
    {
        it->second.erase(it->second.begin() + globalSyllableIndex);
    }

    // 3. Shift boldCells
    std::vector<int> bShift;
    for (auto bit = boldCells.begin(); bit != boldCells.end(); )
    {
        if (bit->first == barIndex)
        {
            if (bit->second == globalSyllableIndex)
            {
                bit = boldCells.erase(bit);
            }
            else if (bit->second > globalSyllableIndex)
            {
                bShift.push_back(bit->second);
                bit = boldCells.erase(bit);
            }
            else
            {
                ++bit;
            }
        }
        else
        {
            ++bit;
        }
    }
    for (int idx : bShift)
        boldCells.insert({ barIndex, idx - 1 });

    // 4. Shift cellAlignments
    std::vector<std::pair<int, CellAlignment>> aShift;
    for (auto ait = cellAlignments.begin(); ait != cellAlignments.end(); )
    {
        if (ait->first.first == barIndex)
        {
            if (ait->first.second == globalSyllableIndex)
            {
                ait = cellAlignments.erase(ait);
            }
            else if (ait->first.second > globalSyllableIndex)
            {
                aShift.push_back({ ait->first.second, ait->second });
                ait = cellAlignments.erase(ait);
            }
            else
            {
                ++ait;
            }
        }
        else
        {
            ++ait;
        }
    }
    for (const auto& p : aShift)
        cellAlignments[{ barIndex, p.first - 1 }] = p.second;

    // 5. Shift customCellColors
    std::vector<std::pair<int, juce::Colour>> cShift;
    for (auto cit = customCellColors.begin(); cit != customCellColors.end(); )
    {
        if (cit->first.first == barIndex)
        {
            if (cit->first.second == globalSyllableIndex)
            {
                cit = customCellColors.erase(cit);
            }
            else if (cit->first.second > globalSyllableIndex)
            {
                cShift.push_back({ cit->first.second, cit->second });
                cit = customCellColors.erase(cit);
            }
            else
            {
                ++cit;
            }
        }
        else
        {
            ++cit;
        }
    }
    for (const auto& p : cShift)
        customCellColors[{ barIndex, p.first - 1 }] = p.second;

    // 6. Shift hiddenColorCells
    std::set<std::pair<int, int>> hShift;
    for (auto hit = hiddenColorCells.begin(); hit != hiddenColorCells.end(); )
    {
        if (hit->first == barIndex)
        {
            if (hit->second == globalSyllableIndex)
            {
                hit = hiddenColorCells.erase(hit);
            }
            else if (hit->second > globalSyllableIndex)
            {
                hShift.insert({ barIndex, hit->second - 1 });
                hit = hiddenColorCells.erase(hit);
            }
            else
            {
                ++hit;
            }
        }
        else
        {
            ++hit;
        }
    }
    for (const auto& p : hShift)
        hiddenColorCells.insert(p);

    // 7. Shift selectedCells
    selectedCells.clear();
    int newTotal = notat.getTotalSyllables();
    int newSelIdx = std::clamp(globalSyllableIndex, 0, std::max(0, newTotal - 1));
    selectedCells.insert({ barIndex, newSelIdx });

    // 8. Clamp customSpokenSyllableCounts
    auto spIt = customSpokenSyllableCounts.find(barIndex);
    if (spIt != customSpokenSyllableCounts.end() && spIt->second > newTotal)
    {
        spIt->second = newTotal;
    }

    // 9. Refresh & Notify
    refreshRhymes();
    notifyBarNotationChanged(barIndex);
    notifyChanged();
    notifySelectionChanged();
}

static juce::String toTitleCaseString(const juce::String& text)
{
    if (text.isEmpty())
        return {};

    juce::String res = text.toLowerCase();
    for (int i = 0; i < res.length(); ++i)
    {
        auto c = res[i];
        if (juce::CharacterFunctions::isLetter(c))
        {
            res = res.substring(0, i) + juce::String::charToString(juce::CharacterFunctions::toUpperCase(c)) + res.substring(i + 1);
            break;
        }
    }
    return res;
}

void LyricDocument::transformCellCase(int barIndex, int globalSyllableIndex, CaseTransform mode)
{
    juce::String cur = getSyllable(barIndex, globalSyllableIndex);
    if (cur.isEmpty())
        return;

    juce::String transformed;
    if (mode == CaseUpper)
        transformed = cur.toUpperCase();
    else if (mode == CaseLower)
        transformed = cur.toLowerCase();
    else if (mode == CaseTitle)
        transformed = toTitleCaseString(cur);

    if (transformed != cur)
    {
        pushUndoSnapshot();
        setSyllable(barIndex, globalSyllableIndex, transformed, false);
        refreshRhymes();
        notifyChanged();
    }
}

void LyricDocument::transformSelectionCase(CaseTransform mode)
{
    if (selectedCells.empty())
        return;

    pushUndoSnapshot();
    for (const auto& cell : selectedCells)
    {
        juce::String cur = getSyllable(cell.first, cell.second);
        if (cur.isNotEmpty())
        {
            juce::String transformed;
            if (mode == CaseUpper)
                transformed = cur.toUpperCase();
            else if (mode == CaseLower)
                transformed = cur.toLowerCase();
            else if (mode == CaseTitle)
                transformed = toTitleCaseString(cur);

            setSyllable(cell.first, cell.second, transformed, false);
        }
    }
    refreshRhymes();
    notifyChanged();
}

void LyricDocument::duplicateCellToNext(int barIndex, int globalSyllableIndex)
{
    juce::String curText = getSyllable(barIndex, globalSyllableIndex);
    if (curText.isEmpty())
        return;

    int nextBar = barIndex;
    int nextSyl = globalSyllableIndex + 1;
    int totalSyls = getNotation(barIndex).getTotalSyllables();
    if (nextSyl >= totalSyls)
    {
        nextBar++;
        nextSyl = 0;
    }

    pushUndoSnapshot();
    setSyllable(nextBar, nextSyl, curText, false);

    if (isCellBold(barIndex, globalSyllableIndex))
        setCellBold(nextBar, nextSyl, true, false);
    if (getCellAlignment(barIndex, globalSyllableIndex) != AlignCenter)
        setCellAlignment(nextBar, nextSyl, getCellAlignment(barIndex, globalSyllableIndex), false);
    if (hasCustomCellColor(barIndex, globalSyllableIndex))
        setCustomCellColor(nextBar, nextSyl, getCustomCellColor(barIndex, globalSyllableIndex), false);

    refreshRhymes();
    notifyChanged();
}

int LyricDocument::getCalculatedSyllableCount(int barIndex) const
{
    int maxSyllables = getNotation(barIndex).getTotalSyllables();
    auto it = barData.find(barIndex);
    if (it != barData.end())
    {
        int nonBlank = 0;
        int limit = std::min((int)it->second.size(), maxSyllables);
        for (int i = 0; i < limit; ++i)
        {
            if (it->second[i].trim().isNotEmpty())
                nonBlank++;
        }
        if (nonBlank > 0)
            return nonBlank;
    }
    return maxSyllables;
}

int LyricDocument::getActualSpokenSyllableCount(int barIndex) const
{
    auto it = customSpokenSyllableCounts.find(barIndex);
    if (it != customSpokenSyllableCounts.end())
        return it->second;
    return getCalculatedSyllableCount(barIndex);
}

void LyricDocument::setActualSpokenSyllableCount(int barIndex, int count)
{
    count = std::max(0, count);
    if (customSpokenSyllableCounts.find(barIndex) == customSpokenSyllableCounts.end() ||
        customSpokenSyllableCounts[barIndex] != count)
    {
        pushUndoSnapshot();
        customSpokenSyllableCounts[barIndex] = count;
        notifyChanged();
    }
}

void LyricDocument::resetActualSpokenSyllableCount(int barIndex)
{
    if (customSpokenSyllableCounts.find(barIndex) != customSpokenSyllableCounts.end())
    {
        pushUndoSnapshot();
        customSpokenSyllableCounts.erase(barIndex);
        notifyChanged();
    }
}

bool LyricDocument::hasCustomSpokenSyllableCount(int barIndex) const
{
    return customSpokenSyllableCounts.find(barIndex) != customSpokenSyllableCounts.end();
}

void LyricDocument::setViewMode(ViewMode mode)
{
    if (viewMode != mode)
    {
        pushUndoSnapshot();
        viewMode = mode;
        notifyChanged();
    }
}

void LyricDocument::setDarkMode(bool dark)
{
    if (darkMode != dark)
    {
        pushUndoSnapshot();
        darkMode = dark;
        NotebookLookAndFeel::setDarkMode(dark);
        notifyChanged();
    }
}

int LyricDocument::getVisibleStartBar() const
{
    if (viewMode == ModeScroll)
        return 0;
    return getFirstBarOfCurrentPage();
}

int LyricDocument::getVisibleEndBar() const
{
    if (viewMode == ModeScroll)
        return totalBars;
    return getLastBarOfCurrentPage();
}

void LyricDocument::setCurrentPage(int page)
{
    int maxPage = getTotalPages() - 1;
    int clamped = std::clamp(page, 0, std::max(0, maxPage));
    if (currentPage != clamped)
    {
        currentPage = clamped;
        notifyChanged();
    }
}

int LyricDocument::getTotalPages() const
{
    return std::max(1, (totalBars + barsPerPage - 1) / barsPerPage);
}

void LyricDocument::addPage()
{
    if (getTotalPages() < 64)
    {
        pushUndoSnapshot();
        totalBars += barsPerPage;
        notifyChanged();
    }
}

void LyricDocument::setBarSpacing(BarSpacing spacing)
{
    if (barSpacing != spacing || customStanzaBreaksActive)
    {
        customStanzaBreaksActive = false;
        barSpacing = spacing;
        notifyChanged();
    }
}

bool LyricDocument::shouldAddSpacingAfterBar(int barIndex) const
{
    if (customStanzaBreaksActive)
    {
        return stanzaBreaks.find(barIndex) != stanzaBreaks.end();
    }

    int barNum = barIndex + 1; // 1-based bar number
    switch (barSpacing)
    {
        case Spacing4:  return (barNum % 4 == 0);
        case Spacing8:  return (barNum % 8 == 0);
        case Spacing16: return (barNum % 16 == 0);
        case SpacingOff:
        default:        return false;
    }
}

void LyricDocument::toggleStanzaBreak(int barIndex)
{
    pushUndoSnapshot();
    if (!customStanzaBreaksActive)
    {
        customStanzaBreaksActive = true;
        stanzaBreaks.clear();
        for (int b = 0; b < totalBars; ++b)
        {
            if ((b + 1) % 4 == 0)
                stanzaBreaks.insert(b);
        }
    }

    if (stanzaBreaks.find(barIndex) != stanzaBreaks.end())
        stanzaBreaks.erase(barIndex);
    else
        stanzaBreaks.insert(barIndex);

    notifyChanged();
}

void LyricDocument::setStanzaBreak(int barIndex, bool enabled)
{
    if (shouldAddSpacingAfterBar(barIndex) == enabled)
        return;
    toggleStanzaBreak(barIndex);
}

void LyricDocument::insertBar(int afterBarIndex)
{
    if (afterBarIndex < 0)
        afterBarIndex = 0;

    pushUndoSnapshot();

    int insertPos = afterBarIndex + 1;
    ensureBarCount(totalBars + 1);

    // 1. Shift barData downwards from back to insertPos
    for (int b = totalBars - 1; b >= insertPos; --b)
    {
        auto it = barData.find(b);
        if (it != barData.end())
        {
            barData[b + 1] = std::move(it->second);
            barData.erase(it);
        }
        else
        {
            barData.erase(b + 1);
        }
    }

    // 2. Shift barNotations
    for (int b = totalBars - 1; b >= insertPos; --b)
    {
        auto it = barNotations.find(b);
        if (it != barNotations.end())
        {
            barNotations[b + 1] = it->second;
            barNotations.erase(it);
        }
        else
        {
            barNotations.erase(b + 1);
        }
    }

    // 3. Shift custom spoken counts
    for (int b = totalBars - 1; b >= insertPos; --b)
    {
        auto it = customSpokenSyllableCounts.find(b);
        if (it != customSpokenSyllableCounts.end())
        {
            customSpokenSyllableCounts[b + 1] = it->second;
            customSpokenSyllableCounts.erase(it);
        }
        else
        {
            customSpokenSyllableCounts.erase(b + 1);
        }
    }

    // 4. Shift bold cells
    std::set<std::pair<int, int>> newBold;
    for (const auto& cell : boldCells)
    {
        if (cell.first >= insertPos)
            newBold.insert({ cell.first + 1, cell.second });
        else
            newBold.insert(cell);
    }
    boldCells = std::move(newBold);

    // 5. Shift cell alignments
    std::map<std::pair<int, int>, CellAlignment> newAlign;
    for (const auto& pair : cellAlignments)
    {
        if (pair.first.first >= insertPos)
            newAlign[{ pair.first.first + 1, pair.first.second }] = pair.second;
        else
            newAlign[pair.first] = pair.second;
    }
    cellAlignments = std::move(newAlign);

    // 6. Shift custom cell colors
    std::map<std::pair<int, int>, juce::Colour> newColors;
    for (const auto& pair : customCellColors)
    {
        if (pair.first.first >= insertPos)
            newColors[{ pair.first.first + 1, pair.first.second }] = pair.second;
        else
            newColors[pair.first] = pair.second;
    }
    customCellColors = std::move(newColors);

    // 7. Shift hidden color cells
    std::set<std::pair<int, int>> newHidden;
    for (const auto& cell : hiddenColorCells)
    {
        if (cell.first >= insertPos)
            newHidden.insert({ cell.first + 1, cell.second });
        else
            newHidden.insert(cell);
    }
    hiddenColorCells = std::move(newHidden);

    // 8. Shift custom stanza breaks
    if (customStanzaBreaksActive)
    {
        std::set<int> newBreaks;
        for (int b : stanzaBreaks)
        {
            if (b >= insertPos)
                newBreaks.insert(b + 1);
            else
                newBreaks.insert(b);
        }
        stanzaBreaks = std::move(newBreaks);
    }

    // 9. Inserted bar gets the notation of afterBarIndex
    MetricNotation inheritedMeter = getNotation(afterBarIndex);
    barNotations[insertPos] = inheritedMeter;
    barData[insertPos] = std::vector<juce::String>(inheritedMeter.getTotalSyllables());

    totalBars++;

    refreshRhymes();
    notifyNotationChanged();
    notifyChanged();
}

void LyricDocument::refreshRhymes()
{
    std::vector<std::vector<juce::String>> lines;
    std::vector<DocumentSyllable> allSyllables;

    for (int b = 0; b < totalBars; ++b)
    {
        auto it = barData.find(b);
        if (it != barData.end() && !it->second.empty())
        {
            int maxS = getNotation(b).getTotalSyllables();
            std::vector<juce::String> line;
            int limit = std::min((int)it->second.size(), maxS);
            for (int s = 0; s < limit; ++s)
            {
                const juce::String& text = it->second[s];
                line.push_back(text);
                if (text.trim().isNotEmpty())
                {
                    allSyllables.push_back({ b, s, text });
                }
            }
            lines.push_back(line);
        }
    }
    rhymeClassifier.updateRhymeMapWithContext(lines, hiddenColorCells);
    rhymeClassifier.updateRepetitionMap(allSyllables, hiddenColorCells);
}

void LyricDocument::setMinRepeatLength(int len)
{
    int clamped = std::clamp(len, 2, 4);
    if (rhymeClassifier.getMinRepeatLength() != clamped)
    {
        pushUndoSnapshot();
        rhymeClassifier.setMinRepeatLength(clamped);
        refreshRhymes();
        notifyChanged();
    }
}

bool LyricDocument::isSequenceHiddenAt(int barIndex, int globalSylIndex) const
{
    return hiddenColorCells.find({ barIndex, globalSylIndex }) != hiddenColorCells.end();
}

void LyricDocument::toggleHideSequenceAt(int barIndex, int globalSylIndex)
{
    pushUndoSnapshot();

    if (isSequenceHiddenAt(barIndex, globalSylIndex))
    {
        // Unhide
        auto span = rhymeClassifier.getRepeatSequenceSpanAt(barIndex, globalSylIndex);
        if (span.empty())
        {
            hiddenColorCells.erase({ barIndex, globalSylIndex });
        }
        else
        {
            for (const auto& cell : span)
                hiddenColorCells.erase(cell);
        }
    }
    else
    {
        // Hide
        auto span = rhymeClassifier.getRepeatSequenceSpanAt(barIndex, globalSylIndex);
        if (span.empty())
        {
            hiddenColorCells.insert({ barIndex, globalSylIndex });
        }
        else
        {
            for (const auto& cell : span)
                hiddenColorCells.insert(cell);
        }
    }

    refreshRhymes();
    notifyChanged();
}

void LyricDocument::unhideAllSequences()
{
    if (!hiddenColorCells.empty())
    {
        pushUndoSnapshot();
        hiddenColorCells.clear();
        refreshRhymes();
        notifyChanged();
    }
}

juce::Colour LyricDocument::getVowelSoundColor(const juce::String& vowelKey) const
{
    return rhymeClassifier.getVowelColour(vowelKey);
}

void LyricDocument::setVowelSoundColor(const juce::String& vowelKey, const juce::Colour& colour)
{
    rhymeClassifier.setVowelColour(vowelKey, colour);
    refreshRhymes();
    notifyChanged();
}

void LyricDocument::resetVowelSoundColorsToDefaults()
{
    rhymeClassifier.resetVowelColoursToDefaults();
    refreshRhymes();
    notifyChanged();
}

juce::ValueTree LyricDocument::toValueTree() const
{
    juce::ValueTree vt("CompassCadenceDocument");
    vt.setProperty("notation", defaultNotation.toNotationString(), nullptr);
    vt.setProperty("totalBars", totalBars, nullptr);
    vt.setProperty("barsPerPage", barsPerPage, nullptr);
    vt.setProperty("currentPage", currentPage, nullptr);
    vt.setProperty("barSpacing", (int)barSpacing, nullptr);
    vt.setProperty("viewMode", (int)viewMode, nullptr);
    vt.setProperty("barHeight", barHeight, nullptr);
    vt.setProperty("darkMode", darkMode, nullptr);
    vt.setProperty("showAlignmentControls", showAlignmentControls, nullptr);
    vt.setProperty("rhymeHighlight", rhymeClassifier.isEnabled(), nullptr);
    vt.setProperty("colorMode", (int)rhymeClassifier.getColorMode(), nullptr);
    vt.setProperty("minRepeatLength", rhymeClassifier.getMinRepeatLength(), nullptr);
    if (!hiddenColorCells.empty())
    {
        juce::String hiddenStr;
        for (const auto& cell : hiddenColorCells)
        {
            if (hiddenStr.isNotEmpty()) hiddenStr += ";";
            hiddenStr += juce::String(cell.first) + "," + juce::String(cell.second);
        }
        vt.setProperty("hiddenColorCells", hiddenStr, nullptr);
    }
    vt.setProperty("customStanzaBreaksActive", customStanzaBreaksActive, nullptr);
    if (customStanzaBreaksActive)
    {
        juce::String breaksStr;
        for (int b : stanzaBreaks)
        {
            if (breaksStr.isNotEmpty()) breaksStr += ",";
            breaksStr += juce::String(b);
        }
        vt.setProperty("stanzaBreaks", breaksStr, nullptr);
    }

    const auto& customVowels = rhymeClassifier.getCustomVowelColours();
    if (!customVowels.empty())
    {
        juce::ValueTree vowelNode("VowelColors");
        for (const auto& kv : customVowels)
        {
            juce::ValueTree item("Color");
            item.setProperty("key", juce::String(kv.first), nullptr);
            item.setProperty("colour", kv.second.toString(), nullptr);
            vowelNode.addChild(item, -1, nullptr);
        }
        vt.addChild(vowelNode, -1, nullptr);
    }

    juce::ValueTree barsNode("Bars");
    for (const auto& pair : barData)
    {
        bool hasContent = false;
        for (const auto& s : pair.second)
        {
            if (s.trim().isNotEmpty()) { hasContent = true; break; }
        }

        bool hasOverride = hasBarNotationOverride(pair.first);

        if (hasContent || hasOverride)
        {
            juce::ValueTree bNode("Bar");
            bNode.setProperty("index", pair.first, nullptr);
            if (hasOverride)
            {
                bNode.setProperty("notation", getNotation(pair.first).toNotationString(), nullptr);
            }
            if (hasCustomSpokenSyllableCount(pair.first))
            {
                bNode.setProperty("spokenSyllables", getActualSpokenSyllableCount(pair.first), nullptr);
            }

            for (size_t s = 0; s < pair.second.size(); ++s)
            {
                if (pair.second[s].isNotEmpty())
                {
                    juce::ValueTree sNode("Syllable");
                    sNode.setProperty("index", (int)s, nullptr);
                    sNode.setProperty("text", pair.second[s], nullptr);
                    if (isCellBold(pair.first, (int)s))
                        sNode.setProperty("bold", 1, nullptr);
                    auto align = getCellAlignment(pair.first, (int)s);
                    if (align != AlignCenter)
                        sNode.setProperty("align", (int)align, nullptr);
                    if (hasCustomCellColor(pair.first, (int)s))
                        sNode.setProperty("customColor", getCustomCellColor(pair.first, (int)s).toString(), nullptr);
                    bNode.addChild(sNode, -1, nullptr);
                }
            }
            barsNode.addChild(bNode, -1, nullptr);
        }
    }
    vt.addChild(barsNode, -1, nullptr);

    return vt;
}

void LyricDocument::fromValueTree(const juce::ValueTree& vt)
{
    if (!vt.isValid() || !vt.hasType("CompassCadenceDocument"))
        return;

    if (vt.hasProperty("notation"))
        defaultNotation = MetricNotation::fromNotationString(vt.getProperty("notation").toString());

    barNotations.clear();
    boldCells.clear();
    cellAlignments.clear();
    customCellColors.clear();
    customSpokenSyllableCounts.clear();
    hiddenColorCells.clear();
    totalBars = vt.getProperty("totalBars", totalBars);
    barsPerPage = vt.getProperty("barsPerPage", barsPerPage);
    currentPage = vt.getProperty("currentPage", currentPage);
    barSpacing = (BarSpacing)(int)vt.getProperty("barSpacing", (int)barSpacing);
    viewMode = (ViewMode)(int)vt.getProperty("viewMode", (int)ModeScroll);
    barHeight = vt.getProperty("barHeight", 50);
    darkMode = vt.getProperty("darkMode", false);
    showAlignmentControls = vt.getProperty("showAlignmentControls", false);
    NotebookLookAndFeel::setDarkMode(darkMode);
    if (vt.hasProperty("colorMode"))
    {
        int cm = vt.getProperty("colorMode");
        rhymeClassifier.setColorMode(static_cast<RhymeClassifier::ColorMode>(std::clamp(cm, 0, 2)));
    }
    else
    {
        rhymeClassifier.setEnabled(vt.getProperty("rhymeHighlight", true));
    }

    if (vt.hasProperty("minRepeatLength"))
    {
        int mrl = (int)vt.getProperty("minRepeatLength", 2);
        rhymeClassifier.setMinRepeatLength(mrl);
    }
    else
    {
        rhymeClassifier.setMinRepeatLength(2);
    }

    if (vt.hasProperty("hiddenColorCells"))
    {
        auto pairs = juce::StringArray::fromTokens(vt.getProperty("hiddenColorCells").toString(), ";", "");
        for (const auto& p : pairs)
        {
            auto coords = juce::StringArray::fromTokens(p.trim(), ",", "");
            if (coords.size() == 2)
            {
                int b = coords[0].getIntValue();
                int s = coords[1].getIntValue();
                hiddenColorCells.insert({ b, s });
            }
        }
    }

    auto vowelNode = vt.getChildWithName("VowelColors");
    if (vowelNode.isValid())
    {
        std::unordered_map<std::string, juce::Colour> colMap;
        for (int i = 0; i < vowelNode.getNumChildren(); ++i)
        {
            auto item = vowelNode.getChild(i);
            juce::String key = item.getProperty("key").toString();
            juce::String colStr = item.getProperty("colour").toString();
            if (key.isNotEmpty() && colStr.isNotEmpty())
            {
                colMap[key.toStdString()] = juce::Colour::fromString(colStr);
            }
        }
        rhymeClassifier.setCustomVowelColours(colMap);
    }
    else
    {
        rhymeClassifier.resetVowelColoursToDefaults();
    }

    customStanzaBreaksActive = vt.getProperty("customStanzaBreaksActive", false);
    stanzaBreaks.clear();
    if (vt.hasProperty("stanzaBreaks"))
    {
        auto tokens = juce::StringArray::fromTokens(vt.getProperty("stanzaBreaks").toString(), ",", "");
        for (const auto& t : tokens)
        {
            int b = t.trim().getIntValue();
            stanzaBreaks.insert(b);
        }
    }

    barData.clear();
    auto barsNode = vt.getChildWithName("Bars");
    if (barsNode.isValid())
    {
        for (int i = 0; i < barsNode.getNumChildren(); ++i)
        {
            auto bNode = barsNode.getChild(i);
            int bIdx = bNode.getProperty("index", -1);
            if (bIdx >= 0)
            {
                if (bNode.hasProperty("notation"))
                {
                    barNotations[bIdx] = MetricNotation::fromNotationString(bNode.getProperty("notation").toString());
                }
                if (bNode.hasProperty("spokenSyllables"))
                {
                    customSpokenSyllableCounts[bIdx] = (int)bNode.getProperty("spokenSyllables", 0);
                }

                auto& vec = barData[bIdx];
                for (int j = 0; j < bNode.getNumChildren(); ++j)
                {
                    auto sNode = bNode.getChild(j);
                    int sIdx = sNode.getProperty("index", -1);
                    juce::String text = sNode.getProperty("text", "");
                    if (sIdx >= 0)
                    {
                        if ((int)vec.size() <= sIdx)
                            vec.resize(sIdx + 1);
                        vec[sIdx] = text;
                        if ((int)sNode.getProperty("bold", 0) != 0)
                            boldCells.insert({ bIdx, sIdx });
                        if (sNode.hasProperty("align"))
                            cellAlignments[{ bIdx, sIdx }] = (CellAlignment)(int)sNode.getProperty("align", 0);
                        if (sNode.hasProperty("customColor"))
                            customCellColors[{ bIdx, sIdx }] = juce::Colour::fromString(sNode.getProperty("customColor").toString());
                    }
                }
            }
        }
    }

    refreshRhymes();
    notifyNotationChanged();
    notifyChanged();
}

void LyricDocument::notifyChanged()
{
    listeners.call([](Listener& l) { l.lyricDocumentChanged(); });
}

void LyricDocument::notifyNotationChanged()
{
    listeners.call([this](Listener& l) { l.metricNotationChanged(defaultNotation); });
}

void LyricDocument::notifySelectionChanged()
{
    listeners.call([](Listener& l) { l.selectionChanged(); });
}

void LyricDocument::setSelectedCells(const std::set<std::pair<int, int>>& selection)
{
    if (selectedCells != selection)
    {
        selectedCells = selection;
        notifySelectionChanged();
    }
}

void LyricDocument::selectCell(int barIndex, int globalSyllableIndex, bool addToSelection)
{
    if (addToSelection)
    {
        selectedCells.insert({ barIndex, globalSyllableIndex });
    }
    else
    {
        selectedCells.clear();
        selectedCells.insert({ barIndex, globalSyllableIndex });
    }
    notifySelectionChanged();
}

void LyricDocument::selectRange(std::pair<int, int> start, std::pair<int, int> end)
{
    selectedCells.clear();
    if (start > end)
        std::swap(start, end);

    for (int b = start.first; b <= end.first; ++b)
    {
        int totalSyls = getNotation(b).getTotalSyllables();
        if (totalSyls <= 0) continue;

        int sStart = (b == start.first) ? start.second : 0;
        int sEnd = (b == end.first) ? end.second : (totalSyls - 1);
        for (int s = sStart; s <= sEnd && s < totalSyls; ++s)
        {
            selectedCells.insert({ b, s });
        }
    }
    notifySelectionChanged();
}

void LyricDocument::clearSelection()
{
    if (!selectedCells.empty())
    {
        selectedCells.clear();
        notifySelectionChanged();
    }
}

bool LyricDocument::isCellSelected(int barIndex, int globalSyllableIndex) const
{
    return selectedCells.find({ barIndex, globalSyllableIndex }) != selectedCells.end();
}

void LyricDocument::deleteSelected()
{
    if (selectedCells.empty())
        return;

    std::vector<std::pair<int, int>> cells(selectedCells.begin(), selectedCells.end());
    clearCells(cells);
}

void LyricDocument::joinSelected()
{
    if (selectedCells.size() >= 2)
    {
        std::vector<std::pair<int, int>> cells(selectedCells.begin(), selectedCells.end());
        joinCells(cells);
    }
    else if (selectedCells.size() == 1)
    {
        auto it = selectedCells.begin();
        joinWithNext(it->first, it->second);
    }
}

void LyricDocument::splitSelected()
{
    if (selectedCells.empty())
        return;

    std::vector<std::pair<int, int>> cells(selectedCells.begin(), selectedCells.end());
    for (int i = (int)cells.size() - 1; i >= 0; --i)
    {
        splitCell(cells[i].first, cells[i].second);
    }
}

juce::String LyricDocument::getSelectedText() const
{
    if (selectedCells.empty())
        return {};

    // Group selected cells by bar index in order
    std::map<int, std::vector<int>> barToSyls;
    for (const auto& cell : selectedCells)
    {
        barToSyls[cell.first].push_back(cell.second);
    }

    juce::String result;
    bool firstBar = true;

    for (auto& pair : barToSyls)
    {
        std::sort(pair.second.begin(), pair.second.end());

        juce::String lineText;
        for (int sylIdx : pair.second)
        {
            juce::String s = getSyllable(pair.first, sylIdx).trim();
            if (s.isEmpty())
                continue;

            if (lineText.isEmpty())
            {
                lineText = s;
            }
            else
            {
                if (lineText.endsWith("-"))
                    lineText += s;
                else
                    lineText += " " + s;
            }
        }

        if (lineText.isNotEmpty())
        {
            if (!firstBar)
                result += "\n";
            result += lineText;
            firstBar = false;
        }
    }

    return result;
}

juce::String LyricDocument::getAllText() const
{
    juce::String result;
    bool firstBar = true;

    for (int b = 0; b < totalBars; ++b)
    {
        auto syls = getBarSyllables(b);
        juce::String lineText;
        for (const auto& s : syls)
        {
            juce::String trimmed = s.trim();
            if (trimmed.isEmpty())
                continue;

            if (lineText.isEmpty())
            {
                lineText = trimmed;
            }
            else
            {
                if (lineText.endsWith("-"))
                    lineText += trimmed;
                else
                    lineText += " " + trimmed;
            }
        }

        if (lineText.isNotEmpty())
        {
            if (!firstBar)
                result += "\n";
            result += lineText;
            firstBar = false;
        }
    }

    return result;
}

int LyricDocument::getLastBarWithContent() const
{
    int maxBar = -1;
    for (const auto& pair : barData)
    {
        for (const auto& s : pair.second)
        {
            if (s.trim().isNotEmpty())
            {
                if (pair.first > maxBar)
                    maxBar = pair.first;
                break;
            }
        }
    }
    return maxBar;
}

juce::String LyricDocument::exportFormattedText(bool includeSyllableCounts, bool includeLineSkips) const
{
    int lastBar = getLastBarWithContent();
    if (lastBar < 0)
        return {};

    juce::String out;
    for (int b = 0; b <= lastBar; ++b)
    {
        auto syls = getBarSyllables(b);
        juce::String lineText;
        for (const auto& s : syls)
        {
            juce::String trimmed = s.trim();
            if (trimmed.isEmpty())
                continue;

            if (lineText.isEmpty())
                lineText = trimmed;
            else if (lineText.endsWith("-"))
                lineText += trimmed;
            else
                lineText += " " + trimmed;
        }

        if (includeSyllableCounts && lineText.isNotEmpty())
        {
            int count = getActualSpokenSyllableCount(b);
            lineText += " [" + juce::String(count) + "]";
        }

        out += lineText + "\n";

        if (includeLineSkips && shouldAddSpacingAfterBar(b))
        {
            out += "\n";
        }
    }

    return out;
}

juce::String LyricDocument::exportCsvSpreadsheet() const
{
    int lastBar = getLastBarWithContent();
    if (lastBar < 0)
        return {};

    juce::String out = "Bar,Metric Schema,Spoken Syllables,Grid Syllables,Lyrics\n";
    for (int b = 0; b <= lastBar; ++b)
    {
        auto syls = getBarSyllables(b);
        juce::String lineText;
        for (const auto& s : syls)
        {
            juce::String trimmed = s.trim();
            if (trimmed.isEmpty())
                continue;

            if (lineText.isEmpty())
                lineText = trimmed;
            else if (lineText.endsWith("-"))
                lineText += trimmed;
            else
                lineText += " " + trimmed;
        }

        int spoken = getActualSpokenSyllableCount(b);
        int grid = getNotation(b).getTotalSyllables();
        juce::String schema = getNotation(b).toNotationString();

        juce::String escapedLyrics = lineText.replace("\"", "\"\"");

        out += juce::String(b + 1) + ","
            + "\"" + schema + "\","
            + juce::String(spoken) + ","
            + juce::String(grid) + ","
            + "\"" + escapedLyrics + "\"\n";

        if (shouldAddSpacingAfterBar(b))
        {
            out += ",,,,\n";
        }
    }

    return out;
}

juce::String LyricDocument::exportHtmlTable(const juce::String& title) const
{
    int lastBar = getLastBarWithContent();
    if (lastBar < 0)
        return {};

    juce::String html;
    html += "<!DOCTYPE html>\n<html>\n<head>\n<meta charset=\"utf-8\">\n";
    html += "<title>" + title + "</title>\n";
    html += "<style>\n";
    html += "  body { font-family: 'Segoe UI', system-ui, -apple-system, sans-serif; background-color: #FAF8F2; color: #262626; margin: 40px auto; max-width: 900px; padding: 0 20px; }\n";
    html += "  h1 { font-size: 24px; border-bottom: 2px solid #EF5350; padding-bottom: 8px; margin-bottom: 24px; color: #1E293B; }\n";
    html += "  table { width: 100%; border-collapse: collapse; background: #FFFFFF; box-shadow: 0 1px 3px rgba(0,0,0,0.08); border-radius: 6px; overflow: hidden; }\n";
    html += "  th { background: #F1EFE9; color: #475569; font-size: 13px; text-transform: uppercase; letter-spacing: 0.5px; padding: 10px 14px; text-align: left; border-bottom: 2px solid #E2E8F0; }\n";
    html += "  td { padding: 9px 14px; font-size: 14px; border-bottom: 1px solid #E2E8F0; vertical-align: middle; }\n";
    html += "  tr.stanza-break td { border-bottom: 3px double #94A3B8; background-color: #FAF9F5; }\n";
    html += "  .bar-num { color: #94A3B8; font-family: Consolas, monospace; font-size: 12px; width: 45px; text-align: center; }\n";
    html += "  .schema { color: #64748B; font-family: Consolas, monospace; font-size: 12px; width: 120px; }\n";
    html += "  .count { width: 60px; text-align: center; font-weight: bold; font-family: Consolas, monospace; color: #B45309; background: #FEF3C7; border-radius: 4px; padding: 2px 6px; }\n";
    html += "  .lyrics { font-weight: 500; font-size: 15px; }\n";
    html += "  @media print { body { background: white; margin: 0; padding: 0; } table { box-shadow: none; } }\n";
    html += "</style>\n</head>\n<body>\n";
    html += "<h1>" + title + "</h1>\n";
    html += "<table>\n";
    html += "<thead><tr><th>Bar</th><th>Metric Schema</th><th>Lyrics</th><th>Syllables</th></tr></thead>\n";
    html += "<tbody>\n";

    for (int b = 0; b <= lastBar; ++b)
    {
        auto syls = getBarSyllables(b);
        juce::String lineText;
        for (const auto& s : syls)
        {
            juce::String trimmed = s.trim();
            if (trimmed.isEmpty())
                continue;

            if (lineText.isEmpty())
                lineText = trimmed;
            else if (lineText.endsWith("-"))
                lineText += trimmed;
            else
                lineText += " " + trimmed;
        }

        int spoken = getActualSpokenSyllableCount(b);
        juce::String schema = getNotation(b).toNotationString();
        bool isStanza = shouldAddSpacingAfterBar(b);

        html += "  <tr" + juce::String(isStanza ? " class=\"stanza-break\"" : "") + ">\n";
        html += "    <td class=\"bar-num\">" + juce::String::formatted("%02d", b + 1) + "</td>\n";
        html += "    <td class=\"schema\">" + schema + "</td>\n";
        html += "    <td class=\"lyrics\">" + lineText + "</td>\n";
        html += "    <td><span class=\"count\">" + juce::String(spoken) + "</span></td>\n";
        html += "  </tr>\n";
    }

    html += "</tbody>\n</table>\n</body>\n</html>\n";
    return html;
}

} // namespace CompassCadence
