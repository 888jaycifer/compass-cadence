#pragma once

#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include "MetricNotation.h"
#include "RhymeClassifier.h"
#include "SyllableSplitter.h"
#include <vector>
#include <map>
#include <set>

namespace CompassCadence
{

class LyricDocument
{
public:
    enum BarSpacing
    {
        SpacingOff = 0,
        Spacing4 = 1,
        Spacing8 = 2,
        Spacing16 = 3
    };

    enum ViewMode
    {
        ModeScroll = 0,
        ModePages = 1
    };

    enum CellAlignment
    {
        AlignCenter = 0,
        AlignLeft = 1,
        AlignRight = 2
    };

    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void lyricDocumentChanged() = 0;
        virtual void metricNotationChanged(const MetricNotation& newNotation) = 0;
        virtual void barNotationChanged(int barIndex, const MetricNotation& newNotation) {}
        virtual void selectionChanged() {}
    };

    LyricDocument();

    void addListener(Listener* l) { listeners.add(l); }
    void removeListener(Listener* l) { listeners.remove(l); }

    // Metric notation (Master & Per-Bar)
    const MetricNotation& getNotation(int barIndex = -1) const noexcept;
    const MetricNotation& getNotationForBar(int barIndex) const noexcept { return getNotation(barIndex); }
    void setNotation(const MetricNotation& newNotation);
    void setBarNotation(int barIndex, const MetricNotation& newNotation);
    void clearBarNotation(int barIndex);
    bool hasBarNotationOverride(int barIndex) const noexcept;

    // Bars & Syllables
    int getBarCount() const noexcept { return totalBars; }
    void ensureBarCount(int count);

    juce::String getSyllable(int barIndex, int globalSyllableIndex) const;
    void setSyllable(int barIndex, int globalSyllableIndex, const juce::String& text, bool notify = true);

    // Returns all syllables for a given bar
    std::vector<juce::String> getBarSyllables(int barIndex) const;
    void setBarSyllables(int barIndex, const std::vector<juce::String>& syllables);

    // Dynamic typing / scratchpad insertion: splits text into syllables and places across consecutive cells
    // Returns the next {barIndex, globalSyllableIndex} target for cursor focus
    std::pair<int, int> insertTextFlow(int barIndex, int startSyllableIndex, const juce::String& text);

    // Clear bar lyrics
    void clearBar(int barIndex);
    void clearAll();

    // Undo / Redo
    void pushUndoSnapshot();
    void undo();
    void redo();
    bool canUndo() const noexcept { return !undoStack.empty(); }
    bool canRedo() const noexcept { return !redoStack.empty(); }

    // Multi-cell batch operations & selection
    const std::set<std::pair<int, int>>& getSelectedCells() const noexcept { return selectedCells; }
    void setSelectedCells(const std::set<std::pair<int, int>>& selection);
    void selectCell(int barIndex, int globalSyllableIndex, bool addToSelection = false);
    void selectRange(std::pair<int, int> start, std::pair<int, int> end);
    void clearSelection();
    bool isCellSelected(int barIndex, int globalSyllableIndex) const;
    void deleteSelected();
    void joinSelected();
    void splitSelected();
    juce::String getSelectedText() const;
    juce::String getAllText() const;

    // Lyrics Export
    int getLastBarWithContent() const;
    juce::String exportFormattedText(bool includeSyllableCounts = true, bool includeLineSkips = true) const;
    juce::String exportCsvSpreadsheet() const;
    juce::String exportHtmlTable(const juce::String& title = "Compass Cadence Lyrics") const;

    void clearCells(const std::vector<std::pair<int, int>>& cells);
    void joinCells(const std::vector<std::pair<int, int>>& cells);
    void joinWithNext(int barIndex, int globalSyllableIndex);
    void splitCell(int barIndex, int globalSyllableIndex);

    // Dynamic syllable insertion and meter alteration via direct interaction
    void insertSyllableInBar(int barIndex, int globalSyllableIndex, bool insertAfter = false);
    void deleteSyllableInBar(int barIndex, int globalSyllableIndex);

    // Bold emphasis notation
    bool isCellBold(int barIndex, int globalSyllableIndex) const;
    void setCellBold(int barIndex, int globalSyllableIndex, bool bold, bool notify = true);
    void toggleCellBold(int barIndex, int globalSyllableIndex);
    void toggleSelectedBold();

    // Cell text alignment
    CellAlignment getCellAlignment(int barIndex, int globalSyllableIndex) const;
    void setCellAlignment(int barIndex, int globalSyllableIndex, CellAlignment align, bool notify = true);
    void setSelectionAlignment(CellAlignment align);
    bool getShowAlignmentControls() const noexcept { return showAlignmentControls; }
    void setShowAlignmentControls(bool show, bool notify = true);

    // Custom cell highlight colors (overrides phoneme rhyme tint)
    bool hasCustomCellColor(int barIndex, int globalSyllableIndex) const;
    juce::Colour getCustomCellColor(int barIndex, int globalSyllableIndex) const;
    void setCustomCellColor(int barIndex, int globalSyllableIndex, const juce::Colour& color, bool notify = true);
    void clearCustomCellColor(int barIndex, int globalSyllableIndex, bool notify = true);
    void setSelectionCustomColor(const juce::Colour& color);
    void clearSelectionCustomColor();
    void clearAllCustomCellColors(bool notify = true);

    // Text casing transforms
    enum CaseTransform
    {
        CaseUpper = 0,
        CaseLower = 1,
        CaseTitle = 2
    };
    void transformCellCase(int barIndex, int globalSyllableIndex, CaseTransform mode);
    void transformSelectionCase(CaseTransform mode);

    // Flow duplication
    void duplicateCellToNext(int barIndex, int globalSyllableIndex);

