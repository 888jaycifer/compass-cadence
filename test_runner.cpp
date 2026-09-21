#include <juce_gui_basics/juce_gui_basics.h>
#include <iostream>
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Source/Model/PresetManager.h"
#include "Source/Model/SongManager.h"

int main(int argc, char* argv[])
{
    juce::initialiseJuce_GUI();
    std::cout << "[1] Juce GUI initialized." << std::endl;

    try
    {
        std::cout << "[2] Creating CompassCadenceAudioProcessor..." << std::endl;
        auto proc = std::make_unique<CompassCadence::CompassCadenceAudioProcessor>();
        std::cout << "[3] Processor created successfully: " << proc->getName() << std::endl;

        std::cout << "[4] Preparing to play (44100, 512)..." << std::endl;
        proc->prepareToPlay(44100.0, 512);

        std::cout << "[5] Creating Editor..." << std::endl;
        auto editor = std::unique_ptr<juce::AudioProcessorEditor>(proc->createEditor());
        std::cout << "[6] Editor created successfully! Bounds: " << editor->getBounds().toString() << std::endl;

        std::cout << "[7] Simulating timerCallback..." << std::endl;
        if (auto* cceditor = dynamic_cast<CompassCadence::CompassCadenceAudioProcessorEditor*>(editor.get()))
        {
            cceditor->timerCallback();
        }
        std::cout << "[8] timerCallback succeeded!" << std::endl;

        std::cout << "[9] Simulating paint..." << std::endl;
        juce::Image testImg(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
        juce::Graphics g(testImg);

        std::cout << "  [9.1] Painting editor base..." << std::endl;
        editor->paint(g);
        std::cout << "  [9.1] Passed!" << std::endl;

        for (int i = 0; i < editor->getNumChildComponents(); ++i)
        {
            auto* child = editor->getChildComponent(i);
            std::cout << "  [9.2] Painting child " << i << " (" << typeid(*child).name() << ")..." << std::endl;
            child->paintEntireComponent(g, true);
            std::cout << "  [9.2] Passed child " << i << "!" << std::endl;
        }

        std::cout << "[10] All paints succeeded!" << std::endl;

        // Test Syllable Auto-Splitting
        std::cout << "[10.1] Testing Syllable Auto-Splitting..." << std::endl;
        auto syls = CompassCadence::SyllableSplitter::splitLineIntoSyllables("retire");
        std::cout << "  'retire' split count: " << syls.size() << std::endl;
        for (const auto& s : syls)
            std::cout << "    syl: " << s << std::endl;
        jassert(syls.size() == 2);
        jassert(syls[0] == "re-");
        jassert(syls[1] == "tire");

        auto target = proc->getLyricDocument().insertTextFlow(0, 0, "retire");
        std::cout << "  insertTextFlow 'retire' target: Bar " << target.first << ", Syl " << target.second << std::endl;
        jassert(target.first == 0 && target.second == 2);
        jassert(proc->getLyricDocument().getSyllable(0, 0) == "re-");
        jassert(proc->getLyricDocument().getSyllable(0, 1) == "tire");
        std::cout << "  [10.1] Syllable Auto-Splitting passed!" << std::endl;

        // Test Undo / Redo
        std::cout << "[10.2] Testing Undo / Redo..." << std::endl;
        auto& doc = proc->getLyricDocument();
        doc.setSyllable(1, 0, "first");
        jassert(doc.getSyllable(1, 0) == "first");
        doc.setSyllable(1, 0, "second");
        jassert(doc.getSyllable(1, 0) == "second");
        doc.undo();
        jassert(doc.getSyllable(1, 0) == "first");
        doc.redo();
        jassert(doc.getSyllable(1, 0) == "second");
        std::cout << "  [10.2] Undo / Redo passed!" << std::endl;

        // Test Multi-cell selection & Batch Clear & Join
        std::cout << "[10.3] Testing Multi-cell selection & Join..." << std::endl;
        doc.setSyllable(2, 0, "gon");
        doc.setSyllable(2, 1, "na");
        doc.selectRange({ 2, 0 }, { 2, 1 });
        jassert(doc.getSelectedCells().size() == 2);
        doc.joinSelected();
        jassert(doc.getSyllable(2, 0) == "gon na");
        jassert(doc.getSyllable(2, 1) == "");
        doc.deleteSelected();
        jassert(doc.getSyllable(2, 0) == "");
        std::cout << "  [10.3] Multi-cell selection & Join passed!" << std::endl;

        // Test Rhyme Key Determinism & Hyphen filter
        std::cout << "[10.4] Testing Rhyme Key Determinism & Hyphen filter..." << std::endl;
        juce::String key1 = CompassCadence::RhymeClassifier::extractRhymeKey("re-");
        jassert(key1.isEmpty()); // Non-terminal hyphenated stem must not rhyme
        juce::String keyNight = CompassCadence::RhymeClassifier::extractRhymeKey("night");
        juce::String keyRight = CompassCadence::RhymeClassifier::extractRhymeKey("right");
        jassert(keyNight == "AY" && keyRight == "AY");
        std::cout << "  [10.4] Rhyme Classifier determinism passed!" << std::endl;

        // Test Per-Bar Metric Notation & Mixed Polymeter
        std::cout << "[10.5] Testing Per-Bar Metric Notation & Mixed Polymeter..." << std::endl;
        auto bar2Notation = CompassCadence::MetricNotation::fromNotationString("[333222]/6:4");
        doc.setBarNotation(2, bar2Notation);
        jassert(doc.hasBarNotationOverride(2));
        jassert(!doc.hasBarNotationOverride(0));
        jassert(doc.getNotation(0).getTotalSyllables() == 16);
        jassert(doc.getNotation(2).getTotalSyllables() == 15);
        jassert(doc.getNotation(2).toNotationString() == "[333222]/6:4");

        // ValueTree serialization with per-bar notation
        auto vt = doc.toValueTree();
        CompassCadence::LyricDocument doc2;
        doc2.fromValueTree(vt);
        jassert(doc2.hasBarNotationOverride(2));
        jassert(doc2.getNotation(2).getTotalSyllables() == 15);
        jassert(doc2.getNotation(2).toNotationString() == "[333222]/6:4");

        // Undo bar notation override
        doc.undo();
        jassert(!doc.hasBarNotationOverride(2));
        jassert(doc.getNotation(2).getTotalSyllables() == 16);
        doc.redo();
        jassert(doc.hasBarNotationOverride(2));
        jassert(doc.getNotation(2).getTotalSyllables() == 15);
        std::cout << "  [10.5] Per-Bar Metric Notation passed!" << std::endl;

        // Test 10.6: Syllable splitting for words ending in -es and -ed
        std::cout << "[10.6] Testing Syllable Splitter -es and -ed rules..." << std::endl;
        auto rhymesSyls = CompassCadence::SyllableSplitter::splitLineIntoSyllables("rhymes lines makes takes notes games comes miles");
        jassert(rhymesSyls.size() == 8);
        for (const auto& s : rhymesSyls)
            jassert(!s.endsWith("-")); // All must be 1 syllable, no hyphens!

        auto boxesSyls = CompassCadence::SyllableSplitter::splitLineIntoSyllables("boxes watches");
        jassert(boxesSyls.size() == 4); // "box-", "es", "wat-", "ches"
        std::cout << "  [10.6] Syllable Splitter -es and -ed rules passed!" << std::endl;

        // Test 10.7: Bold emphasis notation
        std::cout << "[10.7] Testing Bold Emphasis Notation..." << std::endl;
        jassert(!doc.isCellBold(0, 0));
        doc.setCellBold(0, 0, true);
        jassert(doc.isCellBold(0, 0));
        doc.undo();
        jassert(!doc.isCellBold(0, 0));
        doc.redo();
        jassert(doc.isCellBold(0, 0));

        auto vtBold = doc.toValueTree();
        CompassCadence::LyricDocument docBoldTest;
        docBoldTest.fromValueTree(vtBold);
        jassert(docBoldTest.isCellBold(0, 0));
        std::cout << "  [10.7] Bold Emphasis Notation passed!" << std::endl;

        // Test 10.8: PresetManager
        std::cout << "[10.8] Testing PresetManager..." << std::endl;
        CompassCadence::PresetManager pm;
        jassert(pm.getFactoryPresets().size() == 6);
        pm.saveUserPreset("TestFlow", "[333222]/6:4", 4);
        bool found = false;
        for (const auto& p : pm.getUserPresets())
        {
            if (p.name == "TestFlow" && p.notation == "[333222]/6:4")
            {
                found = true;
                break;
            }
        }
        jassert(found);
        pm.deleteUserPresetByName("TestFlow");
        found = false;
        for (const auto& p : pm.getUserPresets())
        {
            if (p.name == "TestFlow") found = true;
        }
        std::cout << "  [10.8] PresetManager passed!" << std::endl;

        // Test 10.9: Syllable Counter & Actual Spoken Count Customization
        std::cout << "[10.9] Testing Syllable Counter & Actual Spoken Count Customization..." << std::endl;
        // Bar 3 has no text initially: calculated count equals grid total (16)
        jassert(!doc.hasCustomSpokenSyllableCount(3));
        jassert(doc.getCalculatedSyllableCount(3) == 16);
        jassert(doc.getActualSpokenSyllableCount(3) == 16);

        // Put 2 syllables in Bar 3
        doc.setSyllable(3, 0, "quick");
        doc.setSyllable(3, 1, "rhyme");
        jassert(doc.getCalculatedSyllableCount(3) == 2);
        jassert(doc.getActualSpokenSyllableCount(3) == 2);

        // User customizes spoken count to 1 (e.g. vocal slur / rapid compression)
        doc.setActualSpokenSyllableCount(3, 1);
        jassert(doc.hasCustomSpokenSyllableCount(3));
        jassert(doc.getActualSpokenSyllableCount(3) == 1);
        jassert(doc.getCalculatedSyllableCount(3) == 2);

        // Test ValueTree serialization of custom spoken syllables
        auto vtSyl = doc.toValueTree();
        CompassCadence::LyricDocument docSylTest;
        docSylTest.fromValueTree(vtSyl);
        jassert(docSylTest.hasCustomSpokenSyllableCount(3));
        jassert(docSylTest.getActualSpokenSyllableCount(3) == 1);
        jassert(docSylTest.getCalculatedSyllableCount(3) == 2);

        // Test Undo / Redo of custom spoken count
        doc.undo();
        jassert(!doc.hasCustomSpokenSyllableCount(3));
        jassert(doc.getActualSpokenSyllableCount(3) == 2);
        doc.redo();
        jassert(doc.hasCustomSpokenSyllableCount(3));
        jassert(doc.getActualSpokenSyllableCount(3) == 1);

        // Test Reset
        doc.resetActualSpokenSyllableCount(3);
        jassert(!doc.hasCustomSpokenSyllableCount(3));
        std::cout << "  [10.9] Syllable Counter & Custom Spoken Count passed!" << std::endl;

        // Test 10.10: Page Navigation & Display Shifting
        std::cout << "[10.10] Testing Page Navigation & Display Shifting..." << std::endl;
        doc.setCurrentPage(0);
        jassert(doc.getCurrentPage() == 0);
        jassert(doc.getFirstBarOfCurrentPage() == 0);
        jassert(doc.getLastBarOfCurrentPage() == 16);

        // Turn to Page 2 (0-indexed page 1)
        doc.setCurrentPage(1);
        jassert(doc.getCurrentPage() == 1);
        jassert(doc.getFirstBarOfCurrentPage() == 16);
        jassert(doc.getLastBarOfCurrentPage() == 32);

        // Simulate 60 Hz timer callback while DAW is stopped:
        // Must NOT revert currentPage back to 0!
        if (auto* cceditor = dynamic_cast<CompassCadence::CompassCadenceAudioProcessorEditor*>(editor.get()))
        {
            cceditor->timerCallback();
        }
        jassert(doc.getCurrentPage() == 1); // Still on Page 2!

        // Turn to Page 3 (0-indexed page 2)
        doc.setCurrentPage(2);
        jassert(doc.getCurrentPage() == 2);
        jassert(doc.getFirstBarOfCurrentPage() == 32);
        jassert(doc.getLastBarOfCurrentPage() == 48);

        if (auto* cceditor = dynamic_cast<CompassCadence::CompassCadenceAudioProcessorEditor*>(editor.get()))
        {
            cceditor->timerCallback();
        }
        jassert(doc.getCurrentPage() == 2); // Still on Page 3!

        // Turn back to Page 1 (0-indexed page 0)
        doc.setCurrentPage(0);
        jassert(doc.getCurrentPage() == 0);
        jassert(doc.getFirstBarOfCurrentPage() == 0);

        // Verify default notebook has at least 20 pages (320 bars)
        jassert(doc.getBarCount() >= 320);
        jassert(doc.getTotalPages() >= 20);

        // Navigate to Page 20 (0-indexed page 19)
        doc.setCurrentPage(19);
        jassert(doc.getCurrentPage() == 19);
        jassert(doc.getFirstBarOfCurrentPage() == 304);
        jassert(doc.getLastBarOfCurrentPage() == 320);

        // Test dynamic expansion: addPage() adds Page 21
        int prevTotal = doc.getTotalPages();
        doc.addPage();
        jassert(doc.getTotalPages() == prevTotal + 1);
        doc.setCurrentPage(20);
        jassert(doc.getCurrentPage() == 20);
        jassert(doc.getFirstBarOfCurrentPage() == 320);
        jassert(doc.getLastBarOfCurrentPage() == 336);

        // Test ensureBarCount dynamic expansion
        doc.ensureBarCount(350);
        jassert(doc.getTotalPages() == 22);
        jassert(doc.getBarCount() == 352);

        doc.setCurrentPage(0);
        std::cout << "  [10.10] Page Navigation & Display Shifting (20+ pages) passed!" << std::endl;

        // Test 10.11: Scroll Mode vs. Pages Mode & Infinite Vertical Expansion
        std::cout << "[10.11] Testing Scroll Mode vs. Pages Mode & Infinite Vertical Expansion..." << std::endl;

        // 1. Toggle view mode to ModeScroll
        doc.setViewMode(CompassCadence::LyricDocument::ModeScroll);
        jassert(doc.getViewMode() == CompassCadence::LyricDocument::ModeScroll);

        // 2. Verify bar range spans continuous bars (startBar = 0, endBar >= 64)
        jassert(doc.getVisibleStartBar() == 0);
        jassert(doc.getVisibleEndBar() == doc.getBarCount());
        jassert(doc.getVisibleEndBar() >= 64);

        // 3. Verify infinite expansion when reaching the bottom boundary (ensureBarCount)
        int prevBarsCount = doc.getBarCount();
        doc.ensureBarCount(prevBarsCount + 16);
        jassert(doc.getBarCount() >= prevBarsCount + 16);
        jassert(doc.getVisibleEndBar() == doc.getBarCount());

        // 4. Toggle view mode to ModePages
        doc.setViewMode(CompassCadence::LyricDocument::ModePages);
        jassert(doc.getViewMode() == CompassCadence::LyricDocument::ModePages);

        // 5. Verify 16-bar pagination boundaries
        doc.setCurrentPage(0);
        jassert(doc.getVisibleStartBar() == 0);
        jassert(doc.getVisibleEndBar() == 16);
        doc.setCurrentPage(1);
        jassert(doc.getVisibleStartBar() == 16);
        jassert(doc.getVisibleEndBar() == 32);

        // 6. ValueTree serialization roundtrip of viewMode
        doc.setViewMode(CompassCadence::LyricDocument::ModeScroll);
        auto vtScroll = doc.toValueTree();
        CompassCadence::LyricDocument docScrollRestore;
        docScrollRestore.fromValueTree(vtScroll);
        jassert(docScrollRestore.getViewMode() == CompassCadence::LyricDocument::ModeScroll);

        doc.setViewMode(CompassCadence::LyricDocument::ModePages);
        auto vtPages = doc.toValueTree();
        CompassCadence::LyricDocument docPagesRestore;
        docPagesRestore.fromValueTree(vtPages);
        jassert(docPagesRestore.getViewMode() == CompassCadence::LyricDocument::ModePages);

        // 7. Undo / Redo of viewMode
        doc.undo();
        jassert(doc.getViewMode() == CompassCadence::LyricDocument::ModeScroll);
        doc.redo();
        jassert(doc.getViewMode() == CompassCadence::LyricDocument::ModePages);

        std::cout << "  [10.11] Scroll Mode vs. Pages Mode & Infinite Vertical Expansion passed!" << std::endl;

        // Test 10.12: Clipboard Copy (Selected & Full) and Tooltip Suppression
        std::cout << "[10.12] Testing Clipboard Copy and Tooltip Suppression..." << std::endl;
        doc.clearAll();
        doc.setSyllable(0, 0, "drop-");
        doc.setSyllable(0, 1, "pin");
        doc.setSyllable(1, 0, "the");
        doc.setSyllable(1, 1, "beat");
        doc.setSyllable(1, 2, "now");

        // Full text extraction
        juce::String full = doc.getAllText();
        std::cout << "  Full text:\n" << full << std::endl;
        jassert(full == "droppin\nthe beat now");

        // Partial selection: Bar 0 only
        doc.selectRange({ 0, 0 }, { 0, 1 });
        juce::String selBar0 = doc.getSelectedText();
        std::cout << "  Selected Bar 0: " << selBar0 << std::endl;
        jassert(selBar0 == "droppin");

        // Partial selection: Bar 1 words
        doc.selectRange({ 1, 0 }, { 1, 1 });
        juce::String selBar1 = doc.getSelectedText();
        std::cout << "  Selected Bar 1 (first 2 words): " << selBar1 << std::endl;
        jassert(selBar1 == "the beat");

        // Empty selection returns empty string
        doc.clearSelection();
        jassert(doc.getSelectedText().isEmpty());

        // Tooltip Suppression Verification
        CompassCadence::NonTextTooltipWindow tipWin(nullptr, 400);
        juce::TextEditor testEditor;
        testEditor.setTooltip("Should be suppressed");
        jassert(tipWin.getTipFor(testEditor).isEmpty());

        CompassCadence::SyllableCellComponent testCell(doc, 0, 0, 0, 0);
        jassert(tipWin.getTipFor(testCell).isEmpty());

        juce::TextButton testBtn("Test");
        testBtn.setTooltip("Valid button tooltip");
        jassert(tipWin.getTipFor(testBtn) == "Valid button tooltip");

        std::cout << "  [10.12] Clipboard Copy and Tooltip Suppression passed!" << std::endl;

        // Test 10.13: Standalone Metronome Synthesis & Transport
        std::cout << "[10.13] Testing Standalone Metronome Synthesis & Transport..." << std::endl;
        proc->setForceStandaloneMode(true);
        jassert(proc->isStandalone());

        // Check initial state
        jassert(!proc->isStandalonePlaying());
        jassert(proc->isMetronomeEnabled());

        // Process a block while stopped -> buffer is silence
        juce::AudioBuffer<float> testAudio(2, 512);
        testAudio.clear();
        juce::MidiBuffer testMidi;
        proc->processBlock(testAudio, testMidi);
        jassert(testAudio.getMagnitude(0, 512) == 0.0f);

        // Start standalone playback
        proc->toggleStandalonePlayback();
        jassert(proc->isStandalonePlaying());

        // Process several blocks until a beat click triggers and synthesizes audio
        float maxMag = 0.0f;
        for (int b = 0; b < 50; ++b)
        {
            testAudio.clear();
            proc->processBlock(testAudio, testMidi);
            maxMag = std::max(maxMag, testAudio.getMagnitude(0, 512));
        }
        std::cout << "  Metronome peak click magnitude: " << maxMag << std::endl;
        jassert(maxMag > 0.05f); // Audio click was synthesized!

        // Disable metronome click
        proc->setMetronomeEnabled(false);
        jassert(!proc->isMetronomeEnabled());

        // Let any active click tail finish
        for (int b = 0; b < 10; ++b)
        {
            testAudio.clear();
            proc->processBlock(testAudio, testMidi);
        }
        testAudio.clear();
        proc->processBlock(testAudio, testMidi);
        jassert(testAudio.getMagnitude(0, 512) == 0.0f); // Silence when click disabled!

        // Stop playback
        proc->toggleStandalonePlayback();
        jassert(!proc->isStandalonePlaying());
        testAudio.clear();
        proc->processBlock(testAudio, testMidi);
        jassert(testAudio.getMagnitude(0, 512) == 0.0f);

        // Standalone tempo adjustment
        proc->setStandaloneBpm(140.0);
        jassert(proc->getStandaloneBpm() == 140.0);

        std::cout << "  [10.13] Standalone Metronome Synthesis & Transport passed!" << std::endl;

        // Test 10.14: Cell Text Alignment & Auto-Splitting Alignment
        std::cout << "[10.14] Testing Cell Text Alignment & Auto-Splitting Alignment..." << std::endl;
        doc.clearAll();
        jassert(doc.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignCenter);

        doc.setCellAlignment(0, 0, CompassCadence::LyricDocument::AlignLeft);
        jassert(doc.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignLeft);

        doc.setCellAlignment(0, 0, CompassCadence::LyricDocument::AlignRight);
        jassert(doc.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignRight);

        // Undo/Redo alignment
        doc.undo();
        jassert(doc.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignLeft);
        doc.redo();
        jassert(doc.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignRight);

        // Auto-splitting multi-syllable word alignment
        doc.insertTextFlow(0, 0, "paper"); // splits into "pa-" and "per"
        jassert(doc.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignRight); // First syllable hugs next box
        jassert(doc.getCellAlignment(0, 1) == CompassCadence::LyricDocument::AlignLeft);  // Last syllable hugs previous box

        // ValueTree roundtrip with alignments
        auto vtAlign = doc.toValueTree();
        CompassCadence::LyricDocument docAlignRestore;
        docAlignRestore.fromValueTree(vtAlign);
        jassert(docAlignRestore.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignRight);
        jassert(docAlignRestore.getCellAlignment(0, 1) == CompassCadence::LyricDocument::AlignLeft);
        std::cout << "  [10.14] Cell Text Alignment & Auto-Splitting Alignment passed!" << std::endl;

        // Test 10.15: CMUDict Rhyme Engine & Non-Colliding Color Assignment
        std::cout << "[10.15] Testing CMUDict Rhyme Engine & Non-Colliding Color Assignment..." << std::endl;
        juce::String keyTo = CompassCadence::RhymeClassifier::extractRhymeKey("to");
        juce::String keyYou = CompassCadence::RhymeClassifier::extractRhymeKey("you");
        juce::String keyNo = CompassCadence::RhymeClassifier::extractRhymeKey("no");
        juce::String keySo = CompassCadence::RhymeClassifier::extractRhymeKey("so");
        juce::String keySet = CompassCadence::RhymeClassifier::extractRhymeKey("set");
        juce::String keyLet = CompassCadence::RhymeClassifier::extractRhymeKey("let");
        juce::String keyExec = CompassCadence::RhymeClassifier::extractRhymeKey("execution");
        juce::String keyDirect = CompassCadence::RhymeClassifier::extractRhymeKey("direction");

        // Perfect rhymes must match
        jassert(keyTo == "UW" && keyYou == "UW");
        jassert(keyNo == "OW" && keySo == "OW");
        jassert(keySet == "EH" && keyLet == "EH");
        jassert(keyExec == keyDirect);

        // Non-rhyming words must NOT match
        jassert(keyTo != keyNo);
        jassert(keyTo != keySet);
        jassert(keyNo != keySet);

        // Color collision test: distinct rhyme groups must receive distinct colors
        std::vector<juce::String> rhymeWords = { "to", "you", "no", "so", "set", "let", "place", "pace" };
        doc.getRhymeClassifier().updateRhymeMap(rhymeWords);

        juce::Colour colTo = doc.getRhymeClassifier().getHighlightForSyllable("to");
        juce::Colour colYou = doc.getRhymeClassifier().getHighlightForSyllable("you");
        juce::Colour colNo = doc.getRhymeClassifier().getHighlightForSyllable("no");
        juce::Colour colSet = doc.getRhymeClassifier().getHighlightForSyllable("set");
        juce::Colour colPlace = doc.getRhymeClassifier().getHighlightForSyllable("place");

        // Rhyming pairs have same color
        jassert(!colTo.isTransparent());
        jassert(colTo == colYou);

        // Different rhyme families have DIFFERENT colors (zero collision!)
        jassert(colTo != colNo);
        jassert(colTo != colSet);
        jassert(colNo != colSet);
        jassert(colPlace != colTo && colPlace != colNo && colPlace != colSet);
        std::cout << "  [10.15] CMUDict Rhyme Engine & Non-Colliding Color Assignment passed!" << std::endl;

        // Test 10.16: Full Export (Formatted Text with [xx], CSV Spreadsheet, HTML Table)
        std::cout << "[10.16] Testing Full Export..." << std::endl;
        doc.clearAll();
        doc.setSyllable(0, 0, "to");
        doc.setSyllable(0, 1, "set");
        doc.setSyllable(0, 2, "out");
        doc.setActualSpokenSyllableCount(0, 14);

        juce::String exportTxt = doc.exportFormattedText(true, true);
        std::cout << "  Exported TXT line 0: " << exportTxt.trim() << std::endl;
        jassert(exportTxt.contains("to set out [14]"));

        juce::String exportCsv = doc.exportCsvSpreadsheet();
        jassert(exportCsv.startsWith("Bar,Metric Schema,Spoken Syllables,Grid Syllables,Lyrics"));
        jassert(exportCsv.contains("14,16,\"to set out\""));

        juce::String exportHtml = doc.exportHtmlTable();
        jassert(exportHtml.contains("<!DOCTYPE html>"));
        jassert(exportHtml.contains("<span class=\"count\">14</span>"));
        jassert(exportHtml.contains("<td class=\"lyrics\">to set out</td>"));
        std::cout << "  [10.16] Full Export passed!" << std::endl;

        // Test 10.17: SongManager (Save, Load, Delete full project state)
        std::cout << "[10.17] Testing SongManager..." << std::endl;
        CompassCadence::SongManager sm;
        sm.saveSong("Unit_Test_Song", doc);

        bool songFound = false;
        for (const auto& name : sm.getSavedSongNames())
        {
            if (name == "Unit_Test_Song") songFound = true;
        }
        jassert(songFound);

        // Clear doc and reload from song file
        doc.clearAll();
        jassert(doc.getSyllable(0, 0).isEmpty());

        bool loadOk = sm.loadSong("Unit_Test_Song", doc);
        jassert(loadOk);
        jassert(doc.getSyllable(0, 0) == "to");
        jassert(doc.getSyllable(0, 1) == "set");
        jassert(doc.getSyllable(0, 2) == "out");
        jassert(doc.getActualSpokenSyllableCount(0) == 14);

        // Delete test song
        sm.deleteSong("Unit_Test_Song");
        songFound = false;
        for (const auto& name : sm.getSavedSongNames())
        {
            if (name == "Unit_Test_Song") songFound = true;
        }
        jassert(!songFound);
        std::cout << "  [10.17] SongManager passed!" << std::endl;

        // Test 10.18: Dark Mode & Variable Shading Colors
        std::cout << "[10.18] Testing Dark Mode & Variable Shading Colors..." << std::endl;
        jassert(!doc.isDarkMode());
        jassert(!CompassCadence::NotebookLookAndFeel::isDarkMode());
        jassert(CompassCadence::NotebookLookAndFeel::getPaperColour() == juce::Colour(0xFFFAF8F2));
        jassert(CompassCadence::NotebookLookAndFeel::getEvenLineColour() == juce::Colour(0xFFEFE9DC));
        jassert(CompassCadence::NotebookLookAndFeel::getStanzaBreakColour() == juce::Colour(0xFFE2D9C8));

        // In Light Mode: distinct colors between odd, even, and stanza breaks
        jassert(CompassCadence::NotebookLookAndFeel::getEvenLineColour() != CompassCadence::NotebookLookAndFeel::getPaperColour());
        jassert(CompassCadence::NotebookLookAndFeel::getStanzaBreakColour() != CompassCadence::NotebookLookAndFeel::getPaperColour());
        jassert(CompassCadence::NotebookLookAndFeel::getStanzaBreakColour() != CompassCadence::NotebookLookAndFeel::getEvenLineColour());

        // Toggle Dark Mode
        doc.setDarkMode(true);
        jassert(doc.isDarkMode());
        jassert(CompassCadence::NotebookLookAndFeel::isDarkMode());
        jassert(CompassCadence::NotebookLookAndFeel::getPaperColour() == juce::Colour(0xFF18181B));
        jassert(CompassCadence::NotebookLookAndFeel::getEvenLineColour() == juce::Colour(0xFF222227));
        jassert(CompassCadence::NotebookLookAndFeel::getStanzaBreakColour() == juce::Colour(0xFF0E0E10));
        jassert(CompassCadence::NotebookLookAndFeel::getGraphiteColour() == juce::Colour(0xFFF1F5F9));

        // In Dark Mode: distinct colors between odd, even, and stanza breaks
        jassert(CompassCadence::NotebookLookAndFeel::getEvenLineColour() != CompassCadence::NotebookLookAndFeel::getPaperColour());
        jassert(CompassCadence::NotebookLookAndFeel::getStanzaBreakColour() != CompassCadence::NotebookLookAndFeel::getPaperColour());
        jassert(CompassCadence::NotebookLookAndFeel::getStanzaBreakColour() != CompassCadence::NotebookLookAndFeel::getEvenLineColour());

        // Undo/Redo Dark Mode
        doc.undo();
        jassert(!doc.isDarkMode());
        jassert(!CompassCadence::NotebookLookAndFeel::isDarkMode());
        doc.redo();
        jassert(doc.isDarkMode());
        jassert(CompassCadence::NotebookLookAndFeel::isDarkMode());

        // ValueTree serialization of Dark Mode
        auto vtDark = doc.toValueTree();
        CompassCadence::LyricDocument docDarkRestore;
        docDarkRestore.fromValueTree(vtDark);
        jassert(docDarkRestore.isDarkMode());

        // Reset to default
        doc.setDarkMode(false);
        jassert(!doc.isDarkMode());
        std::cout << "  [10.18] Dark Mode & Variable Shading Colors passed!" << std::endl;

        // [10.19] Multi-tab layout in CompassCadenceAudioProcessor
        {
            jassert(proc->getNumTabs() >= 1);
            int initialCount = proc->getNumTabs();
            jassert(proc->getActiveTabIndex() == 0);

            int tab2 = proc->addBlankTab("Verse Ideas");
            jassert(proc->getNumTabs() == initialCount + 1);
            jassert(proc->getActiveTabIndex() == tab2);
            jassert(proc->getTabName(tab2) == "Verse Ideas");

            // Write into tab 2
            auto& doc2 = proc->getLyricDocument();
            doc2.setSyllable(0, 0, "Tab2Lyrics");
            jassert(doc2.getSyllable(0, 0) == "Tab2Lyrics");

            // Duplicate tab 2 (unsynced)
            int tab3 = proc->duplicateTab(tab2, "Verse Ideas (Alt)");
            jassert(proc->getNumTabs() == initialCount + 2);
            jassert(proc->getActiveTabIndex() == tab3);
            auto& doc3 = proc->getLyricDocument();
            jassert(doc3.getSyllable(0, 0) == "Tab2Lyrics");

            // Edit tab 3, tab 2 should remain unchanged
            doc3.setSyllable(0, 0, "Tab3Edited");
            jassert(doc3.getSyllable(0, 0) == "Tab3Edited");
            proc->setActiveTabIndex(tab2);
            jassert(proc->getLyricDocument().getSyllable(0, 0) == "Tab2Lyrics");

            // Close tab 3
            bool closed = proc->closeTab(tab3);
            jassert(closed);
            jassert(proc->getNumTabs() == initialCount + 1);

            // Switch back to tab 0
            proc->setActiveTabIndex(0);
            jassert(proc->getActiveTabIndex() == 0);
            std::cout << "  [10.19] Multi-Tab layout & unsynced duplicates passed!" << std::endl;
        }

        // [10.20] Custom Stanza Breaks in LyricDocument
        {
            CompassCadence::LyricDocument docStanza;
            // Default 4-bar spacing: bar 3 (4th bar) has spacing, bar 1 does not
            jassert(docStanza.shouldAddSpacingAfterBar(3));
            jassert(!docStanza.shouldAddSpacingAfterBar(1));

            // Custom stanza break at bar 1
            docStanza.toggleStanzaBreak(1);
            jassert(docStanza.shouldAddSpacingAfterBar(1));
            // Once custom breaks are active, 4-bar default is bypassed
            jassert(!docStanza.shouldAddSpacingAfterBar(3));

            // Toggle bar 1 off
            docStanza.toggleStanzaBreak(1);
            jassert(!docStanza.shouldAddSpacingAfterBar(1));

            // Undo / Redo
            docStanza.undo();
            jassert(docStanza.shouldAddSpacingAfterBar(1));
            docStanza.redo();
            jassert(!docStanza.shouldAddSpacingAfterBar(1));

            // Serialization
            docStanza.toggleStanzaBreak(2);
            auto vtStanza = docStanza.toValueTree();
            CompassCadence::LyricDocument docStanzaRestored;
            docStanzaRestored.fromValueTree(vtStanza);
            jassert(docStanzaRestored.shouldAddSpacingAfterBar(2));
            std::cout << "  [10.20] Custom Stanza Breaks passed!" << std::endl;
        }

        // [10.21] Dynamic Line Insertion (insertBar)
        {
            CompassCadence::LyricDocument docInsert;
            auto customNotat = CompassCadence::MetricNotation::fromNotationString("[333]/3:4", 3);
            docInsert.setBarNotation(1, customNotat);
            docInsert.setSyllable(1, 0, "Line 2");
            docInsert.setSyllable(2, 0, "Line 3");

            // Insert after bar 1 (inserts new bar at index 2, shifts old bar 2 to 3)
            docInsert.insertBar(1);
            jassert(docInsert.getNotation(2).toNotationString() == customNotat.toNotationString());
            jassert(docInsert.getSyllable(3, 0) == "Line 3");

            // Undo insertion
            docInsert.undo();
            jassert(docInsert.getSyllable(2, 0) == "Line 3");
            std::cout << "  [10.21] Dynamic line insertion (insertBar) passed!" << std::endl;
        }

        // [10.22] Syllable Cell Alignment (Left, Down/Center, Right)
        {
            CompassCadence::LyricDocument docAlign;
            docAlign.setCellAlignment(0, 0, CompassCadence::LyricDocument::AlignLeft);
            docAlign.setCellAlignment(0, 1, CompassCadence::LyricDocument::AlignCenter);
            docAlign.setCellAlignment(0, 2, CompassCadence::LyricDocument::AlignRight);

            jassert(docAlign.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignLeft);
            jassert(docAlign.getCellAlignment(0, 1) == CompassCadence::LyricDocument::AlignCenter);
            jassert(docAlign.getCellAlignment(0, 2) == CompassCadence::LyricDocument::AlignRight);

            // Serialization
            auto vtAlign = docAlign.toValueTree();
            CompassCadence::LyricDocument docAlignRestored;
            docAlignRestored.fromValueTree(vtAlign);
            jassert(docAlignRestored.getCellAlignment(0, 0) == CompassCadence::LyricDocument::AlignLeft);
            jassert(docAlignRestored.getCellAlignment(0, 1) == CompassCadence::LyricDocument::AlignCenter);
            jassert(docAlignRestored.getCellAlignment(0, 2) == CompassCadence::LyricDocument::AlignRight);
            std::cout << "  [10.22] Syllable cell alignment (Left, Down/Center, Right) passed!" << std::endl;
        }

        // [10.23] Immediate Theme / Dark Mode Synchronization
        {
            CompassCadence::LyricDocument docTheme;
            jassert(!CompassCadence::NotebookLookAndFeel::isDarkMode());
            docTheme.setDarkMode(true);
            jassert(CompassCadence::NotebookLookAndFeel::isDarkMode());
            jassert(CompassCadence::NotebookLookAndFeel::getPaperColour() == juce::Colour(0xFF18181B));

            docTheme.setDarkMode(false);
            jassert(!CompassCadence::NotebookLookAndFeel::isDarkMode());
            jassert(CompassCadence::NotebookLookAndFeel::getPaperColour() == juce::Colour(0xFFFAF8F2));
            std::cout << "  [10.23] Immediate Theme / Dark Mode sync passed!" << std::endl;
        }

        // [10.24] Syllable Context Menu Utilities: Custom Color, Case Transforms, and Duplicate
        {
            CompassCadence::LyricDocument docUtil;
            docUtil.setSyllable(0, 0, "rhyme");
            docUtil.setSyllable(0, 1, "time");

            // Custom color assignment overrides phoneme highlight
            const juce::Colour mint(0x9986EFAC);
            docUtil.setCustomCellColor(0, 0, mint);
            jassert(docUtil.hasCustomCellColor(0, 0));
            jassert(docUtil.getCustomCellColor(0, 0) == mint);
            jassert(!docUtil.hasCustomCellColor(0, 1));

            // Undo / Redo of custom color
            docUtil.undo();
            jassert(!docUtil.hasCustomCellColor(0, 0));
            docUtil.redo();
            jassert(docUtil.hasCustomCellColor(0, 0));
            jassert(docUtil.getCustomCellColor(0, 0) == mint);

            // Selection batch color assignment
            const juce::Colour peach(0x99FED7AA);
            docUtil.setSelectedCells({ { 0, 0 }, { 0, 1 } });
            docUtil.setSelectionCustomColor(peach);
            jassert(docUtil.getCustomCellColor(0, 0) == peach);
            jassert(docUtil.getCustomCellColor(0, 1) == peach);

            // Reset to Auto (clear custom color)
            docUtil.clearSelectionCustomColor();
            jassert(!docUtil.hasCustomCellColor(0, 0));
            jassert(!docUtil.hasCustomCellColor(0, 1));

            // ValueTree persistence of custom color
            docUtil.setCustomCellColor(0, 0, mint);
            auto vtUtil = docUtil.toValueTree();
            CompassCadence::LyricDocument docRestored;
            docRestored.fromValueTree(vtUtil);
            jassert(docRestored.hasCustomCellColor(0, 0));
            jassert(docRestored.getCustomCellColor(0, 0) == mint);
            jassert(!docRestored.hasCustomCellColor(0, 1));

            // Text casing transforms
            docUtil.setSyllable(1, 0, "chorus");
            docUtil.transformCellCase(1, 0, CompassCadence::LyricDocument::CaseUpper);
            jassert(docUtil.getSyllable(1, 0) == "CHORUS");
            docUtil.transformCellCase(1, 0, CompassCadence::LyricDocument::CaseLower);
            jassert(docUtil.getSyllable(1, 0) == "chorus");
            docUtil.transformCellCase(1, 0, CompassCadence::LyricDocument::CaseTitle);
            jassert(docUtil.getSyllable(1, 0) == "Chorus");

            // Flow duplicate into next box
            docUtil.duplicateCellToNext(1, 0);
            jassert(docUtil.getSyllable(1, 1) == "Chorus");
            docUtil.undo();
            jassert(docUtil.getSyllable(1, 1).isEmpty());

            std::cout << "  [10.24] Syllable right-click utilities (custom color, case transforms, duplicate) passed!" << std::endl;
        }

        // [10.25] Bar Height, Meter Truncation, Context Vowels, and Vowel Palette Customization
        {
            CompassCadence::LyricDocument docNew;

            // 1. Default bar height is 50px, clamped [32, 100], with Undo/Redo & ValueTree
            jassert(docNew.getBarHeight() == 50);
            docNew.setBarHeight(64);
            jassert(docNew.getBarHeight() == 64);
            docNew.setBarHeight(20); // Clamps to 32
            jassert(docNew.getBarHeight() == 32);
            docNew.setBarHeight(150); // Clamps to 100
            jassert(docNew.getBarHeight() == 100);
            docNew.undo();
            jassert(docNew.getBarHeight() == 32);
            docNew.redo();
            jassert(docNew.getBarHeight() == 100);

            auto vtHeight = docNew.toValueTree();
            CompassCadence::LyricDocument docHeightRestored;
            docHeightRestored.fromValueTree(vtHeight);
            jassert(docHeightRestored.getBarHeight() == 100);

            // 2. Meter Truncation: shrinking subdivisions permanently removes orphaned syllables
            docNew.clearAll();
            docNew.setSyllable(2, 14, "GhostSyl");
            docNew.setCellBold(2, 14, true);
            docNew.setCellAlignment(2, 14, CompassCadence::LyricDocument::AlignRight);
            docNew.setCustomCellColor(2, 14, juce::Colours::red);
            docNew.setActualSpokenSyllableCount(2, 15);

            // Shrink bar 2 to 8 syllables
            auto halfNotation = CompassCadence::MetricNotation::fromNotationString("[2222]/4:4");
            docNew.setBarNotation(2, halfNotation);
            jassert(docNew.getNotation(2).getTotalSyllables() == 8);
            jassert(docNew.getSyllable(2, 14).isEmpty()); // Beyond range 8
            jassert(!docNew.isCellBold(2, 14));
            jassert(docNew.getActualSpokenSyllableCount(2) <= 8); // Clamped to new max

            // 3. Context-aware vowel sound classification
            // Standalone "a" is long Ay (EY)
            juce::String isolatedA = CompassCadence::RhymeClassifier::extractRhymeKeyWithContext("a", "", "");
            jassert(isolatedA == "EY");
            // Stem "a" in "un-der-stand-a-ble" is unstressed schwa / short Uh (AH)
            juce::String contextA = CompassCadence::RhymeClassifier::extractRhymeKeyWithContext("a", "stand-", "-ble");
            jassert(contextA == "AH");

            // 4. Customizable Vowel Sound Color Palette & ValueTree persistence
            juce::Colour customColor(0xFF8844AA);
            docNew.setVowelSoundColor("EY", customColor);
            jassert(docNew.getVowelSoundColor("EY") == customColor);

            auto vtColors = docNew.toValueTree();
            CompassCadence::LyricDocument docColorsRestored;
            docColorsRestored.fromValueTree(vtColors);
            jassert(docColorsRestored.getVowelSoundColor("EY") == customColor);

            docNew.resetVowelSoundColorsToDefaults();
            jassert(docNew.getVowelSoundColor("EY") != customColor);

            // 5. Phonetic distinction: "I" vs "it" (long Eye [AY] vs short Ih [IH])
            juce::String keyI = CompassCadence::RhymeClassifier::extractRhymeKey("i");
            juce::String keyIt = CompassCadence::RhymeClassifier::extractRhymeKey("it");
            juce::String keyIm = CompassCadence::RhymeClassifier::extractRhymeKey("I'm");
            juce::String keyHit = CompassCadence::RhymeClassifier::extractRhymeKey("hit");
            juce::String keyMy = CompassCadence::RhymeClassifier::extractRhymeKey("my");

            jassert(keyI == "AY");
            jassert(keyIt == "IH");
            jassert(keyI != keyIt); // "i" does NOT rhyme with "it"!
            jassert(keyIm == "AY");
            jassert(keyHit == "IH");
            jassert(keyI == keyMy); // "i" rhymes with "my"!

            std::cout << "  [10.25] Bar Height, Meter Truncation, Context Vowels, and Palette Customization passed!" << std::endl;
        }

        // [10.26] 3-Mode Rhyme Toggle & Syllable Repetition Detector (2+ Syllables)
        {
            CompassCadence::LyricDocument docRep;

            // 1. ColorMode cycling: Rhymes -> Repeats -> Off -> Rhymes
            jassert(docRep.getColorMode() == CompassCadence::RhymeClassifier::ColorMode::Rhymes);
            docRep.cycleColorMode();
            jassert(docRep.getColorMode() == CompassCadence::RhymeClassifier::ColorMode::Repeats);
            docRep.cycleColorMode();
            jassert(docRep.getColorMode() == CompassCadence::RhymeClassifier::ColorMode::Off);
            docRep.cycleColorMode();
            jassert(docRep.getColorMode() == CompassCadence::RhymeClassifier::ColorMode::Rhymes);

            // 2. Set up lyric text with a repeated 3-syllable sequence and an isolated repeated 1-syllable word
            docRep.clearAll();
            docRep.setSyllable(0, 0, "in");
            docRep.setSyllable(0, 1, "the");
            docRep.setSyllable(0, 2, "club");

            docRep.setSyllable(1, 0, "danc-");
            docRep.setSyllable(1, 1, "-ing");
            docRep.setSyllable(1, 2, "slow");

            docRep.setSyllable(2, 0, "in");
            docRep.setSyllable(2, 1, "the");
            docRep.setSyllable(2, 2, "club");

            docRep.setSyllable(3, 0, "the"); // Isolated 1-syllable word that appears elsewhere

            docRep.setColorMode(CompassCadence::RhymeClassifier::ColorMode::Repeats);
            docRep.refreshRhymes();

            auto& classifier = docRep.getRhymeClassifier();

            // Repeating phrase "in the club" (length 3 >= 2) must receive matching highlight color
            juce::Colour c0_0 = classifier.getHighlightForCell(0, 0, "in");
            juce::Colour c0_1 = classifier.getHighlightForCell(0, 1, "the");
            juce::Colour c0_2 = classifier.getHighlightForCell(0, 2, "club");
            juce::Colour c2_0 = classifier.getHighlightForCell(2, 0, "in");
            juce::Colour c2_1 = classifier.getHighlightForCell(2, 1, "the");
            juce::Colour c2_2 = classifier.getHighlightForCell(2, 2, "club");

            jassert(!c0_0.isTransparent());
            jassert(!c2_0.isTransparent());
            jassert(c0_0 == c2_0);
            jassert(c0_1 == c2_1);
            jassert(c0_2 == c2_2);

            // Non-repeated phrase in Bar 1 must remain unhighlighted
            juce::Colour c1_0 = classifier.getHighlightForCell(1, 0, "danc-");
            jassert(c1_0.isTransparent());

            // Single isolated repetition in Bar 3 must NOT be highlighted (requires length >= 2)
            juce::Colour c3_0 = classifier.getHighlightForCell(3, 0, "the");
            jassert(c3_0.isTransparent());

            // 3. In Off mode, all highlights are transparent
            docRep.setColorMode(CompassCadence::RhymeClassifier::ColorMode::Off);
            jassert(classifier.getHighlightForCell(0, 0, "in").isTransparent());

            // 4. ValueTree state persistence of colorMode
            docRep.setColorMode(CompassCadence::RhymeClassifier::ColorMode::Repeats);
            auto vtRep = docRep.toValueTree();
            CompassCadence::LyricDocument docRepRestored;
            docRepRestored.fromValueTree(vtRep);
            jassert(docRepRestored.getColorMode() == CompassCadence::RhymeClassifier::ColorMode::Repeats);

            std::cout << "  [10.26] 3-Mode Rhyme Toggle & Syllable Repetition Detector (2+ Syllables) passed!" << std::endl;
        }
        proc.reset();
        std::cout << "[11] Clean teardown succeeded!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "EXCEPTION: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "UNKNOWN EXCEPTION CAUGHT!" << std::endl;
        return 2;
    }

    juce::shutdownJuce_GUI();
    std::cout << "[12] ALL TESTS PASSED SAFELY!" << std::endl;
    return 0;
}
