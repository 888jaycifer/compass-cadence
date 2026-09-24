#include "NotebookHeaderComponent.h"
#include "NotebookLookAndFeel.h"
#include "VowelColorCustomizerDialog.h"
#include "KeyboardShortcutsDialog.h"

namespace CompassCadence
{

class NotebookHeaderComponent::RhymeButtonListener : public juce::MouseListener
{
public:
    RhymeButtonListener(NotebookHeaderComponent& owner) : header(owner) {}
    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu())
        {
            header.showColorModeMenu();
        }
    }
private:
    NotebookHeaderComponent& header;
};

NotebookHeaderComponent::NotebookHeaderComponent(CompassCadenceAudioProcessor& proc, LyricDocument& doc)
    : processor(proc), document(&doc)
{
    if (document != nullptr)
        document->addListener(this);

    // Notation Label & TextEditor
    notationLabel.setText("Metric Grid:", juce::dontSendNotification);
    notationLabel.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::bold)));
    notationLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    notationLabel.setTooltip("Metric cross-rhythm notation [subdivisions per pulse]/pulses:beats (e.g. [333222]/6:4, [4444]/4:4).");
    addAndMakeVisible(notationLabel);

    notationEditor.setFont(juce::Font(juce::FontOptions("Calibri", 14.0f, juce::Font::bold)));
    notationEditor.setJustification(juce::Justification::centred);
    notationEditor.setIndents(4, 1);
    notationEditor.addListener(this);
    updateEditorColours();
    notationEditor.setText(document->getNotation().toNotationString(), false);
    updateEditorColours();
    addAndMakeVisible(notationEditor);

    // Preset Combo & Save Button
    refreshPresetCombo();
    presetCombo.setTooltip("Select standard rhythmic flow template or custom preset.");
    presetCombo.addListener(this);
    addAndMakeVisible(presetCombo);

    savePresetBtn.setTooltip("Save current metric configuration as a custom preset.");
    savePresetBtn.onClick = [this] { promptSavePreset(); };
    addAndMakeVisible(savePresetBtn);

    // Independent Pulse Steppers
    pulseStepperLabel.setText("Pulses: 4", juce::dontSendNotification);
    pulseStepperLabel.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::plain)));
    pulseStepperLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    pulseStepperLabel.setTooltip("Current count of macro-pulse groups (N) spanning the bar.");
    addAndMakeVisible(pulseStepperLabel);

    decPulseBtn.setTooltip("Remove rightmost pulse group from the bar.");
    decPulseBtn.onClick = [this] {
        auto notat = document->getNotation();
        if (notat.getPulseCount() > 1)
        {
            notat.removePulse(notat.getPulseCount() - 1);
            document->setNotation(notat);
            updateNotationDisplay();
        }
    };
    addAndMakeVisible(decPulseBtn);

    incPulseBtn.setTooltip("Append a new pulse group to each bar.");
    incPulseBtn.onClick = [this] {
        auto notat = document->getNotation();
        notat.addPulse(4);
        document->setNotation(notat);
        updateNotationDisplay();
    };
    addAndMakeVisible(incPulseBtn);

    // Independent Beat Steppers
    beatStepperLabel.setText("Beats: 4", juce::dontSendNotification);
    beatStepperLabel.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::plain)));
    beatStepperLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    beatStepperLabel.setTooltip("Reference beats per bar (M). Standard is 4 for 4/4.");
    addAndMakeVisible(beatStepperLabel);

    decBeatBtn.setTooltip("Decrease reference beat count per bar (M).");
    decBeatBtn.onClick = [this] {
        auto notat = document->getNotation();
        if (notat.getBeatsPerBar() > 1)
        {
            notat.setBeatsPerBar(notat.getBeatsPerBar() - 1);
            document->setNotation(notat);
            updateNotationDisplay();
        }
    };
    addAndMakeVisible(decBeatBtn);

    incBeatBtn.setTooltip("Increase reference beat count per bar (M).");
    incBeatBtn.onClick = [this] {
        auto notat = document->getNotation();
        notat.setBeatsPerBar(notat.getBeatsPerBar() + 1);
        document->setNotation(notat);
        updateNotationDisplay();
    };
    addAndMakeVisible(incBeatBtn);

    // Toggles
    rhymeToggleBtn.setClickingTogglesState(false);
    updateRhymeButtonDisplay();
    rhymeToggleBtn.onClick = [this] {
        if (document == nullptr) return;
        document->getRhymeClassifier().cycleColorMode();
        updateRhymeButtonDisplay();
        document->refreshRhymes();
        document->notifyChanged();
        if (auto* p = processor.getAPVTS().getParameter("rhymeHighlight"))
            p->setValueNotifyingHost(document->getRhymeClassifier().isEnabled() ? 1.0f : 0.0f);
    };
    rhymeButtonListener = std::make_unique<RhymeButtonListener>(*this);
    rhymeToggleBtn.addMouseListener(rhymeButtonListener.get(), false);
    addAndMakeVisible(rhymeToggleBtn);

    rhymeColorsBtn.setTooltip("Customize vowel sound color palette...");
    rhymeColorsBtn.onClick = [this] {
        if (document != nullptr)
            VowelColorCustomizerDialog::showDialog(this, *document);
    };
    addAndMakeVisible(rhymeColorsBtn);

    themeAccentBtn.setTooltip("Choose UI Theme Accent Color (Amber, Blue, Green, Rose, Purple, Gold, Teal, or Custom)...");
    themeAccentBtn.onClick = [this] { showThemeAccentMenu(); };
    addAndMakeVisible(themeAccentBtn);

    followToggleBtn.setClickingTogglesState(true);
    followToggleBtn.setToggleState(processor.isFollowDAW(), juce::dontSendNotification);
    followToggleBtn.setButtonText(processor.isFollowDAW() ? "Follow DAW: ON" : "Follow DAW: OFF");
    followToggleBtn.setTooltip("Auto-scroll notebook view to follow active DAW playhead position.");
    followToggleBtn.onClick = [this] {
        bool on = followToggleBtn.getToggleState();
        processor.setFollowDAW(on);
        followToggleBtn.setButtonText(on ? "Follow DAW: ON" : "Follow DAW: OFF");
        if (auto* p = processor.getAPVTS().getParameter("followPlayhead"))
            p->setValueNotifyingHost(on ? 1.0f : 0.0f);
    };
    addAndMakeVisible(followToggleBtn);

    // Copy to Clipboard Button
    copyBtn.setTooltip("Copy highlighted lyrics to system clipboard (or all lyrics if none selected). Also Ctrl+C.");
    copyBtn.onClick = [this] {
        juce::String text = document ? document->getSelectedText() : juce::String();
        if (text.isEmpty() && document != nullptr)
            text = document->getAllText();

        if (text.isNotEmpty())
        {
            juce::SystemClipboard::copyTextToClipboard(text);
            copyBtn.setButtonText("Copied!");
            juce::Timer::callAfterDelay(1200, [this] {
                copyBtn.setButtonText("Copy");
            });
        }
    };
    addAndMakeVisible(copyBtn);

    // Export Button
    exportBtn.setTooltip("Export lyrics as formatted Text (.txt), Spreadsheet (.csv), or printable HTML table (.html).");
    exportBtn.onClick = [this] {
        juce::PopupMenu menu;
        menu.addItem(1, "Copy Formatted Text [xx] to Clipboard");
        menu.addItem(2, "Open / Save Formatted Text (.txt)...");
        menu.addItem(3, "Open / Save Spreadsheet Table (.csv)...");
        menu.addItem(4, "Open / Save Printable HTML Table (.html)...");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&exportBtn), [this](int result) {
            if (result == 1)
            {
                juce::String txt = document ? document->exportFormattedText(true, true) : juce::String();
                if (txt.isNotEmpty())
                {
                    juce::SystemClipboard::copyTextToClipboard(txt);
                    exportBtn.setButtonText("Copied!");
                    juce::Timer::callAfterDelay(1200, [this] {
                        exportBtn.setButtonText("Export");
                    });
                }
            }
            else if (result == 2)
            {
                promptExportTxt();
            }
            else if (result == 3)
            {
                promptExportCsv();
            }
            else if (result == 4)
            {
                promptExportHtml();
            }
        });
    };
    addAndMakeVisible(exportBtn);

    // Song Projects Button
    songsBtn.setTooltip("Save, load, and manage full song/lyrics projects (separate from baseline rhythmic presets).");
    songsBtn.onClick = [this] {
        juce::PopupMenu menu;
        menu.addItem(1, "+ Save Current Song Project...");
        menu.addSeparator();

        auto savedSongs = songManager.getSavedSongNames();
        if (savedSongs.empty())
        {
            menu.addItem(0, "(No saved songs yet)", false);
        }
        else
        {
            juce::PopupMenu loadMenu;
            for (size_t i = 0; i < savedSongs.size(); ++i)
            {
                loadMenu.addItem(100 + (int)i, savedSongs[i]);
            }
            menu.addSubMenu("Load Song Project", loadMenu);

            juce::PopupMenu delMenu;
            for (size_t i = 0; i < savedSongs.size(); ++i)
            {
                delMenu.addItem(500 + (int)i, "Delete: " + savedSongs[i]);
            }
            menu.addSubMenu("Delete Song Project", delMenu);
        }

        menu.addSeparator();
        menu.addItem(999, "New Song (Clear Notepad)");

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&songsBtn), [this, savedSongs](int result) {
            if (result == 1)
            {
                promptSaveSong();
            }
            else if (result >= 100 && result < 500)
            {
                int idx = result - 100;
                if (idx >= 0 && idx < (int)savedSongs.size() && document != nullptr)
                {
                    currentSongName = savedSongs[idx];
                    songManager.loadSong(savedSongs[idx], *document);
                    songsBtn.setButtonText("Loaded!");
                    juce::Timer::callAfterDelay(1200, [this] {
                        songsBtn.setButtonText("Songs");
                    });
                }
            }
            else if (result >= 500 && result < 999)
            {
                int idx = result - 500;
                if (idx >= 0 && idx < (int)savedSongs.size())
                {
                    songManager.deleteSong(savedSongs[idx]);
                }
            }
            else if (result == 999)
            {
                currentSongName = "Untitled Song";
                if (document != nullptr)
                    document->clearAll();
            }
        });
    };
    addAndMakeVisible(songsBtn);

    // Alignment Toggle Button
    alignToggleBtn.setClickingTogglesState(false);
    alignToggleBtn.setTooltip("Toggle concurrent visibility of alignment buttons on all syllable boxes.");
    alignToggleBtn.onClick = [this] {
        if (document == nullptr) return;
        document->setShowAlignmentControls(!document->getShowAlignmentControls());
        updateAlignToggleDisplay();
    };
    addAndMakeVisible(alignToggleBtn);

    // Keyboard Shortcuts Button (No emojis)
    shortcutsBtn.setTooltip("View keyboard shortcuts and metric alteration controls.");
    shortcutsBtn.onClick = [this] {
        KeyboardShortcutsDialog::showDialog(this);
    };
    addAndMakeVisible(shortcutsBtn);

    // Dark Mode Toggle
    darkModeToggleBtn.setClickingTogglesState(false);
    darkModeToggleBtn.setTooltip("Toggle between Warm Paper Light Mode and High-Contrast Dark Mode.");
    darkModeToggleBtn.onClick = [this] {
        if (document == nullptr) return;
        bool newDark = !document->isDarkMode();
        document->setDarkMode(newDark);
        if (auto* param = dynamic_cast<juce::AudioParameterBool*>(processor.getAPVTS().getParameter("darkMode")))
        {
            *param = newDark;
        }
        updateDarkModeDisplay();
    };
    addAndMakeVisible(darkModeToggleBtn);

    // View Mode Toggle
    viewModeToggleBtn.setClickingTogglesState(false);
    viewModeToggleBtn.onClick = [this] {
        if (document == nullptr) return;
        auto newMode = (document->getViewMode() == LyricDocument::ModeScroll)
                           ? LyricDocument::ModePages
                           : LyricDocument::ModeScroll;
        document->setViewMode(newMode);
        if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(processor.getAPVTS().getParameter("viewMode")))
        {
            *param = (newMode == LyricDocument::ModePages ? 1 : 0);
        }
        updateViewModeDisplay();
    };
    addAndMakeVisible(viewModeToggleBtn);

    // Page Controls
    pageLabel.setText("Page 1 / 4", juce::dontSendNotification);
    pageLabel.setFont(juce::Font(juce::FontOptions("Calibri", 12.0f, juce::Font::bold)));
    pageLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    pageLabel.setJustificationType(juce::Justification::centred);
    pageLabel.setTooltip("Active page / Total pages (automatable parameter in DAW).");
    addAndMakeVisible(pageLabel);

    prevPageBtn.setTooltip("Go to previous page (automatable parameter).");
    prevPageBtn.onClick = [this] {
        if (document == nullptr) return;
        if (document->getViewMode() == LyricDocument::ModeScroll)
        {
            if (onScrollByBars)
                onScrollByBars(-16);
        }
        else
        {
            int p = document->getCurrentPage();
            if (p > 0)
            {
                int targetPage = p - 1;
                document->setCurrentPage(targetPage);
                if (auto* param = dynamic_cast<juce::AudioParameterInt*>(processor.getAPVTS().getParameter("page")))
                {
                    *param = targetPage;
                }
            }
        }
    };
    addAndMakeVisible(prevPageBtn);

    nextPageBtn.setTooltip("Go to next page (adds a new page if on the last page).");
    nextPageBtn.onClick = [this] {
        if (document == nullptr) return;
        if (document->getViewMode() == LyricDocument::ModeScroll)
        {
            if (onScrollByBars)
                onScrollByBars(16);
        }
        else
        {
            int p = document->getCurrentPage();
            int tot = document->getTotalPages();
            if (p < tot - 1)
            {
                int targetPage = p + 1;
                document->setCurrentPage(targetPage);
                if (auto* param = dynamic_cast<juce::AudioParameterInt*>(processor.getAPVTS().getParameter("page")))
                {
                    *param = targetPage;
                }
            }
            else if (tot < 64)
            {
                document->addPage();
                int targetPage = p + 1;
                document->setCurrentPage(targetPage);
                if (auto* param = dynamic_cast<juce::AudioParameterInt*>(processor.getAPVTS().getParameter("page")))
                {
                    *param = targetPage;
                }
            }
        }
    };
    addAndMakeVisible(nextPageBtn);

    // DAW Status (for DAW VST3 plugin mode)
    dawStatusLabel.setText("120.0 BPM  |  4/4  |  [STOPPED]", juce::dontSendNotification);
    dawStatusLabel.setFont(juce::Font(juce::FontOptions("Calibri", 11.5f, juce::Font::bold)));
    dawStatusLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    dawStatusLabel.setJustificationType(juce::Justification::centredRight);
    dawStatusLabel.setTooltip("Host DAW tempo (BPM), time signature, and transport playback status.");
    addAndMakeVisible(dawStatusLabel);

    // Standalone Transport & Metronome Controls (for .exe standalone mode)
    standalonePlayBtn.setTooltip("Start or stop standalone transport playback (Spacebar).");
    standalonePlayBtn.onClick = [this] {
        processor.toggleStandalonePlayback();
        updateStandaloneTransportDisplay();
    };
    addChildComponent(standalonePlayBtn);

    metronomeToggleBtn.setTooltip("Toggle audio metronome click on/off for standalone playback.");
    metronomeToggleBtn.onClick = [this] {
        processor.setMetronomeEnabled(!processor.isMetronomeEnabled());
        updateStandaloneTransportDisplay();
    };
    addChildComponent(metronomeToggleBtn);

    decBpmBtn.setTooltip("Decrease standalone tempo (BPM).");
    decBpmBtn.onClick = [this] {
        processor.setStandaloneBpm(processor.getStandaloneBpm() - 5.0);
        updateStandaloneTransportDisplay();
    };
    addChildComponent(decBpmBtn);

    bpmLabel.setText("120 BPM", juce::dontSendNotification);
    bpmLabel.setFont(juce::Font(juce::FontOptions("Calibri", 11.5f, juce::Font::bold)));
    bpmLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    bpmLabel.setJustificationType(juce::Justification::centred);
    bpmLabel.setTooltip("Standalone playback tempo in BPM.");
    addChildComponent(bpmLabel);

    incBpmBtn.setTooltip("Increase standalone tempo (BPM).");
    incBpmBtn.onClick = [this] {
        processor.setStandaloneBpm(processor.getStandaloneBpm() + 5.0);
        updateStandaloneTransportDisplay();
    };
    addChildComponent(incBpmBtn);

    updateNotationDisplay();
    updateViewModeDisplay();
    updateDarkModeDisplay();
    updateAlignToggleDisplay();
    updateDAWStatus(processor.getTransportState());
}