    // Spoken syllable count (Calculated vs. Custom)
    int getCalculatedSyllableCount(int barIndex) const;
    int getActualSpokenSyllableCount(int barIndex) const;
    void setActualSpokenSyllableCount(int barIndex, int count);
    void resetActualSpokenSyllableCount(int barIndex);
    bool hasCustomSpokenSyllableCount(int barIndex) const;

    // View mode (Scroll vs Pages)
    ViewMode getViewMode() const noexcept { return viewMode; }
    void setViewMode(ViewMode mode);
    int getVisibleStartBar() const;
    int getVisibleEndBar() const;

    // Theme Mode
    bool isDarkMode() const noexcept { return darkMode; }
    void setDarkMode(bool dark);

    // Row / Bar Height (Default 50, minimum 32)
    int getBarHeight() const noexcept { return barHeight; }
    void setBarHeight(int h);

    // Page management
    int getCurrentPage() const noexcept { return currentPage; }
    void setCurrentPage(int page);
    int getTotalPages() const;
    void addPage();
    int getBarsPerPage() const noexcept { return barsPerPage; }
    void setBarsPerPage(int bpp) { barsPerPage = std::max(4, bpp); }

    int getFirstBarOfCurrentPage() const { return currentPage * barsPerPage; }
    int getLastBarOfCurrentPage() const { return std::min(totalBars, (currentPage + 1) * barsPerPage); }

    // Line spacing between bar groups & custom stanza breaks
    BarSpacing getBarSpacing() const noexcept { return barSpacing; }
    void setBarSpacing(BarSpacing spacing);
    bool shouldAddSpacingAfterBar(int barIndex) const;
    void toggleStanzaBreak(int barIndex);
    void setStanzaBreak(int barIndex, bool enabled);
    bool hasStanzaBreak(int barIndex) const { return shouldAddSpacingAfterBar(barIndex); }

    // Dynamic bar insertion (e.g. from line hover "+" menu)
    void insertBar(int afterBarIndex);

    // Rhyme classification
    RhymeClassifier& getRhymeClassifier() noexcept { return rhymeClassifier; }
    const RhymeClassifier& getRhymeClassifier() const noexcept { return rhymeClassifier; }
    RhymeClassifier::ColorMode getColorMode() const noexcept { return rhymeClassifier.getColorMode(); }
    void setColorMode(RhymeClassifier::ColorMode mode) { rhymeClassifier.setColorMode(mode); refreshRhymes(); notifyChanged(); }
    void cycleColorMode() { rhymeClassifier.cycleColorMode(); refreshRhymes(); notifyChanged(); }
    void refreshRhymes();
    juce::Colour getVowelSoundColor(const juce::String& vowelKey) const;
    void setVowelSoundColor(const juce::String& vowelKey, const juce::Colour& colour);
    void resetVowelSoundColorsToDefaults();

    // Repeats settings & sequence suppression
    int getMinRepeatLength() const noexcept { return rhymeClassifier.getMinRepeatLength(); }
    void setMinRepeatLength(int len);
    bool isSequenceHiddenAt(int barIndex, int globalSylIndex) const;
    void toggleHideSequenceAt(int barIndex, int globalSylIndex);
    void unhideAllSequences();
    bool hasHiddenSequences() const noexcept { return !hiddenColorCells.empty(); }
    const std::set<std::pair<int, int>>& getHiddenColorCells() const noexcept { return hiddenColorCells; }

    // ValueTree Serialization for DAW project saving
    juce::ValueTree toValueTree() const;
    void fromValueTree(const juce::ValueTree& vt);

    void notifyChanged();

private:
    void notifyNotationChanged();
    void notifyBarNotationChanged(int barIndex);
    void notifySelectionChanged();

    struct DocumentSnapshot
    {
        std::map<int, std::vector<juce::String>> barData;
        MetricNotation defaultNotation;
        std::map<int, MetricNotation> barNotations;
        std::set<std::pair<int, int>> boldCells;
        std::map<std::pair<int, int>, CellAlignment> cellAlignments;
        std::map<std::pair<int, int>, juce::Colour> customCellColors;
        std::set<std::pair<int, int>> hiddenColorCells;
        int minRepeatLength = 2;
        std::map<int, int> customSpokenSyllableCounts;
        int totalBars = 320;
        int barHeight = 50;
        ViewMode viewMode = ModeScroll;
        bool darkMode = false;
        bool customStanzaBreaksActive = false;
        std::set<int> stanzaBreaks;
        bool showAlignmentControls = false;
    };

    MetricNotation defaultNotation;
    std::map<int, MetricNotation> barNotations;
    RhymeClassifier rhymeClassifier;

    int totalBars = 320;
    int barsPerPage = 16;
    int currentPage = 0;
    int barHeight = 50;

    BarSpacing barSpacing = Spacing4;
    ViewMode viewMode = ModeScroll;
    bool darkMode = false;
    bool showAlignmentControls = false;

    // Ordered map of bar index to array of syllable strings (guarantees deterministic iteration)
    std::map<int, std::vector<juce::String>> barData;

    std::set<std::pair<int, int>> selectedCells;
    std::set<std::pair<int, int>> boldCells;
    std::map<std::pair<int, int>, CellAlignment> cellAlignments;
    std::map<std::pair<int, int>, juce::Colour> customCellColors;
    std::set<std::pair<int, int>> hiddenColorCells;
    std::map<int, int> customSpokenSyllableCounts;
    bool customStanzaBreaksActive = false;
    std::set<int> stanzaBreaks;

    std::vector<DocumentSnapshot> undoStack;
    std::vector<DocumentSnapshot> redoStack;

    juce::ListenerList<Listener> listeners;
};

} // namespace CompassCadence
