# Compass Cadence — Release Update & Walkthrough

## Summary of Completed Enhancements

### 1. Syllable Right-Click Context Menu & Custom Color Highlight Selection
- **Phonetic Sound & Syllable Header**: The top section header of the right-click menu identifies the clicked syllable and the phonetically detected rhyme group (e.g. `Syllable: "rhymes" [AY1_M_Z]`), giving instant phonetic insight into the engine's rhyme detection.
- **Custom Color Override**: Allows assigning a custom highlight color to replace the automatic phoneme rhyme tint on a per-syllable basis:
  - `Auto (Phoneme Rhyme Tint)`: Restores the automatic dynamic phoneme classification.
  - **7 Stationery Pastel Swatches**: Specially balanced pastel highlighter tones with ~60% alpha for effortless text readability in both Light and Dark modes:
    - Pastel Mint (`#86EFAC`)
    - Pastel Sky (`#93C5FD`)
    - Pastel Peach (`#FED7AA`)
    - Pastel Lavender (`#DDD6FE`)
    - Pastel Butter (`#FEF08A`)
    - Pastel Rose (`#FECDD3`)
    - Slate Gray (`#CBD5E1`)
  - `Clear Highlight (None)`: Explicitly silences highlighting on that cell even if phoneme rhymes exist.
  - `Custom Color...`: Opens a native, interactive `juce::ColourSelector` inside a floating `juce::CallOutBox` with live preview and automatic dismissal when clicking outside.
- **Multi-Cell Batch Highlighting**: When multiple cells are selected via mouse drag or shift/ctrl click, applying a custom color, resetting to Auto, or clearing highlight applies across the entire selection simultaneously.
- **Full Undo / Redo & DAW State Persistence**: Custom cell colors are tracked in snapshot history (`Ctrl+Z` / `Ctrl+Y`) and saved in DAW session states via `ValueTree` serialization (`customColor`).

---

### 2. Context Menu Songwriter Utilities
Right-clicking any syllable box (or active inline text editor) provides quick access to flow and text operations:
- **Text Casing Transforms**:
  - `UPPERCASE`: Converts syllable(s) to all caps (great for chorus hooks, shouts, or adlibs).
  - `lowercase`: Converts syllable(s) to lowercase.
  - `Capitalize Word`: Capitalizes the first letter of the word/syllable.
  - Supports multi-cell batch transformations.
- **Flow Duplication**:
  - `Duplicate into Next Box`: Copies the syllable text, bold accent, alignment, and custom color into the immediate next syllable box. Invaluable for writing rhythmic echoes, vocal stutters, or repeated syllables without retyping.
- **Text Alignment**:
  - `Align Left (←)`
  - `Align Center / Down (↓)`
  - `Align Right (→)`
  - Also accessible via cell hover arrow buttons (`←`, `↓`, `→`).
- **Formatting**:
  - `Bold Emphasis (Ctrl+B)`: Toggles stress notation.
- **Universal Windows Clipboard**:
  - `Cut (Ctrl+X)`: Copies text to Windows clipboard and clears cell(s).
  - `Copy (Ctrl+C)`: Copies selected text to Windows clipboard for pasting externally (e.g. into Notepad, DAW notes, Discord, browser).
  - `Paste (Ctrl+V)`: Pastes from Windows clipboard, with multi-syllable and multi-word auto-spread across boxes.
- **Structural Cell Operations**:
  - `Join Cells (Ctrl+J)`: Combines multi-syllable phrases.
  - `Split Syllables (Ctrl+K)`: Splits multi-syllable words across consecutive boxes.
  - `Clear Cell (Del)`: Clears syllable text.

---

### 3. Active Inline Typing Right-Click Support
- **Seamless Typing Flow**: Right-clicking while actively typing inside a syllable text box (`SyllableInlineEditor`) now opens this rich custom context menu rather than JUCE's default generic text editor menu.
- **Automatic Commit**: Before opening the menu, any in-progress typed characters are automatically committed so no words are lost.

---