NotebookHeaderComponent::~NotebookHeaderComponent()
{
    if (document != nullptr)
        document->removeListener(this);
}

void NotebookHeaderComponent::setDocument(LyricDocument& newDoc)
{
    if (document != &newDoc)
    {
        if (document != nullptr)
            document->removeListener(this);

        document = &newDoc;
        document->addListener(this);

        updateNotationDisplay();
        updatePageDisplay();
        updateViewModeDisplay();
        updateDarkModeDisplay();
        updateAlignToggleDisplay();
        updateRhymeButtonDisplay();
        refreshPresetCombo();
        repaint();
    }
}

void NotebookHeaderComponent::updateDAWStatus(const TransportState& st)
{
    if (processor.isStandalone())
    {
        dawStatusLabel.setVisible(false);
        standalonePlayBtn.setVisible(true);
        metronomeToggleBtn.setVisible(true);
        decBpmBtn.setVisible(true);
        bpmLabel.setVisible(true);
        incBpmBtn.setVisible(true);
        updateStandaloneTransportDisplay();
    }
    else
    {
        dawStatusLabel.setVisible(true);
        standalonePlayBtn.setVisible(false);
        metronomeToggleBtn.setVisible(false);
        decBpmBtn.setVisible(false);
        bpmLabel.setVisible(false);
        incBpmBtn.setVisible(false);

        juce::String status = juce::String(st.bpm, 1) + " BPM  |  "
                            + juce::String(st.timeSigNumerator) + "/" + juce::String(st.timeSigDenominator) + "  |  "
                            + (st.isPlaying ? "[PLAYING]" : "[STOPPED]");

        if (dawStatusLabel.getText() != status)
            dawStatusLabel.setText(status, juce::dontSendNotification);
    }
}

