# Implementation Plan — Scroll / Pages Infinite Vertical Scroll Toggle

Add an interactive **Scroll / Pages View Mode toggle** that allows users to smoothly scroll through bars continuously down an infinite vertical notebook canvas, completely eliminating page swapping when in Scroll mode while preserving the 16-bar notebook pagination when in Pages mode.

## User Review Required

> [!IMPORTANT]
> - **Default Mode**: When opening Compass Cadence, which mode should be the default? We propose defaulting to **Scroll Mode** (continuous vertical flow) based on your feedback, with the toggle allowing quick switching to Pages mode at any time.
> - **Infinite Auto-Expansion**: As you scroll near the bottom of the verse in Scroll Mode (or type past the bottom line), the document seamlessly appends another 16-bar chunk, giving you an infinite, uninterrupted writing canvas.
> - **Zero Glow / Zero Gradients**: The new toggle button matches the stationery look (pencil graphite border, cream tab tone, soft yellow highlighter wash when active).

## Proposed Changes

### Model Layer

#### [MODIFY] [LyricDocument.h](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/Model/LyricDocument.h)
#### [MODIFY] [LyricDocument.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/Model/LyricDocument.cpp)
- Add `enum ViewMode { ModeScroll, ModePages };`.
- Add `ViewMode getViewMode() const noexcept;` and `void setViewMode(ViewMode mode);`.
- In `ModeScroll`:
  - `getVisibleStartBar()` returns `0`.
  - `getVisibleEndBar()` returns `totalBars` (e.g. 64 bars initially, dynamically expanding).
- In `ModePages`:
  - `getVisibleStartBar()` returns `currentPage * barsPerPage`.
  - `getVisibleEndBar()` returns `(currentPage + 1) * barsPerPage`.
- Add `ensureScrollCapacity(int currentScrollY, int viewportHeight, int totalContentHeight)` to automatically append new 16-bar sections as the user scrolls near the bottom (infinite vertical expansion).
- Serialize `viewMode` into ValueTree project state and undo snapshots.

---

### UI Layer

#### [MODIFY] [NotebookPageView.h](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/UI/NotebookPageView.h)
#### [MODIFY] [NotebookPageView.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/UI/NotebookPageView.cpp)
- In `NotebookPageContent::rebuildBars()`:
  - If in `ModeScroll`: instantiates bar lines `0` to `totalBars` continuously on a single vertically scrolling canvas.
  - If in `ModePages`: instantiates bar lines `getFirstBarOfCurrentPage()` to `getLastBarOfCurrentPage()`.
- In `NotebookPageView`:
  - Listen to viewport scroll position: when scrolling within 120px of the bottom in Scroll mode, automatically invoke `document.ensureBarCount(totalBars + 16)` to append another page of bars seamlessly without interrupting typing or scrolling.
  - In `updatePlayhead(location, isPlaying, followDAW)`:
    - In `ModeScroll`: `scrollToBar(location.bar)` smoothly glides the viewport to keep the active DAW bar centered on screen at 60 Hz without any page flips.
    - In `ModePages`: switches pages when the bar moves outside the current 16-bar page during playback as before.

#### [MODIFY] [NotebookHeaderComponent.h](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/UI/NotebookHeaderComponent.h)
#### [MODIFY] [NotebookHeaderComponent.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/UI/NotebookHeaderComponent.cpp)
- Add `juce::TextButton viewModeToggleBtn;` in Row 2 of the header next to the page controls.
- Displays `"Scroll"` or `"Pages"`.
- Clicking the button toggles between continuous infinite scrolling and 16-bar page views.
- In `ModeScroll`:
  - `pageLabel` displays the current bar range (e.g. `Bars 1–64+` or `Scroll Mode`).
  - `<` and `>` buttons function as 16-bar leap jumps (Page Up / Page Down).
- In `ModePages`:
  - `pageLabel` displays `Page X / Y`, and `<` / `>` flip between 16-bar pages.

#### [MODIFY] [PluginProcessor.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/PluginProcessor.cpp)
- Add `"viewMode"` choice parameter (`"Scroll"`, `"Pages"`) to APVTS for full host automation and project recall.

---

### Verification Plan

#### Automated Tests
- Add Test `[10.11] Testing Scroll Mode vs. Pages Mode & Infinite Vertical Expansion` to `test_runner.cpp`:
  1. Toggle view mode to `ModeScroll`.
  2. Verify bar range spans continuous bars (`startBar = 0`, `endBar >= 64`).
  3. Verify infinite expansion when reaching the bottom boundary (`ensureBarCount`).
  4. Toggle view mode to `ModePages`.
  5. Verify 16-bar pagination boundaries.
  6. ValueTree serialization roundtrip of `viewMode`.

#### Manual Verification
- Verify mouse wheel and scrollbar scrolling vertically through dozens of bars continuously.
- Verify follow DAW smoothly autoscrolls the viewport to track playback.
- Verify switching back and forth between Scroll and Pages mode is instantaneous and seamless.