### 4. Cell Hover Alignment Controls: Corner Arrows (`←`, `→`) & Bottom Underline
- **Hover-Only Visibility**:
  - Uses `addChildComponent` with initial `setVisible(false)` and `!isMouseOver(true)` guards in `updateContent()` and `mouseExit()`. Alignment controls stay completely invisible on load and only show on hover for that specific cell.
- **Removed Middle Down Arrow & Unblocked Text Editing**:
  - Removed the bulky middle down arrow button that used to sit in the center of the box.
  - Replaced it with a 3.5px thick rounded underline bar (`AlignUnderlineButton`) positioned at the bottom edge (`y = cellH - 5`) spanning beneath the word.
  - Clicking the underline centers the word, while the entire middle 85% of the cell remains completely open so clicking in the middle directly focuses and edits text.
- **Vibrant Copper Amber Palette (Zero Gray)**:
  - Both the corner arrows (`←`, `→`) and the bottom underline are rendered in rich copper amber (`#D97706`), brightening to `#F59E0B` on hover and `#B45309` on click, eliminating the previous gray tones.
  - The currently active alignment illuminates solidly with active styling.

---

### 5. Add New Line Button ("+") Hover Visibility & Bottom-Right Placement
- **Initial Load Visibility Bug Resolved**:
  - Replaced `addAndMakeVisible(addLineBtn)` with `addChildComponent(addLineBtn); addLineBtn.setVisible(false);` in [BarLineComponent.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/UI/BarLineComponent.cpp).
  - Added strict `if (!isMouseOver(true)) addLineBtn.setVisible(false);` guards in `mouseExit()`, `updateContent()`, and `rebuildPulses()`.
  - Added `mouseMove()` handler to ensure the button reveals responsively upon hovering anywhere within the bar line.
- **Tucked into Bottom-Right Corner of Line (Separated from Syllable Counter)**:
  - Removed `addLineBtn` from sitting in-line with the syllable count controls (`[-] [16/16] [+]`).
  - Moved `addLineBtn` to the absolute bottom-right corner of each bar line (`x = width - 19, y = height - 17, w = 16, h = 15`), resting discreetly right above the bottom ruling line.
  - Preserved natural layout for syllable counter controls (`counterW = 88, counterMarginRight = 24`), keeping `decSylBtn`, `sylCountLabel`, and `incSylBtn` vertically centered at `y = 8` without any horizontal or vertical overlap.

---

### 6. Middle Alignment Underline Button Clickability & Responsiveness
- **Expanded Hit Target & Click Zone**:
  - Sized `alignCenterBtn` with a generous 10–13px interactive height (`underlineH = std::clamp(cellH / 2, 10, 13)`), spanning cleanly between the left and right corner arrow buttons with no overlap.
  - Inside `AlignUnderlineButton::paintButton`, the visual underline remains a sleek 3.5px thick rounded bar resting at the bottom edge with a subtle amber hover glow across the button target area.
- **Immediate Mouse-Down Triggering**:
  - Enabled `setTriggeredOnMouseDown(true)` on `AlignUnderlineButton` (and `AlignArrowButton`), ensuring clicks register the instant the mouse button goes down without waiting for release.
  - Overrode `hitTest()` with generous hit padding.
- **Direct Cell Mouse Fallback**:
  - Added direct button bounds checking in [SyllableCellComponent.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/UI/SyllableCellComponent.cpp)`::mouseDown()` so clicks on the alignment area immediately trigger `triggerClick()` and return.

---

### 7. WebAssembly (WASM) & Progressive Web App (PWA) Target

#### Architectural Overview
We integrated a WebAssembly build target alongside our existing Windows VST3 and Standalone targets without disturbing or modifying desktop builds:
- **CMake Decoupling**: In [CMakeLists.txt](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/CMakeLists.txt), the shared C++ source list is captured in `COMPASS_CADENCE_SOURCES`. When compiling for Emscripten (`EMSCRIPTEN` or `COMPASS_CADENCE_BUILD_WEB`), it builds `CompassCadenceWeb` as an executable target with dedicated Emscripten compiler and linker flags.
- **WASM Linker Settings**:
  - `-s WASM=1`
  - `-s USE_PTHREADS=0`: Avoids requiring `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp` HTTP response headers, ensuring the app runs immediately on GitHub Pages and standard static web hosts.
  - `-s ALLOW_MEMORY_GROWTH=1`: Starts at 64MB and dynamically scales up to 512MB as large song projects or long lyrics are loaded.
  - `-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','FS','IDBFS']` & `-lidbfs.js`: Enables persistent browser storage via IndexedDB.
  - `--shell-file Web/shell.html`: Injects our custom high-DPI HTML5 template.