void NotebookHeaderComponent::updateStandaloneTransportDisplay()
{
    if (!processor.isStandalone())
        return;

    bool playing = processor.isStandalonePlaying();
    standalonePlayBtn.setButtonText(playing ? "Stop" : "Play");
    standalonePlayBtn.setToggleState(playing, juce::dontSendNotification);

    bool clickOn = processor.isMetronomeEnabled();
    metronomeToggleBtn.setButtonText(clickOn ? "Click: ON" : "Click: OFF");
    metronomeToggleBtn.setToggleState(clickOn, juce::dontSendNotification);

    bpmLabel.setText(juce::String((int)std::round(processor.getStandaloneBpm())) + " BPM", juce::dontSendNotification);
}

void NotebookHeaderComponent::updateNotationDisplay()
{
    if (document == nullptr) return;
    const auto& notat = document->getNotation();
    juce::String str = notat.toNotationString();
    if (notationEditor.getText() != str)
    {
        notationEditor.setText(str, false);
        updateEditorColours();
    }

    pulseStepperLabel.setText("Pulses: " + juce::String(notat.getPulseCount()), juce::dontSendNotification);
    beatStepperLabel.setText("Beats: " + juce::String(notat.getBeatsPerBar()), juce::dontSendNotification);
}

void NotebookHeaderComponent::updatePageDisplay()
{
    if (document == nullptr) return;
    bool isScroll = (document->getViewMode() == LyricDocument::ModeScroll);
    if (isScroll)
    {
        pageLabel.setText("Bars 1-" + juce::String(document->getBarCount()), juce::dontSendNotification);
        pageLabel.setTooltip("Continuous scroll canvas (" + juce::String(document->getBarCount()) + " bars).");
        prevPageBtn.setEnabled(true);
        prevPageBtn.setTooltip("Scroll up 16 bars (Page Up).");
        nextPageBtn.setEnabled(true);
        nextPageBtn.setTooltip("Scroll down 16 bars (Page Down).");
    }
    else
    {
        int cur = document->getCurrentPage() + 1;
        int tot = document->getTotalPages();
        pageLabel.setText("Page " + juce::String(cur) + " / " + juce::String(tot), juce::dontSendNotification);
        pageLabel.setTooltip("Active page / Total pages (automatable parameter in DAW).");
        prevPageBtn.setEnabled(document->getCurrentPage() > 0);
        prevPageBtn.setTooltip("Go to previous 16-bar page.");
        nextPageBtn.setEnabled(document->getCurrentPage() < tot - 1 || tot < 64);
        nextPageBtn.setTooltip("Go to next 16-bar page (adds a new page if on last page).");
    }
}

