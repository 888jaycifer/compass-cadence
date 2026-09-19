/**
 * Compass Cadence - Web PWA Engine
 * Complete 1:1 Implementation of JUCE UI & Logic in JavaScript
 */

(function () {
  // --- 1. Audio Engine (Web Audio API) ---
  class MetronomeEngine {
    constructor() {
      this.ctx = null;
      this.bpm = 120;
      this.isPlaying = false;
      this.clickEnabled = true;
      this.currentBar = 0;
      this.currentBeat = 0;
      this.nextNoteTime = 0.0;
      this.timerID = null;
      this.onBeatCallback = null;
    }

    initContext() {
      if (!this.ctx) {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        this.ctx = new AudioContext();
      }
      if (this.ctx.state === 'suspended') {
        this.ctx.resume();
      }
    }

    setBpm(newBpm) {
      this.bpm = Math.max(20, Math.min(300, newBpm));
    }

    togglePlay(onBeat) {
      this.initContext();
      this.onBeatCallback = onBeat;
      this.isPlaying = !this.isPlaying;
      if (this.isPlaying) {
        this.currentBar = 0;
        this.currentBeat = 0;
        this.nextNoteTime = this.ctx.currentTime + 0.05;
        this.scheduler();
      } else {
        clearTimeout(this.timerID);
      }
      return this.isPlaying;
    }

    scheduler() {
      if (!this.isPlaying) return;
      while (this.nextNoteTime < this.ctx.currentTime + 0.1) {
        this.scheduleNote(this.nextNoteTime, this.currentBar, this.currentBeat);
        this.advanceNote();
      }
      this.timerID = setTimeout(() => this.scheduler(), 25);
    }

    scheduleNote(time, bar, beat) {
      if (this.clickEnabled) {
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();
        osc.type = 'sine';
        // Downbeat click 1600 Hz, offbeat 1000 Hz
        osc.frequency.setValueAtTime(beat === 0 ? 1600 : 1000, time);
        gain.gain.setValueAtTime(0.7, time);
        gain.gain.exponentialRampToValueAtTime(0.0001, time + 0.035);
        osc.connect(gain);
        gain.connect(this.ctx.destination);
        osc.start(time);
        osc.stop(time + 0.035);
      }

      // Notify UI
      const delayMs = Math.max(0, (time - this.ctx.currentTime) * 1000);
      setTimeout(() => {
        if (this.isPlaying && this.onBeatCallback) {
          this.onBeatCallback(bar, beat);
        }
      }, delayMs);
    }

    advanceNote() {
      const secondsPerBeat = 60.0 / this.bpm;
      this.nextNoteTime += secondsPerBeat;
      this.currentBeat++;
      if (this.currentBeat >= 4) { // Default 4 beats per bar
        this.currentBeat = 0;
        this.currentBar++;
      }
    }
  }

  // --- 2. Syllable & Phoneme Engine ---
  class PhonemeEngine {
    static pastelPalette = [
      '#86EFAC', '#93C5FD', '#FED7AA', '#DDD6FE', '#FEF08A', '#FECDD3', '#CBD5E1',
      '#A7F3D0', '#BAE6FD', '#FDE68A', '#FBCFE8', '#C7D2FE', '#E9D5FF', '#D1D5DB'
    ];

    // English Syllable Splitter respecting -es and -ed rules
    static splitWord(word) {
      if (!word) return [];
      const clean = word.toLowerCase().replace(/[^a-z'-]/g, '');
      if (clean.length <= 3) return [word];

      // Sibilant check for -es endings (watches vs rhymes)
      const nonSibilantEs = /[^sckxzjg]es$/i;
      const isSilentEs = nonSibilantEs.test(clean) && !clean.endsWith('ches') && !clean.endsWith('shes');

      // Vowel clusters
      const vowels = /[aeiouy]+/gi;
      const matches = [...clean.matchAll(vowels)];
      if (matches.length <= 1) return [word];

      // Approximate 1-syllable silent endings
      if (isSilentEs && matches.length === 2 && clean.endsWith('es')) {
        return [word];
      }

      // Basic hyphen split
      if (clean.length >= 6) {
        const mid = Math.floor(word.length / 2);
        return [word.slice(0, mid) + '-', word.slice(mid)];
      }
      return [word];
    }

    static getRhymeKey(word) {
      if (!word) return '';
      const clean = word.toLowerCase().replace(/[^a-z]/g, '');
      if (clean.length < 2) return clean;
      // Phonetic vowel approximation
      if (/([aeiouy]+[^aeiouy]*)$/i.test(clean)) {
        return RegExp.$1;
      }
      return clean.slice(-2);
    }

    static getColorForRhyme(rhymeKey, activeRhymeKeys) {
      if (!rhymeKey) return null;
      let idx = activeRhymeKeys.indexOf(rhymeKey);
      if (idx === -1) {
        activeRhymeKeys.push(rhymeKey);
        idx = activeRhymeKeys.length - 1;
      }
      return this.pastelPalette[idx % this.pastelPalette.length];
    }
  }

  // --- 3. Notebook State ---
  class NotebookModel {
    constructor() {
      this.tabs = [
        { id: 'tab-1', title: 'Verse 1', bars: this.createDefaultBars(16) }
      ];
      this.activeTabId = 'tab-1';
      this.viewMode = 'scroll'; // 'scroll' or 'pages'
      this.currentPage = 0;
      this.darkMode = false;
      this.activeRhymeKeys = [];
      this.customPresets = {};
    }

    createDefaultBars(count) {
      const bars = [];
      for (let i = 0; i < count; i++) {
        bars.push({
          meter: [4, 4],
          pulses: 16,
          pulseGroups: [4, 4, 4, 4],
          syllables: Array(16).fill('').map(() => ({
            text: '',
            bold: false,
            align: 'center',
            customColor: null
          })),
          spokenSyllables: null, // null = auto
          isStanzaBreak: (i + 1) % 4 === 0
        });
      }
      return bars;
    }

    getActiveTab() {
      return this.tabs.find(t => t.id === this.activeTabId) || this.tabs[0];
    }
  }

  // --- 4. Main Application UI Controller ---
  class AppController {
    constructor() {
      this.model = new NotebookModel();
      this.metronome = new MetronomeEngine();
      this.selectedCell = null;
      this.contextCellTarget = null;
      this.initElements();
      this.loadSavedState();
      this.bindEvents();
      this.render();
    }

    initElements() {
      this.container = document.getElementById('app-container');
      this.sheet = document.getElementById('notebook-sheet');
      this.tabsBar = document.getElementById('tabs-bar');
      this.contextMenu = document.getElementById('context-menu');
      this.playBtn = document.getElementById('play-btn');
      this.clickBtn = document.getElementById('click-btn');
      this.tempoSpan = document.getElementById('tempo-span');
      this.viewModeBtn = document.getElementById('view-mode-btn');
      this.darkModeBtn = document.getElementById('dark-mode-btn');
      this.pageLabel = document.getElementById('page-label');
      this.copyBtn = document.getElementById('copy-btn');
    }

    bindEvents() {
      // Metronome
      this.playBtn.addEventListener('click', () => this.togglePlayback());
      this.clickBtn.addEventListener('click', () => {
        this.metronome.clickEnabled = !this.metronome.clickEnabled;
        this.clickBtn.textContent = this.metronome.clickEnabled ? 'Click: ON' : 'Click: OFF';
      });

      document.getElementById('bpm-dec').addEventListener('click', () => this.changeBpm(-1));
      document.getElementById('bpm-inc').addEventListener('click', () => this.changeBpm(1));

      // View & Theme
      this.viewModeBtn.addEventListener('click', () => {
        this.model.viewMode = this.model.viewMode === 'scroll' ? 'pages' : 'scroll';
        this.viewModeBtn.textContent = this.model.viewMode === 'scroll' ? 'Scroll' : 'Pages';
        this.render();
      });

      this.darkModeBtn.addEventListener('click', () => {
        this.model.darkMode = !this.model.darkMode;
        document.body.classList.toggle('dark-mode', this.model.darkMode);
        this.darkModeBtn.textContent = this.model.darkMode ? 'Dark: ON' : 'Dark: OFF';
        this.saveState();
      });

      this.copyBtn.addEventListener('click', () => this.copyLyricsToClipboard());

      // Global Context Menu Dismiss
      document.addEventListener('click', (e) => {
        if (!this.contextMenu.contains(e.target)) {
          this.contextMenu.style.display = 'none';
        }
      });

      // Keyboard Shortcuts
      document.addEventListener('keydown', (e) => {
        if (e.code === 'Space' && document.activeElement.tagName !== 'INPUT') {
          e.preventDefault();
          this.togglePlayback();
        }
        if ((e.ctrlKey || e.metaKey) && e.key === 'b' && this.selectedCell) {
          e.preventDefault();
          this.toggleCellBold(this.selectedCell);
        }
        if ((e.ctrlKey || e.metaKey) && e.key === 'c' && document.activeElement.tagName !== 'INPUT') {
          this.copyLyricsToClipboard();
        }
      });
    }

    changeBpm(delta) {
      this.metronome.setBpm(this.metronome.bpm + delta);
      this.tempoSpan.textContent = `${this.metronome.bpm} BPM`;
    }

    togglePlayback() {
      const playing = this.metronome.togglePlay((barIdx, beatIdx) => {
        this.updatePlayhead(barIdx);
      });
      this.playBtn.textContent = playing ? 'Stop' : 'Play';
      this.playBtn.classList.toggle('active', playing);
      if (!playing) {
        document.querySelectorAll('.playhead-marker').forEach(el => el.classList.remove('active'));
      }
    }

    updatePlayhead(barIdx) {
      document.querySelectorAll('.playhead-marker').forEach((el, idx) => {
        el.classList.toggle('active', idx === barIdx);
      });
      if (this.model.viewMode === 'scroll') {
        const row = document.getElementById(`bar-row-${barIdx}`);
        if (row) {
          row.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
      }
    }

    render() {
      this.renderTabs();
      this.renderBars();
      this.saveState();
    }

    renderTabs() {
      this.tabsBar.innerHTML = '';
      this.model.tabs.forEach(tab => {
        const tabEl = document.createElement('div');
        tabEl.className = `tab ${tab.id === this.model.activeTabId ? 'active' : ''}`;
        tabEl.innerHTML = `
          <span>${tab.title}</span>
          ${this.model.tabs.length > 1 ? '<span class="tab-close">&times;</span>' : ''}
        `;
        tabEl.addEventListener('click', (e) => {
          if (e.target.classList.contains('tab-close')) {
            this.closeTab(tab.id);
          } else {
            this.model.activeTabId = tab.id;
            this.render();
          }
        });
        this.tabsBar.appendChild(tabEl);
      });

      const newBtn = document.createElement('button');
      newBtn.className = 'tab-new-btn';
      newBtn.title = 'Add New Song / Tab';
      newBtn.textContent = '+';
      newBtn.addEventListener('click', () => this.addNewTab());
      this.tabsBar.appendChild(newBtn);
    }

    addNewTab(title = null, clone = false) {
      const tabNum = this.model.tabs.length + 1;
      const newId = `tab-${Date.now()}`;
      const newTitle = title || `Song ${tabNum}`;
      const bars = clone ? JSON.parse(JSON.stringify(this.model.getActiveTab().bars)) : this.model.createDefaultBars(16);
      this.model.tabs.push({ id: newId, title: newTitle, bars });
      this.model.activeTabId = newId;
      this.render();
    }

    closeTab(tabId) {
      if (this.model.tabs.length <= 1) return;
      this.model.tabs = this.model.tabs.filter(t => t.id !== tabId);
      this.model.activeTabId = this.model.tabs[0].id;
      this.render();
    }

    renderBars() {
      this.sheet.innerHTML = '';
      const tab = this.model.getActiveTab();

      // Spiral Ring Header Decoration
      const spiral = document.createElement('div');
      spiral.className = 'spiral-rings';
      for (let i = 0; i < 18; i++) {
        const ring = document.createElement('div');
        ring.className = 'spiral-ring';
        spiral.appendChild(ring);
      }
      this.sheet.appendChild(spiral);

      // Crimson Margin Rule
      const margin = document.createElement('div');
      margin.className = 'margin-rule';
      this.sheet.appendChild(margin);

      tab.bars.forEach((bar, barIdx) => {
        const row = document.createElement('div');
        row.id = `bar-row-${barIdx}`;
        row.className = `bar-row ${barIdx % 2 === 1 ? 'even' : ''} ${bar.isStanzaBreak ? 'stanza-break-after' : ''}`;

        // Playhead & Line Number
        const gutter = document.createElement('div');
        gutter.className = 'playhead-gutter';
        gutter.innerHTML = `
          <span class="playhead-marker">▶</span>
          <span class="bar-num">${barIdx + 1}</span>
        `;
        row.appendChild(gutter);

        // Meter Badge
        const meter = document.createElement('div');
        meter.className = 'meter-badge';
        meter.textContent = `[${bar.meter[0]}/${bar.meter[1]}]`;
        meter.title = 'Click to change meter';
        meter.addEventListener('click', () => this.promptMeterChange(barIdx));
        row.appendChild(meter);

        // Pulse Groups & Syllable Cells
        const pulsesContainer = document.createElement('div');
        pulsesContainer.className = 'pulses-container';

        let sylIdx = 0;
        bar.pulseGroups.forEach((groupSize) => {
          const group = document.createElement('div');
          group.className = 'pulse-group';

          for (let p = 0; p < groupSize; p++) {
            const currentIdx = sylIdx++;
            const sylData = bar.syllables[currentIdx] || { text: '', bold: false, align: 'center', customColor: null };
            const cell = document.createElement('div');
            cell.className = 'syl-cell';
            cell.dataset.bar = barIdx;
            cell.dataset.syl = currentIdx;

            // Highlight Color: Custom or Phonetic
            if (sylData.customColor) {
              cell.style.backgroundColor = sylData.customColor;
            } else if (sylData.text) {
              const rhymeKey = PhonemeEngine.getRhymeKey(sylData.text);
              const color = PhonemeEngine.getColorForRhyme(rhymeKey, this.model.activeRhymeKeys);
              if (color) cell.style.backgroundColor = color;
            }

            const input = document.createElement('input');
            input.type = 'text';
            input.className = `syl-input align-${sylData.align} ${sylData.bold ? 'bold' : ''}`;
            input.value = sylData.text;

            // Input handlers
            input.addEventListener('focus', () => {
              this.selectedCell = { barIdx, sylIdx: currentIdx };
              cell.classList.add('selected');
            });
            input.addEventListener('blur', () => cell.classList.remove('selected'));
            input.addEventListener('input', (e) => this.handleCellInput(barIdx, currentIdx, e.target.value));

            // Right-click context menu
            cell.addEventListener('contextmenu', (e) => {
              e.preventDefault();
              this.openContextMenu(e.clientX, e.clientY, barIdx, currentIdx);
            });

            // Hover Alignment Controls
            const alignCtrls = document.createElement('div');
            alignCtrls.className = 'cell-align-ctrls';
            alignCtrls.innerHTML = `
              <button class="align-arrow-left" title="Align Left">←</button>
              <div class="align-underline-center" title="Align Center"></div>
              <button class="align-arrow-right" title="Align Right">→</button>
            `;
            alignCtrls.querySelector('.align-arrow-left').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlign(barIdx, currentIdx, 'left');
            });
            alignCtrls.querySelector('.align-underline-center').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlign(barIdx, currentIdx, 'center');
            });
            alignCtrls.querySelector('.align-arrow-right').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlign(barIdx, currentIdx, 'right');
            });

            cell.appendChild(input);
            cell.appendChild(alignCtrls);
            group.appendChild(cell);
          }
          pulsesContainer.appendChild(group);
        });
        row.appendChild(pulsesContainer);

        // Syllable Counter Column
        const counterCol = document.createElement('div');
        counterCol.className = 'syl-counter-col';
        const calculatedSyls = bar.syllables.filter(s => s.text.trim().length > 0).length;
        const displaySyls = bar.spokenSyllables !== null ? bar.spokenSyllables : calculatedSyls;
        const isCustom = bar.spokenSyllables !== null;

        counterCol.innerHTML = `
          <button class="syl-counter-btn dec-syl">-</button>
          <span class="syl-counter-badge ${isCustom ? 'custom' : ''}" title="Double click to reset">${displaySyls} / ${bar.pulses}</span>
          <button class="syl-counter-btn inc-syl">+</button>
        `;
        counterCol.querySelector('.dec-syl').addEventListener('click', () => {
          bar.spokenSyllables = Math.max(0, (bar.spokenSyllables ?? calculatedSyls) - 1);
          this.render();
        });
        counterCol.querySelector('.inc-syl').addEventListener('click', () => {
          bar.spokenSyllables = (bar.spokenSyllables ?? calculatedSyls) + 1;
          this.render();
        });
        counterCol.querySelector('.syl-counter-badge').addEventListener('dblclick', () => {
          bar.spokenSyllables = null;
          this.render();
        });
        row.appendChild(counterCol);

        // Row Hover '+' Button
        const rowAddBtn = document.createElement('button');
        rowAddBtn.className = 'row-add-btn';
        rowAddBtn.textContent = '+';
        rowAddBtn.title = 'Add new bar or toggle stanza break';
        rowAddBtn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.insertBarBelow(barIdx);
        });
        row.appendChild(rowAddBtn);

        this.sheet.appendChild(row);
      });
    }

    handleCellInput(barIdx, sylIdx, text) {
      const bar = this.model.getActiveTab().bars[barIdx];
      const splits = PhonemeEngine.splitWord(text);

      if (splits.length > 1) {
        // Auto-split across consecutive cells with auto-anchoring!
        splits.forEach((part, offset) => {
          const targetIdx = sylIdx + offset;
          if (targetIdx < bar.syllables.length) {
            bar.syllables[targetIdx].text = part;
            // Hug divider: first part anchors right, last anchors left
            if (offset === 0) bar.syllables[targetIdx].align = 'right';
            else if (offset === splits.length - 1) bar.syllables[targetIdx].align = 'left';
            else bar.syllables[targetIdx].align = 'center';
          }
        });
      } else {
        bar.syllables[sylIdx].text = text;
      }
      this.render();
    }

    setCellAlign(barIdx, sylIdx, align) {
      this.model.getActiveTab().bars[barIdx].syllables[sylIdx].align = align;
      this.render();
    }

    toggleCellBold(target) {
      const cell = this.model.getActiveTab().bars[target.barIdx].syllables[target.sylIdx];
      cell.bold = !cell.bold;
      this.render();
    }

    insertBarBelow(barIdx) {
      const tab = this.model.getActiveTab();
      const currentBar = tab.bars[barIdx];
      const newBar = {
        meter: [...currentBar.meter],
        pulses: currentBar.pulses,
        pulseGroups: [...currentBar.pulseGroups],
        syllables: Array(currentBar.pulses).fill('').map(() => ({
          text: '',
          bold: false,
          align: 'center',
          customColor: null
        })),
        spokenSyllables: null,
        isStanzaBreak: false
      };
      tab.bars.splice(barIdx + 1, 0, newBar);
      this.render();
    }

    promptMeterChange(barIdx) {
      const bar = this.model.getActiveTab().bars[barIdx];
      const meterStr = prompt('Enter new meter (e.g. 4/4, 3/4, 6/8, 5/4):', `${bar.meter[0]}/${bar.meter[1]}`);
      if (!meterStr) return;
      const parts = meterStr.split('/').map(Number);
      if (parts.length === 2 && !isNaN(parts[0]) && !isNaN(parts[1])) {
        bar.meter = parts;
        bar.pulses = parts[0] * 4; // 16ths representation
        bar.pulseGroups = Array(parts[0]).fill(4);
        bar.syllables = Array(bar.pulses).fill('').map(() => ({
          text: '',
          bold: false,
          align: 'center',
          customColor: null
        }));
        this.render();
      }
    }

    openContextMenu(x, y, barIdx, sylIdx) {
      this.contextCellTarget = { barIdx, sylIdx };
      const syl = this.model.getActiveTab().bars[barIdx].syllables[sylIdx];
      const rhyme = PhonemeEngine.getRhymeKey(syl.text) || 'NONE';

      this.contextMenu.style.left = `${Math.min(window.innerWidth - 220, x)}px`;
      this.contextMenu.style.top = `${Math.min(window.innerHeight - 320, y)}px`;
      this.contextMenu.style.display = 'block';

      this.contextMenu.innerHTML = `
        <div class="menu-header">Syllable: "${syl.text || ' '}" [${rhyme.toUpperCase()}]</div>
        <div class="color-swatches">
          ${PhonemeEngine.pastelPalette.slice(0, 7).map(c => `<div class="swatch" style="background:${c}" data-color="${c}"></div>`).join('')}
        </div>
        <div class="menu-item" id="menu-auto-color">Auto (Phoneme Tint)</div>
        <div class="menu-item" id="menu-clear-color">Clear Highlight</div>
        <div class="menu-divider"></div>
        <div class="menu-item" id="menu-upper">UPPERCASE</div>
        <div class="menu-item" id="menu-lower">lowercase</div>
        <div class="menu-item" id="menu-capitalize">Capitalize Word</div>
        <div class="menu-divider"></div>
        <div class="menu-item" id="menu-dup">Duplicate into Next Box</div>
        <div class="menu-item" id="menu-bold">Bold Emphasis (Ctrl+B)</div>
      `;

      // Swatches click
      this.contextMenu.querySelectorAll('.swatch').forEach(sw => {
        sw.addEventListener('click', () => {
          syl.customColor = sw.dataset.color;
          this.contextMenu.style.display = 'none';
          this.render();
        });
      });

      document.getElementById('menu-auto-color').addEventListener('click', () => {
        syl.customColor = null;
        this.contextMenu.style.display = 'none';
        this.render();
      });

      document.getElementById('menu-clear-color').addEventListener('click', () => {
        syl.customColor = 'transparent';
        this.contextMenu.style.display = 'none';
        this.render();
      });

      document.getElementById('menu-upper').addEventListener('click', () => {
        syl.text = syl.text.toUpperCase();
        this.contextMenu.style.display = 'none';
        this.render();
      });

      document.getElementById('menu-lower').addEventListener('click', () => {
        syl.text = syl.text.toLowerCase();
        this.contextMenu.style.display = 'none';
        this.render();
      });

      document.getElementById('menu-capitalize').addEventListener('click', () => {
        syl.text = syl.text.charAt(0).toUpperCase() + syl.text.slice(1).toLowerCase();
        this.contextMenu.style.display = 'none';
        this.render();
      });

      document.getElementById('menu-dup').addEventListener('click', () => {
        const bar = this.model.getActiveTab().bars[barIdx];
        if (sylIdx + 1 < bar.syllables.length) {
          bar.syllables[sylIdx + 1] = { ...syl };
          this.contextMenu.style.display = 'none';
          this.render();
        }
      });

      document.getElementById('menu-bold').addEventListener('click', () => {
        syl.bold = !syl.bold;
        this.contextMenu.style.display = 'none';
        this.render();
      });
    }

    copyLyricsToClipboard() {
      const tab = this.model.getActiveTab();
      const lines = tab.bars.map(bar => {
        const words = bar.syllables.map(s => s.text).filter(Boolean).join(' ');
        const sylCount = bar.spokenSyllables !== null ? bar.spokenSyllables : bar.syllables.filter(s => s.text).length;
        return words ? `${words} [${sylCount}]` : '';
      });
      const text = lines.filter(Boolean).join('\n');
      navigator.clipboard.writeText(text).then(() => {
        this.copyBtn.textContent = 'Copied!';
        setTimeout(() => { this.copyBtn.textContent = 'Copy'; }, 1200);
      });
    }

    saveState() {
      try {
        localStorage.setItem('compass_cadence_web_state', JSON.stringify({
          tabs: this.model.tabs,
          activeTabId: this.model.activeTabId,
          darkMode: this.model.darkMode,
          bpm: this.metronome.bpm
        }));
      } catch (e) {
        console.warn('Storage failed', e);
      }
    }

    loadSavedState() {
      try {
        const saved = JSON.parse(localStorage.getItem('compass_cadence_web_state'));
        if (saved && saved.tabs && saved.tabs.length) {
          this.model.tabs = saved.tabs;
          this.model.activeTabId = saved.activeTabId || saved.tabs[0].id;
          this.model.darkMode = !!saved.darkMode;
          if (saved.bpm) this.metronome.setBpm(saved.bpm);
          document.body.classList.toggle('dark-mode', this.model.darkMode);
          this.darkModeBtn.textContent = this.model.darkMode ? 'Dark: ON' : 'Dark: OFF';
          this.tempoSpan.textContent = `${this.metronome.bpm} BPM`;
        }
      } catch (e) {
        console.warn('Could not load saved state', e);
      }
    }
  }

  // Launch on DOM Ready
  window.addEventListener('DOMContentLoaded', () => {
    window.app = new AppController();

    // Register Service Worker for Offline PWA
    if ('serviceWorker' in navigator) {
      navigator.serviceWorker.register('./sw.js').catch(err => {
        console.log('SW registration failed:', err);
      });
    }
  });
})();