- **Audio & Standalone Playhead Decoupling**:
  - On WASM, DAW host integration is unavailable (`getAudioPlayHead()` is null).
  - In [Source/PluginProcessor.h](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/PluginProcessor.h), `isStandalone()` evaluates to `true` when compiling for Web. This automatically reuses our sample-accurate standalone metronome click synthesizer, tempo stepper, and play/stop transport engine.
- **Persistent Storage in Browser (`IDBFS`)**:
  - In [Source/MainWeb.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/MainWeb.cpp), the Emscripten virtual filesystem mounts `IDBFS` at `/CompassCadence`.
  - On launch, `FS.syncfs(true, ...)` reads any saved songs and custom presets from IndexedDB.
  - Custom flow presets (`PresetManager`) and song files (`SongManager`) persist across browser reloads, private tabs, and mobile restarts.
- **Fluid & Responsive UI Layout**:
  - [Source/MainWeb.cpp](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Source/MainWeb.cpp) hooks browser window resize events (`window.innerWidth`, `window.innerHeight`) and dynamically resizes the JUCE window to fit any mobile or desktop screen.
  - Desktop-only corner resizers are disabled on Web (`#if !defined(__EMSCRIPTEN__)`).
- **Progressive Web App (PWA) Capabilities**:
  - [Web/manifest.json](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Web/manifest.json): Enables full "Add to Home Screen" standalone app mode on iOS Safari and Android Chrome with matching warm stationery theme colors (`#F5F2EB` background, `#D97706` theme color).
  - [Web/sw.js](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Web/sw.js): Lightweight Service Worker caches WASM, JS, and HTML assets for instant loading and offline capability.
  - [Web/shell.html](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Web/shell.html): Includes Web Audio gesture auto-unlocking so audio playback starts cleanly without browser autoplay policy blocks.

---

### 8. GitHub Actions CI/CD Workflow & GitHub Pages Deployment Guide

#### Automated Workflow (`.github/workflows/deploy-pages.yml`)
We added an automated CI/CD pipeline that builds the WebAssembly target and publishes it to GitHub Pages:
- **Triggers**: Runs automatically on every `push` to `master` or `main`, and can also be triggered manually via GitHub's `workflow_dispatch` ("Run workflow" button).
- **Environment**: Runs on `ubuntu-latest`, checks out submodules, installs Ninja, and provisions the Emscripten toolchain using `mymindstorm/setup-emsdk@v14`.
- **Build Step**:
  ```bash
  emcmake cmake -B build-web -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCOMPASS_CADENCE_BUILD_WEB=ON
  cmake --build build-web --config Release --target CompassCadenceWeb
  ```
- **Staging & Packaging**: Packages `CompassCadenceWeb.html` as `index.html`, along with `CompassCadenceWeb.js`, `CompassCadenceWeb.wasm`, `manifest.json`, and `sw.js`.
- **Deployment**: Uses official `actions/upload-pages-artifact@v3` and `actions/deploy-pages@v4` to publish the site with zero server maintenance.

#### How to Enable GitHub Pages on Your Repository (One-Time Setup)
To make your WebAssembly PWA accessible to anyone on the web or mobile:
1. **Push your code to GitHub**:
   ```bash
   git add .
   git commit -m "Add WebAssembly PWA build target and GitHub Pages CI/CD"
   git push origin master
   ```
2. **Open your GitHub repository in your browser**.
3. Go to **Settings** (top navigation tab).
4. In the left sidebar under the **Code and automation** section, click **Pages**.
5. Under **Build and deployment**:
   - **Source**: Select **GitHub Actions** (instead of "Deploy from a branch").
6. The GitHub Actions workflow will automatically run on push (or you can trigger it under the **Actions** tab by selecting **Deploy Compass Cadence Web to GitHub Pages** > **Run workflow**).
7. Once finished (typically 2-3 minutes), GitHub Pages will display your live URL:
   `https://<your-username>.github.io/<repo-name>/`