void NotebookHeaderComponent::updateViewModeDisplay()
{
    if (document == nullptr) return;
    bool isScroll = (document->getViewMode() == LyricDocument::ModeScroll);
    viewModeToggleBtn.setButtonText(isScroll ? "Scroll" : "Pages");
    viewModeToggleBtn.setToggleState(isScroll, juce::dontSendNotification);
    viewModeToggleBtn.setTooltip(isScroll ? "Current: Continuous Scroll Mode. Click to switch to 16-Bar Pages."
                                         : "Current: 16-Bar Pages Mode. Click to switch to Continuous Scroll.");
    updatePageDisplay();
}

void NotebookHeaderComponent::updateEditorColours()
{
    bool dark = document != nullptr ? document->isDarkMode() : NotebookLookAndFeel::isDarkMode();
    auto textCol     = dark ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF1E293B);
    auto bgCol       = dark ? juce::Colour(0xFF27272A) : juce::Colour(0xFFFFFFFF);
    auto outlineCol  = dark ? juce::Colour(0xFF52525B) : juce::Colour(0xFFCBD5E1);
    auto focusCol    = dark ? juce::Colour(0xFFF59E0B) : juce::Colour(0xFFD97706);

    notationEditor.setColour(juce::TextEditor::textColourId, textCol);
    notationEditor.setColour(juce::TextEditor::backgroundColourId, bgCol);
    notationEditor.setColour(juce::TextEditor::outlineColourId, outlineCol);
    notationEditor.setColour(juce::TextEditor::focusedOutlineColourId, focusCol);
    notationEditor.setColour(juce::TextEditor::highlightColourId, dark ? juce::Colour(0x603B82F6) : juce::Colour(0x60FFF59D));
    notationEditor.setColour(juce::TextEditor::highlightedTextColourId, textCol);

    notationEditor.applyColourToAllText(textCol, true);
}

void NotebookHeaderComponent::updateDarkModeDisplay()
{
    if (document == nullptr) return;
    bool dark = document->isDarkMode();
    NotebookLookAndFeel::setDarkMode(dark);

    darkModeToggleBtn.setButtonText(dark ? "Dark: ON" : "Dark: OFF");
    darkModeToggleBtn.setToggleState(dark, juce::dontSendNotification);
    darkModeToggleBtn.setTooltip(dark ? "Current: Dark Mode. Click to switch to Light Mode."
                                      : "Current: Light Mode. Click to switch to Dark Mode.");

    // Update label text colors
    notationLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    pulseStepperLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    beatStepperLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    pageLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    dawStatusLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());
    bpmLabel.setColour(juce::Label::textColourId, NotebookLookAndFeel::getGraphiteColour());

    updateEditorColours();

    repaint();
    if (auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>())
        editor->repaint();
}

void NotebookHeaderComponent::updateRhymeButtonDisplay()
{
    if (document == nullptr) return;
    auto mode = document->getRhymeClassifier().getColorMode();
    int minLen = document->getMinRepeatLength();
    switch (mode)
    {
        case RhymeClassifier::ColorMode::Off:
            rhymeToggleBtn.setToggleState(false, juce::dontSendNotification);
            rhymeToggleBtn.setButtonText("Colors: OFF");
            rhymeToggleBtn.setTooltip("Color Mode: OFF. Click to cycle to Rhymes (phonetic vowels), then Repeats. Right-click for menu.");
            break;
        case RhymeClassifier::ColorMode::Rhymes:
            rhymeToggleBtn.setToggleState(true, juce::dontSendNotification);
            rhymeToggleBtn.setButtonText("Rhymes: ON");
            rhymeToggleBtn.setTooltip("Color Mode: Rhyme Scheme (phonetic vowel & slant rhymes). Click for Repeats. Right-click for menu.");
            break;
        case RhymeClassifier::ColorMode::Repeats:
            rhymeToggleBtn.setToggleState(true, juce::dontSendNotification);
            rhymeToggleBtn.setButtonText("Repeats: " + juce::String(minLen) + "+");
            rhymeToggleBtn.setTooltip("Color Mode: Repetition Detector (highlights matching sequences of " + juce::String(minLen) + "+ exact syllables). Click to turn OFF. Right-click for menu.");
            break;
    }
}

