#include "PluginEditor.h"

namespace CompassCadence
{

CompassCadenceAudioProcessorEditor::CompassCadenceAudioProcessorEditor(CompassCadenceAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
      header(p, p.getLyricDocument()),
      tabBar(p),
      pageView(p, p.getLyricDocument())
{
    setLookAndFeel(&lookAndFeel);

    addAndMakeVisible(header);
    addAndMakeVisible(tabBar);
    addAndMakeVisible(pageView);

    header.onScrollByBars = [this](int bars) {
        pageView.scrollByBars(bars);
        pageView.checkInfiniteScroll();
    };

    audioProcessor.onActiveTabChanged = [this](int) {
        tabBar.refreshTabs();
        header.setDocument(audioProcessor.getLyricDocument());
        pageView.setDocument(audioProcessor.getLyricDocument());
        repaint();
    };

    audioProcessor.onTabsChanged = [this] {
        tabBar.refreshTabs();
        header.setDocument(audioProcessor.getLyricDocument());
        pageView.setDocument(audioProcessor.getLyricDocument());
        repaint();
    };

#if defined(__EMSCRIPTEN__) || defined(JUCE_WASM) || defined(COMPASS_CADENCE_BUILD_WEB)
    setResizable(true, false);
    constrainer.setMinimumSize(320, 320);
    constrainer.setMaximumSize(3840, 2160);
    setConstrainer(&constrainer);
#else
    setResizable(true, true);
    constrainer.setMinimumSize(780, 500);
    constrainer.setMaximumSize(2560, 1440);
    setConstrainer(&constrainer);

    resizer = std::make_unique<juce::ResizableCornerComponent>(this, &constrainer);
    addAndMakeVisible(resizer.get());
#endif

    setSize(1040, 720);

    // 60 Hz timer for ultra-smooth playhead animation
    startTimerHz(60);
}

CompassCadenceAudioProcessorEditor::~CompassCadenceAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.onActiveTabChanged = nullptr;
    audioProcessor.onTabsChanged = nullptr;
    setLookAndFeel(nullptr);
}

void CompassCadenceAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(NotebookLookAndFeel::getPaperColour());
}

void CompassCadenceAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    header.setBounds(area.removeFromTop(74));
    tabBar.setBounds(area.removeFromTop(28));
    pageView.setBounds(area);

    if (resizer != nullptr)
        resizer->setBounds(getWidth() - 16, getHeight() - 16, 16, 16);
}

bool CompassCadenceAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    auto& doc = audioProcessor.getLyricDocument();

    // Ctrl+Z: Undo
    if (key == juce::KeyPress('z', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('z', juce::ModifierKeys::commandModifier, 0))
    {
        doc.undo();
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
        doc.redo();
        return true;
    }

    // Ctrl+J: Join
    if (key == juce::KeyPress('j', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('j', juce::ModifierKeys::commandModifier, 0))
    {
        doc.joinSelected();
        return true;
    }

    // Ctrl+C: Copy selected / highlighted text to system clipboard
    if (key == juce::KeyPress('c', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('c', juce::ModifierKeys::commandModifier, 0))
    {
        juce::String text = doc.getSelectedText();
        if (text.isEmpty())
            text = doc.getAllText();
        if (text.isNotEmpty())
            juce::SystemClipboard::copyTextToClipboard(text);
        return true;
    }

    // Ctrl+Left / Cmd+Left: Align Left
    if (key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::leftKey, juce::ModifierKeys::commandModifier, 0))
    {
        doc.setSelectionAlignment(LyricDocument::AlignLeft);
        return true;
    }

    // Ctrl+Right / Cmd+Right: Align Right
    if (key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::rightKey, juce::ModifierKeys::commandModifier, 0))
    {
        doc.setSelectionAlignment(LyricDocument::AlignRight);
        return true;
    }

    // Ctrl+Up / Ctrl+Down: Align Center
    if (key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::upKey, juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress(juce::KeyPress::downKey, juce::ModifierKeys::commandModifier, 0))
    {
        doc.setSelectionAlignment(LyricDocument::AlignCenter);
        return true;
    }

    // Ctrl+K / Ctrl+Shift+S: Split
    if (key == juce::KeyPress('k', juce::ModifierKeys::ctrlModifier, 0) ||
        key == juce::KeyPress('k', juce::ModifierKeys::commandModifier, 0) ||
        key == juce::KeyPress('s', juce::ModifierKeys::ctrlModifier | juce::ModifierKeys::shiftModifier, 0) ||
        key == juce::KeyPress('s', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier, 0))
    {
        doc.splitSelected();
        return true;
    }

    // Delete or Backspace: Clear selection
    if (key.isKeyCode(juce::KeyPress::deleteKey) || key.isKeyCode(juce::KeyPress::backspaceKey))
    {
        doc.deleteSelected();
        return true;
    }

    // Spacebar: Toggle standalone playback when running as .exe
    if (key.isKeyCode(juce::KeyPress::spaceKey) && audioProcessor.isStandalone())
    {
        audioProcessor.toggleStandalonePlayback();
        return true;
    }

    return false;
}

void CompassCadenceAudioProcessorEditor::timerCallback()
{
    auto transport = audioProcessor.getTransportState();
    header.updateDAWStatus(transport);

    // Check host automated page parameter
    if (auto* pageParam = audioProcessor.getAPVTS().getRawParameterValue("page"))
    {
        int p = (int)*pageParam;
        if (p != audioProcessor.getLyricDocument().getCurrentPage() &&
            p >= 0 && p < audioProcessor.getLyricDocument().getTotalPages())
        {
            audioProcessor.getLyricDocument().setCurrentPage(p);
        }
    }

    // Check host automated viewMode parameter
    if (auto* vmParam = audioProcessor.getAPVTS().getRawParameterValue("viewMode"))
    {
        int modeIdx = (int)*vmParam;
        auto expectedMode = (modeIdx == 1) ? LyricDocument::ModePages : LyricDocument::ModeScroll;
        if (audioProcessor.getLyricDocument().getViewMode() != expectedMode)
        {
            audioProcessor.getLyricDocument().setViewMode(expectedMode);
        }
    }

    // Check host automated darkMode parameter
    if (auto* darkParam = audioProcessor.getAPVTS().getRawParameterValue("darkMode"))
    {
        bool isDark = *darkParam > 0.5f;
        if (isDark != audioProcessor.getLyricDocument().isDarkMode())
        {
            audioProcessor.getLyricDocument().setDarkMode(isDark);
            header.updateDarkModeDisplay();
            tabBar.repaint();
            pageView.repaint();
            repaint();
        }
    }

    bool followDAW = audioProcessor.isFollowDAW();

    // Calculate exact playhead syllable location per bar
    auto& doc = audioProcessor.getLyricDocument();
    const double bpb = (double)doc.getNotation().getBeatsPerBar();
    int bar = (int)std::floor(transport.ppqPosition / std::max(1.0, bpb));
    if (bar < 0) bar = 0;
    double beatInBar = transport.ppqPosition - ((double)bar * bpb);
    if (beatInBar < 0.0) beatInBar = 0.0;
    auto location = doc.getNotation(bar).calculateLocationInBar(bar, beatInBar, bpb);

    pageView.updatePlayhead(location, transport.isPlaying, followDAW);
}

} // namespace CompassCadence