8. Open the URL on your phone or desktop:
   - On **iPhone / iPad (Safari)**: Tap **Share** > **Add to Home Screen** to install as a full-screen standalone app.
   - On **Android (Chrome)**: Tap the menu (three dots) > **Install app** / **Add to Home screen**.
   - On **Desktop (Chrome/Edge)**: Click the install icon in the URL bar to run Compass Cadence as a desktop web app.

---

## Verification Results

### Automated Unit Tests (`TestPlugin.exe`)
All 24 test suites passed with exit code 0:
```
[1] Juce GUI initialized.
[2] Creating CompassCadenceAudioProcessor...
[3] Processor created successfully: Compass Cadence
[4] Preparing to play (44100, 512)...
[5] Creating Editor...
[6] Editor created successfully! Bounds: 0 0 1040 720
[7] Simulating timerCallback...
[8] timerCallback succeeded!
[9] Simulating paint...
  [9.1] Painting editor base... Passed!
  [9.2] Painting child 0 (class CompassCadence::NotebookHeaderComponent)... Passed child 0!
  [9.2] Painting child 1 (class CompassCadence::NotebookTabBarComponent)... Passed child 1!
  [9.2] Painting child 2 (class CompassCadence::NotebookPageView)... Passed child 2!
  [9.2] Painting child 3 (class juce::ResizableCornerComponent)... Passed child 3!
  [9.2] Painting child 4 (class CompassCadence::NonTextTooltipWindow)... Passed child 4!
  [9.2] Painting child 5 (class juce::ResizableCornerComponent)... Passed child 5!
[10] All paints succeeded!
[10.1] Testing Syllable Auto-Splitting... Passed!
[10.2] Testing Undo / Redo... Passed!
[10.3] Testing Multi-cell selection & Join... Passed!
[10.4] Testing Rhyme Key Determinism & Hyphen filter... Passed!
[10.5] Testing Per-Bar Metric Notation & Mixed Polymeter... Passed!
[10.6] Testing Syllable Splitter -es and -ed rules... Passed!
[10.7] Testing Bold Emphasis Notation... Passed!
[10.8] Testing PresetManager... Passed!
[10.9] Testing Syllable Counter & Actual Spoken Count Customization... Passed!
[10.10] Testing Page Navigation & Display Shifting... Passed!
[10.11] Testing Scroll Mode vs. Pages Mode & Infinite Vertical Expansion... Passed!
[10.12] Testing Clipboard Copy and Tooltip Suppression... Passed!
[10.13] Testing Standalone Metronome Synthesis & Transport... Passed!
[10.14] Testing Cell Text Alignment & Auto-Splitting Alignment... Passed!
[10.15] Testing CMUDict Rhyme Engine & Non-Colliding Color Assignment... Passed!
[10.16] Testing Full Export... Passed!
[10.17] Testing SongManager... Passed!
[10.18] Testing Dark Mode & Variable Shading Colors... Passed!
[10.19] Multi-Tab layout & unsynced duplicates... Passed!
[10.20] Custom Stanza Breaks... Passed!
[10.21] Dynamic line insertion (insertBar)... Passed!
[10.22] Syllable cell alignment (Left, Down/Center, Right)... Passed!
[10.23] Immediate Theme / Dark Mode sync... Passed!
### 8. 3-Mode Rhyme Toggle & Syllable Repetition Detector (2+ Syllables)
- **3-Mode Toggle System**:
  - `Rhymes: ON`: Phonetic vowel sound and slant rhyme detector (15 basic vowel families with CMUDict and surround-context classification).
  - `Repeats: 2+`: Repetition detector highlighting exact matching syllable sequences of length $\ge 2$ across the document.
  - `Colors: OFF`: Silences all automated highlights (custom manual colors remain untouched).
  - Left-clicking cycles: `Rhymes: ON` $\rightarrow$ `Repeats: 2+` $\rightarrow$ `Colors: OFF` $\rightarrow$ `Rhymes: ON`.
  - Right-clicking opens a context menu allowing direct selection of any mode with checkmark indicators.
- **Maximal Sequence Matching Algorithm**:
  - Normalizes syllable text (lowercasing and stripping punctuation so commas, hyphens, exclamation marks don't hinder matching).
  - Discards non-maximal left extensions so identical substrings belong to one maximal sequence.
  - Enforces minimum match length $L \ge 2$: isolated repeated syllables (e.g. "the", "a", "I") are left uncolored.
  - Distinct highlighter colors from the 12-color palette are assigned per unique phrase key, ensuring matching phrases visually cluster together across verses.
- **1:1 Alignment Across Engines**:
  - Implemented in JUCE C++ (`RhymeClassifier.h/cpp`, `LyricDocument.h/cpp`, `SyllableCellComponent.cpp`, `NotebookHeaderComponent.h/cpp`) for VST3 and Standalone.
  - Implemented in Vanilla JS (`Web/app.js`) for the Web & PWA builds.
  - Fully integrated into ValueTree serialization for DAW project save/restore.

---

### Test Suite Output
```
[1] Juce GUI initialized.
[2] Creating CompassCadenceAudioProcessor...
[3] Processor created successfully: Compass Cadence
[4] Preparing to play (44100, 512)...
[5] Creating Editor...
[6] Editor created successfully! Bounds: 0 0 1040 720
[7] Simulating timerCallback...
[8] timerCallback succeeded!
[9] Simulating paint...
  [9.1] Painting editor base... Passed!
  [9.2] Painting child 0 (class CompassCadence::NotebookHeaderComponent)... Passed child 0!
  [9.2] Painting child 1 (class CompassCadence::NotebookTabBarComponent)... Passed child 1!
  [9.2] Painting child 2 (class CompassCadence::NotebookPageView)... Passed child 2!
  [9.2] Painting child 3 (class juce::ResizableCornerComponent)... Passed child 3!
  [9.2] Painting child 4 (class CompassCadence::NonTextTooltipWindow)... Passed child 4!
  [9.2] Painting child 5 (class juce::ResizableCornerComponent)... Passed child 5!