void NotebookHeaderComponent::showColorModeMenu()
{
    if (document == nullptr) return;
    auto curMode = document->getRhymeClassifier().getColorMode();
    int minLen = document->getMinRepeatLength();
    int curDist = document->getMaxRepeatLineDistance();

    juce::PopupMenu menu;
    menu.addItem(1, "Rhymes (Phonetic Vowels)", true, curMode == RhymeClassifier::ColorMode::Rhymes);
    
    juce::PopupMenu repeatsMenu;
    repeatsMenu.addItem(101, "1+ Syllables", true, curMode == RhymeClassifier::ColorMode::Repeats && minLen == 1);
    repeatsMenu.addItem(2, "2+ Syllables", true, curMode == RhymeClassifier::ColorMode::Repeats && minLen == 2);
    repeatsMenu.addItem(3, "3+ Syllables", true, curMode == RhymeClassifier::ColorMode::Repeats && minLen == 3);
    repeatsMenu.addItem(4, "4+ Syllables", true, curMode == RhymeClassifier::ColorMode::Repeats && minLen == 4);
    repeatsMenu.addItem(105, "5+ Syllables", true, curMode == RhymeClassifier::ColorMode::Repeats && minLen == 5);
    repeatsMenu.addItem(106, "6+ Syllables", true, curMode == RhymeClassifier::ColorMode::Repeats && minLen == 6);
    repeatsMenu.addItem(10, "Custom Syllable Count (" + juce::String(minLen) + "+)...", true);
    repeatsMenu.addSeparator();

    juce::PopupMenu distMenu;
    distMenu.addItem(20, "0 Lines (Same Line Only)", true, curDist == 0);
    distMenu.addItem(21, "1 Line (Couplets)", true, curDist == 1);
    distMenu.addItem(22, "2 Lines", true, curDist == 2);
    distMenu.addItem(23, "4 Lines (Single Stanza)", true, curDist == 4);
    distMenu.addItem(24, "8 Lines (2 Stanzas)", true, curDist == 8);
    distMenu.addItem(25, "12 Lines", true, curDist == 12);
    distMenu.addItem(26, "16 Lines (1 Page)", true, curDist == 16);
    distMenu.addItem(27, "24 Lines (Default Baseline)", true, curDist == 24);
    distMenu.addItem(28, "32 Lines (2 Pages)", true, curDist == 32);
    distMenu.addItem(29, "48 Lines", true, curDist == 48);
    distMenu.addItem(30, "64 Lines (Full Song)", true, curDist == 64);
    distMenu.addItem(31, "256 Lines (Unlimited / All)", true, curDist >= 256);
    distMenu.addSeparator();
    distMenu.addItem(50, "Custom Line Distance (" + juce::String(curDist) + " lines)...", true);
    repeatsMenu.addSubMenu("Max Line Distance (" + juce::String(curDist) + " lines)", distMenu, true);

    menu.addSubMenu("Repeats (Exact Matches)", repeatsMenu, true);

    menu.addItem(5, "Colors OFF", true, curMode == RhymeClassifier::ColorMode::Off);
    menu.addSeparator();

    juce::PopupMenu tupletMenu;
    auto tMode = document->getTupletBracketMode();
    tupletMenu.addItem(801, "All Lines (Always On)", true, tMode == LyricDocument::BracketAllOn);
    tupletMenu.addItem(802, "Active Line Only", true, tMode == LyricDocument::BracketActiveLine);
    tupletMenu.addItem(803, "All Off (Hidden)", true, tMode == LyricDocument::BracketAllOff);
    menu.addSubMenu("Tuplet Brackets", tupletMenu, true);
    menu.addSeparator();

    if (document->hasHiddenSequences())
    {
        menu.addItem(6, "Unhide All Rhyme Color Pairs / Sequences");
    }
    menu.addItem(7, "Clear All Custom Cell Colors (Reset to Auto)");

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&rhymeToggleBtn), [this](int result) {
        if (document == nullptr || result == 0) return;
        if (result == 1)
        {
            document->setColorMode(RhymeClassifier::ColorMode::Rhymes);
        }
        else if (result == 101)
        {
            document->setMinRepeatLength(1);
            document->setColorMode(RhymeClassifier::ColorMode::Repeats);
        }
        else if (result == 2)
        {
            document->setMinRepeatLength(2);
            document->setColorMode(RhymeClassifier::ColorMode::Repeats);
        }
        else if (result == 3)
        {
            document->setMinRepeatLength(3);
            document->setColorMode(RhymeClassifier::ColorMode::Repeats);
        }
        else if (result == 4)
        {
            document->setMinRepeatLength(4);
            document->setColorMode(RhymeClassifier::ColorMode::Repeats);
        }
        else if (result == 105)
        {
            document->setMinRepeatLength(5);
            document->setColorMode(RhymeClassifier::ColorMode::Repeats);
        }
        else if (result == 106)
        {
            document->setMinRepeatLength(6);
            document->setColorMode(RhymeClassifier::ColorMode::Repeats);
        }
        else if (result == 10)
        {
            auto* w = new juce::AlertWindow("Custom Repeat Syllables", "Enter minimum syllable count for repeat match detection (1 to 32):", juce::AlertWindow::QuestionIcon);
            w->addTextEditor("syl", juce::String(document->getMinRepeatLength()), "Syllables:");
            w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
            w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int r) {
                if (r == 1 && document != nullptr)
                {
                    int val = w->getTextEditorContents("syl").getIntValue();
                    if (val >= 1)
                    {
                        document->setMinRepeatLength(val);
                        document->setColorMode(RhymeClassifier::ColorMode::Repeats);
                        updateRhymeButtonDisplay();
                        document->refreshRhymes();
                        document->notifyChanged();
                    }
                }
            }), true);
            return;
        }
        else if (result >= 20 && result <= 31)
        {
            const int distValues[] = { 0, 1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 256 };
            int idx = result - 20;
            if (idx >= 0 && idx < 12)
            {
                document->setMaxRepeatLineDistance(distValues[idx]);
                document->setColorMode(RhymeClassifier::ColorMode::Repeats);
            }
        }
        else if (result == 50)
        {
            auto* w = new juce::AlertWindow("Custom Max Line Distance", "Enter maximum line distance for repetition matches (0 = same line, 1 = couplet, 24 = default baseline, up to 256):", juce::AlertWindow::QuestionIcon);
            w->addTextEditor("dist", juce::String(document->getMaxRepeatLineDistance()), "Lines:");
            w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
            w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
            w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int r) {
                if (r == 1 && document != nullptr)
                {
                    int val = w->getTextEditorContents("dist").getIntValue();
                    if (val >= 0)
                    {
                        document->setMaxRepeatLineDistance(val);
                        document->setColorMode(RhymeClassifier::ColorMode::Repeats);
                        updateRhymeButtonDisplay();
                        document->refreshRhymes();
                        document->notifyChanged();
                    }
                }
            }), true);
            return;
        }
        else if (result == 5)
        {
            document->setColorMode(RhymeClassifier::ColorMode::Off);
        }
        else if (result == 6)
        {
            document->unhideAllSequences();
        }
        else if (result == 7)
        {
            document->clearAllCustomCellColors();
        }
        else if (result == 801)
        {
            document->setTupletBracketMode(LyricDocument::BracketAllOn);
        }
        else if (result == 802)
        {
            document->setTupletBracketMode(LyricDocument::BracketActiveLine);
        }
        else if (result == 803)
        {
            document->setTupletBracketMode(LyricDocument::BracketAllOff);
        }

        updateRhymeButtonDisplay();
        document->refreshRhymes();
        document->notifyChanged();
        if (auto* p = processor.getAPVTS().getParameter("rhymeHighlight"))
            p->setValueNotifyingHost(document->getRhymeClassifier().isEnabled() ? 1.0f : 0.0f);
    });
}

void NotebookHeaderComponent::updateAlignToggleDisplay()
{
    if (document == nullptr) return;
    bool show = document->getShowAlignmentControls();
    alignToggleBtn.setToggleState(show, juce::dontSendNotification);
    alignToggleBtn.setButtonText(show ? "Align: ON" : "Align: OFF");
}

void NotebookHeaderComponent::lyricDocumentChanged()
{
    updateNotationDisplay();
    updateViewModeDisplay();
    updateDarkModeDisplay();
    updateRhymeButtonDisplay();
    updateAlignToggleDisplay();
}

void NotebookHeaderComponent::metricNotationChanged(const MetricNotation&)
{
    updateNotationDisplay();
}

void NotebookHeaderComponent::applyNotationFromText()
{
    if (document == nullptr) return;
    juce::String text = notationEditor.getText().trim();
    if (text.isNotEmpty())
    {
        MetricNotation parsed = MetricNotation::fromNotationString(text, document->getNotation().getBeatsPerBar());
        document->setNotation(parsed);
        updateNotationDisplay();
    }
    updateEditorColours();
}

void NotebookHeaderComponent::textEditorTextChanged(juce::TextEditor& ed)
{
    if (&ed == &notationEditor)
    {
        bool dark = document != nullptr ? document->isDarkMode() : NotebookLookAndFeel::isDarkMode();
        auto textCol = dark ? juce::Colour(0xFFF1F5F9) : juce::Colour(0xFF1E293B);
        notationEditor.applyColourToAllText(textCol, true);
    }
}

void NotebookHeaderComponent::refreshPresetCombo()
{
    presetCombo.clear(juce::dontSendNotification);

    // 1. Factory Templates (IDs 1..6)
    const auto& factory = presetManager.getFactoryPresets();
    for (size_t i = 0; i < factory.size(); ++i)
    {
        presetCombo.addItem(factory[i].name, (int)i + 1);
    }

    // 2. User Custom Presets (IDs 100+)
    const auto& user = presetManager.getUserPresets();
    if (!user.empty())
    {
        presetCombo.addSeparator();
        for (size_t i = 0; i < user.size(); ++i)
        {
            juce::String label = user[i].name;
            if (!label.contains(user[i].notation))
                label += " [" + user[i].notation + "]";
            presetCombo.addItem(label, 100 + (int)i);
        }
    }

    // 3. Actions
    presetCombo.addSeparator();
    presetCombo.addItem("+ Save Current As Preset...", 998);
    if (!user.empty())
    {
        presetCombo.addItem("Delete Custom Preset...", 999);
    }

    presetCombo.setTextWhenNothingSelected("Templates / Presets...");
}

void NotebookHeaderComponent::promptSavePreset()
{
    if (document == nullptr) return;
    juce::String curNotation = document->getNotation().toNotationString();
    auto* w = new juce::AlertWindow("Save Custom Preset", "Enter a name for this metric preset:", juce::AlertWindow::NoIcon);
    w->addTextEditor("presetName", currentPresetName.isNotEmpty() ? currentPresetName : ("My Flow " + curNotation));
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    w->enterModalState(true, juce::ModalCallbackFunction::create([this, w, curNotation](int result) {
        if (result == 1 && document != nullptr)
        {
            juce::String name = w->getTextEditorContents("presetName").trim();
            if (name.isNotEmpty())
            {
                currentPresetName = name;
                int bpb = document->getNotation().getBeatsPerBar();
                presetManager.saveUserPreset(name, curNotation, bpb);
                refreshPresetCombo();
            }
        }
        delete w;
    }));
}

void NotebookHeaderComponent::promptDeletePreset()
{
    const auto& user = presetManager.getUserPresets();
    if (user.empty())
        return;

    juce::PopupMenu m;
    for (size_t i = 0; i < user.size(); ++i)
    {
        m.addItem((int)i + 1, "Delete: " + user[i].name + " [" + user[i].notation + "]");
    }

    m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&presetCombo), [this](int result) {
        if (result > 0)
        {
            presetManager.deleteUserPreset(result - 1);
            refreshPresetCombo();
        }
    });
}