[10] All paints succeeded!
[10.1] Testing Syllable Auto-Splitting... Passed!
[10.2] Testing Undo / Redo... Passed!
[10.3] Testing Multi-cell selection & Join... Passed!
[10.4] Testing Rhyme Key Determinism & Hyphen filter... Passed!
[10.5] Testing Per-Bar Metric Notation & Mixed Polymeter... Passed!
[10.6] Testing Syllable Splitter -es and -ed rules... Passed!
[10.7] Testing Bold Emphasis Notation... Passed!
[10.8] Testing PresetManager... Passed!
[10.9] Testing Syllable Counter & Actual Spoken Count Customization... Passed!
[10.10] Testing Page Navigation & Display Shifting... Passed!
[10.11] Testing Scroll Mode vs. Pages Mode & Infinite Vertical Expansion... Passed!
[10.12] Testing Clipboard Copy and Tooltip Suppression... Passed!
[10.13] Testing Standalone Metronome Synthesis & Transport... Passed!
[10.14] Testing Cell Text Alignment & Auto-Splitting Alignment... Passed!
[10.15] Testing CMUDict Rhyme Engine & Non-Colliding Color Assignment... Passed!
[10.16] Testing Full Export... Passed!
[10.17] Testing SongManager... Passed!
[10.18] Testing Dark Mode & Variable Shading Colors... Passed!
[10.19] Multi-Tab layout & unsynced duplicates... Passed!
[10.20] Custom Stanza Breaks... Passed!
[10.21] Dynamic line insertion (insertBar)... Passed!
[10.22] Syllable cell alignment (Left, Down/Center, Right)... Passed!
[10.23] Immediate Theme / Dark Mode sync... Passed!
[10.24] Syllable right-click utilities (custom color, case transforms, duplicate)... Passed!
[10.25] Bar Height, Meter Truncation, Context Vowels, and Palette Customization... Passed!
[10.26] 3-Mode Rhyme Toggle & Syllable Repetition Detector (2+ Syllables)... Passed!
[11] Clean teardown succeeded!
[12] ALL TESTS PASSED SAFELY!
```

### Desktop VST3 & Standalone Status
- Built cleanly in **Release (x64)** mode with zero errors.
- Deployed updated `compass4cadence.vst3` directly to:
  `%LOCALAPPDATA%\Programs\Common\VST3\compass4cadence.vst3`
- Verified **zero regressions** on Windows desktop targets.

---

### 8. True Time-Proportional Flex Mapping, Tuplet Brackets, Stress Borders, Variable Padding & Static DAW Grid Lines

#### 1. True Time-Proportional Flex Mapping (Tier 1 Macro-Pulse Allocation)
- **Decoupled Phonetic Density from Musical Time**: Deprecated uniform `1/numPulses` division. Pulse widths are derived strictly from musical temporal durations in beats (`beatWeight` / `pulseDuration`).
- **Equal Time Proportions for Equal Beats**: A triplet and duplet across two beats (`[32]/2:4`) each occupy exactly 1 quarter note of DAW time and physically render at 50% / 50% width on screen.
- **Asymmetric Additive Meter Parsing**: Supports explicit temporal tags `<d1,d2,...>` or `<d1+d2+...>` in notation strings (e.g. `[332]<1.5,1.5,1.0>:4` or `[332]<3+3+2>:4`), normalizing and physically scaling pulse boxes to exact mathematical percentages ($3/8 = 37.5\%$, $3/8 = 37.5\%$, $2/8 = 25\%$).
- **Playhead Mapping**: DAW playhead position interpolates along cumulative pulse durations rather than uniform steps.

#### 2. Customizable Tuplet Brackets
- **Bracket & Ticks Rendering**: Horizontal line spanning each pulse group box with downward vertical ticks on the left and right edges.
- **Center Numeral**: Displays the subdivision count (`numCells`) centered within a clean gap in the bracket line.
- **Line & Text Styling**: 1.5px stroke and numeral color bound to `themeTupletAccent` (`#D97706`).
- **Visibility Threshold**: When row height is compressed below 36px, tuplet numerals are hidden to prevent visual clutter; below 24px, brackets are omitted.
- **State Machine (3 Modes)**:
  - `ALL_ON`: Visible on all lines.
  - `ACTIVE_LINE`: Visible only on the active line currently being edited.
  - `ALL_OFF`: Hidden everywhere.