void NotebookHeaderComponent::promptSaveSong()
{
    auto* w = new juce::AlertWindow("Save Song Project", "Enter a name for this song project:", juce::AlertWindow::NoIcon);
    w->addTextEditor("songName", currentSongName.isNotEmpty() ? currentSongName : "My Song");
    w->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    w->enterModalState(true, juce::ModalCallbackFunction::create([this, w](int result) {
        if (result == 1 && document != nullptr)
        {
            juce::String name = w->getTextEditorContents("songName").trim();
            if (name.isNotEmpty())
            {
                currentSongName = name;
                songManager.saveSong(name, *document);
                songsBtn.setButtonText("Saved!");
                juce::Timer::callAfterDelay(1200, [this] {
                    songsBtn.setButtonText("Songs");
                });
            }
        }
        delete w;
    }));
}

void NotebookHeaderComponent::promptExportTxt()
{
    if (document == nullptr) return;
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CompassCadence").getChildFile("exports");
    dir.createDirectory();
    auto file = dir.getChildFile("lyrics_export.txt");
    juce::String text = document->exportFormattedText(true, true);
    if (file.replaceWithText(text))
    {
        file.startAsProcess();
        exportBtn.setButtonText("Exported!");
        juce::Timer::callAfterDelay(1200, [this] {
            exportBtn.setButtonText("Export");
        });
    }
}

void NotebookHeaderComponent::promptExportCsv()
{
    if (document == nullptr) return;
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CompassCadence").getChildFile("exports");
    dir.createDirectory();
    auto file = dir.getChildFile("lyrics_table.csv");
    juce::String csv = document->exportCsvSpreadsheet();
    if (file.replaceWithText(csv))
    {
        file.startAsProcess();
        exportBtn.setButtonText("Exported!");
        juce::Timer::callAfterDelay(1200, [this] {
            exportBtn.setButtonText("Export");
        });
    }
}

void NotebookHeaderComponent::promptExportHtml()
{
    if (document == nullptr) return;
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("CompassCadence").getChildFile("exports");
    dir.createDirectory();
    auto file = dir.getChildFile("lyrics_sheet.html");
    juce::String html = document->exportHtmlTable("compass4cadence — Lyric Sheet");
    if (file.replaceWithText(html))
    {
        file.startAsProcess();
        exportBtn.setButtonText("Opened!");
        juce::Timer::callAfterDelay(1200, [this] {
            exportBtn.setButtonText("Export");
        });
    }
}

void NotebookHeaderComponent::applyPreset(int presetId)
{
    if (document == nullptr) return;
    if (presetId >= 1 && presetId <= 6)
    {
        MetricNotation notat;
        switch (presetId)
        {
            case 1: notat = MetricNotation(4, { 4, 4, 4, 4 }); currentPresetName = "Straight 16ths"; break;
            case 2: notat = MetricNotation(4, { 2, 2, 2, 2 }); currentPresetName = "Straight 8ths"; break;
            case 3: notat = MetricNotation(4, { 3, 3, 3, 3 }); currentPresetName = "Triplets"; break;
            case 4: notat = MetricNotation(4, { 5, 5, 5, 5 }); currentPresetName = "Quintuplets"; break;
            case 5: notat = MetricNotation(4, { 6, 6, 6, 6 }); currentPresetName = "Sextuplets"; break;
            case 6: notat = MetricNotation(3, { 3, 3, 3 }); currentPresetName = "Waltz"; break;
            default: return;
        }
        document->setNotation(notat);
        updateNotationDisplay();
    }
    else if (presetId >= 100 && presetId < 998)
    {
        int userIdx = presetId - 100;
        const auto& user = presetManager.getUserPresets();
        if (userIdx >= 0 && userIdx < (int)user.size())
        {
            currentPresetName = user[userIdx].name;
            MetricNotation parsed = MetricNotation::fromNotationString(user[userIdx].notation, user[userIdx].beatsPerBar);
            document->setNotation(parsed);
            updateNotationDisplay();
        }
    }
}

void NotebookHeaderComponent::textEditorReturnKeyPressed(juce::TextEditor&)
{
    applyNotationFromText();
}

void NotebookHeaderComponent::textEditorFocusLost(juce::TextEditor&)
{
    applyNotationFromText();
}

void NotebookHeaderComponent::comboBoxChanged(juce::ComboBox* cb)
{
    if (cb == &presetCombo)
    {
        int id = presetCombo.getSelectedId();
        if (id == 998)
        {
            presetCombo.setSelectedId(0, juce::dontSendNotification);
            promptSavePreset();
        }
        else if (id == 999)
        {
            presetCombo.setSelectedId(0, juce::dontSendNotification);
            promptDeletePreset();
        }
        else if (id > 0)
        {
            applyPreset(id);
        }
    }
}

void NotebookHeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // 1. Warm cream notebook paper background (continuous with page below)
    g.setColour(NotebookLookAndFeel::getPaperColour());
    g.fillRect(bounds);

    // 2. Ruled horizontal faint blue lines (matching the 36px bar pitch of the page)
    g.setColour(NotebookLookAndFeel::getRuleLineColour());
    g.drawLine(0.0f, 36.0f, bounds.getRight(), 36.0f, 1.0f);
    g.drawLine(0.0f, bounds.getBottom() - 1.0f, bounds.getRight(), bounds.getBottom() - 1.0f, 1.0f);

    // 3. Continuous vertical Red Margin Rule (running straight from the top edge at x = 65.0f)
    g.setColour(NotebookLookAndFeel::getMarginRedColour());
    g.drawLine(MARGIN_X, 0.0f, MARGIN_X, bounds.getBottom(), 1.5f);

    // 4. Spiral Binder Wire Rings along the far left edge
    NotebookLookAndFeel::drawSpiralRings(g, 12.0f, bounds.getHeight(), 18.0f, 28.8f);
}