- **Full Menu Access**: Switchable via syllable context menu and header options in both JUCE and Web.

#### 3. Continuous Internal Perimeter Stress Borders
- **Perimeter Inset Rectangle**: The first syllable cell (`index 0`) of each pulse group box features a full continuous internal perimeter box (`box-shadow: inset 0 0 0 2.5px var(--copper-accent)` in Web CSS, `drawRoundedRectangle(textBounds.reduced(1.25f), 2.0f, 2.5f)` in JUCE).
- **Collision Avoidance**: Completely replaces left-only border styling, preventing visual collision with 1px DAW structural beat lines.

#### 4. Macro-Pulse Group Padding
- **10px Horizontal Separation**: Increased the physical gap between pulse group boxes from 8px to 10px in both JUCE (`BarLineComponent::resized`) and Web CSS (`.bar-pulses-grid { gap: 10px; }`).

#### 5. Static DAW Structural Beat Grid Lines
- **Structural Beat Guides**: Static 1px vertical guide lines rendered at exact physical screen positions corresponding to the DAW's structural beats ($1/B, 2/B, \dots$).
- **Layering & Color**: Drawn behind pulse group boxes using translucent theme-aware styling (`rgba(0, 0, 0, 0.06)` in light mode, `rgba(255, 255, 255, 0.04)` in dark mode).

#### 6. Cross-Platform Parity & Deployment
- **JUCE VST3 & Standalone EXE**: Successfully compiled in Release x64 mode with zero errors and deployed to `%LOCALAPPDATA%\Programs\Common\VST3\compass4cadence.vst3`.
- **Web PWA**: Full parity in [Web/style.css](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Web/style.css) and [Web/app.js](file:///c:/Users/jjntw/Desktop/MUSIC_boot/VST_customs/compass-cadence/Web/app.js); bumped Service Worker cache to `compass4cadence-v14`.
- **Git Repository**: Committed and pushed to `origin main`.