void NotebookHeaderComponent::resized()
{
    auto bounds = getLocalBounds();

    // All controls live strictly to the right of the vertical red margin line (65px)
    int startX = (int)MARGIN_X + 14; // 79px
    int totalW = bounds.getWidth() - startX - 16;
    if (totalW <= 0)
        return;

    // Row 1: y = 5 to 31 (height 26px)
    int row1Y = 5;
    int curX = startX;

    notationLabel.setBounds(curX, row1Y, 74, 26);
    curX += 74;

    notationEditor.setBounds(curX, row1Y, 120, 26);
    curX += 120 + 8;

    presetCombo.setBounds(curX, row1Y, 155, 26);
    curX += 155 + 6;

    savePresetBtn.setBounds(curX, row1Y, 46, 26);
    curX += 46 + 10;

    decPulseBtn.setBounds(curX, row1Y, 22, 26);
    curX += 22;
    pulseStepperLabel.setBounds(curX, row1Y, 62, 26);
    curX += 62;
    incPulseBtn.setBounds(curX, row1Y, 22, 26);
    curX += 22 + 8;

    decBeatBtn.setBounds(curX, row1Y, 22, 26);
    curX += 22;
    beatStepperLabel.setBounds(curX, row1Y, 58, 26);
    curX += 58;
    incBeatBtn.setBounds(curX, row1Y, 22, 26);

    // Right side of Row 1: DAW Status (in DAW) or Standalone Transport (in .exe)
    int rightW = 210;
    int rightX = bounds.getWidth() - 225;

    dawStatusLabel.setBounds(rightX, row1Y, rightW, 26);

    int sX = rightX;
    standalonePlayBtn.setBounds(sX, row1Y, 44, 26);
    sX += 44 + 4;

    metronomeToggleBtn.setBounds(sX, row1Y, 66, 26);
    sX += 66 + 4;

    decBpmBtn.setBounds(sX, row1Y, 18, 26);
    sX += 18;

    bpmLabel.setBounds(sX, row1Y, 56, 26);
    sX += 56;

    incBpmBtn.setBounds(sX, row1Y, 18, 26);

    // Row 2: y = 41 to 67 (height 26px)
    int row2Y = 41;
    curX = startX;

    rhymeToggleBtn.setBounds(curX, row2Y, 96, 26);
    curX += 96 + 3;

    rhymeColorsBtn.setBounds(curX, row2Y, 32, 26);
    curX += 32 + 4;

    themeAccentBtn.setBounds(curX, row2Y, 52, 26);
    curX += 52 + 6;

    followToggleBtn.setBounds(curX, row2Y, 114, 26);
    curX += 114 + 6;

    copyBtn.setBounds(curX, row2Y, 52, 26);
    curX += 52 + 6;

    exportBtn.setBounds(curX, row2Y, 56, 26);
    curX += 56 + 6;

    songsBtn.setBounds(curX, row2Y, 56, 26);
    curX += 56 + 6;

    alignToggleBtn.setBounds(curX, row2Y, 78, 26);
    curX += 78 + 6;

    shortcutsBtn.setBounds(curX, row2Y, 72, 26);

    // Right side of Row 2: Page controls & View mode toggle & Dark mode toggle
    int pageRight = bounds.getWidth() - 16;
    nextPageBtn.setBounds(pageRight - 26, row2Y, 26, 26);
    pageLabel.setBounds(pageRight - 26 - 90, row2Y, 90, 26);
    prevPageBtn.setBounds(pageRight - 26 - 90 - 26, row2Y, 26, 26);
    viewModeToggleBtn.setBounds(pageRight - 26 - 90 - 26 - 6 - 58, row2Y, 58, 26);
    darkModeToggleBtn.setBounds(pageRight - 26 - 90 - 26 - 6 - 58 - 6 - 72, row2Y, 72, 26);
}

void NotebookHeaderComponent::showThemeAccentMenu()
{
    if (document == nullptr) return;

    juce::PopupMenu menu;
    menu.addSectionHeader("Theme Accent Color");

    struct AccentPreset {
        const char* name;
        uint32_t colour;
    };
    static const AccentPreset presets[] = {
        { "Amber Copper (Default)", 0xFFD97706 },
        { "Electric Blue",          0xFF2563EB },
        { "Emerald Green",          0xFF059669 },
        { "Crimson Rose",           0xFFE11D48 },
        { "Vivid Purple",           0xFF7C3AED },
        { "Golden Sun",             0xFFF59E0B },
        { "Cyan Teal",              0xFF0D9488 }
    };

    auto curAccent = NotebookLookAndFeel::getAccentColour();

    for (int i = 0; i < 7; ++i)
    {
        bool isSelected = (curAccent.getARGB() == presets[i].colour);
        menu.addItem(i + 1, presets[i].name, true, isSelected);
    }

    menu.addSeparator();
    menu.addItem(8, "Custom Color...", true);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&themeAccentBtn), [this](int result)
    {
        if (result >= 1 && result <= 7)
        {
            static const uint32_t colours[] = {
                0xFFD97706, 0xFF2563EB, 0xFF059669, 0xFFE11D48, 0xFF7C3AED, 0xFFF59E0B, 0xFF0D9488
            };
            juce::Colour chosen(colours[result - 1]);
            NotebookLookAndFeel::setAccentColour(chosen);
            if (document != nullptr)
            {
                document->setThemeTupletAccent(chosen);
                document->notifyChanged();
            }
            updateEditorColours();
            repaint();
            if (auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>())
                editor->repaint();
        }
        else if (result == 8)
        {
            auto* selector = new juce::ColourSelector(
                juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders | juce::ColourSelector::showColourspace);
            selector->setCurrentColour(NotebookLookAndFeel::getAccentColour());
            selector->setSize(300, 260);
            selector->addChangeListener(this);

            juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(selector),
                                                   themeAccentBtn.getScreenBounds(), nullptr);
        }
    });
}

void NotebookHeaderComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (auto* cs = dynamic_cast<juce::ColourSelector*>(source))
    {
        auto col = cs->getCurrentColour();
        NotebookLookAndFeel::setAccentColour(col);
        if (document != nullptr)
        {
            document->setThemeTupletAccent(col);
            document->notifyChanged();
        }
        updateEditorColours();
        repaint();
        if (auto* editor = findParentComponentOfClass<juce::AudioProcessorEditor>())
            editor->repaint();
    }
}

} // namespace CompassCadence
