/**
 * Compass Cadence — Full 1:1 VST3 Feature Engine & UI
 * Pixel-perfect reproduction of JUCE C++ UI, typing flow, metric math, and Web Audio transport.
 */

(function () {
  'use strict';

  // =========================================================================
  // 1. Metric Notation Model
  // =========================================================================
  class MetricNotation {
    constructor(pulseSubdivs = [4, 4, 4, 4], beats = 4) {
      this.pulseSubdivs = [...pulseSubdivs];
      this.beatsPerBar = beats;
    }

    static fromString(str) {
      if (!str) return new MetricNotation([4, 4, 4, 4], 4);
      const match = str.trim().match(/^\[([0-9]+)\](?:\/([0-9]+):([0-9]+))?/);
      if (!match) return new MetricNotation([4, 4, 4, 4], 4);
      const digits = match[1].split('').map(Number);
      const beats = match[3] ? parseInt(match[3], 10) : (match[2] ? parseInt(match[2], 10) : digits.length);
      return new MetricNotation(digits.length ? digits : [4, 4, 4, 4], beats || 4);
    }

    toString() {
      const digits = this.pulseSubdivs.join('');
      return `[${digits}]/${this.pulseSubdivs.length}:${this.beatsPerBar}`;
    }

    getTotalSyllables() {
      return this.pulseSubdivs.reduce((a, b) => a + b, 0);
    }

    clone() {
      return new MetricNotation(this.pulseSubdivs, this.beatsPerBar);
    }
  }

  // =========================================================================
  // 2. Syllable Splitter & Phonetic Engine
  // =========================================================================
  class SyllableSplitter {
    static splitWord(word) {
      if (!word) return [];
      const clean = word.toLowerCase().replace(/[^a-z'-]/g, '');
      if (clean.length <= 3) return [word];

      // Sibilant rule for -es endings: -es only adds a syllable if preceded by sibilant
      const isSibilantEnding = /(?:[scxz]|ch|sh)es$/i.test(clean);
      const isSilentEs = !isSibilantEnding && /[^aeiouy]es$/i.test(clean);
      const isSilentEd = !/[td]ed$/i.test(clean) && /[^aeiouy]ed$/i.test(clean);

      // Vowel clusters
      const vowelClusters = clean.match(/[aeiouy]+/gi) || [];
      let count = vowelClusters.length;
      if (isSilentEs) count--;
      if (isSilentEd) count--;
      if (clean.endsWith('e') && !clean.endsWith('le') && !clean.endsWith('ee')) count--;

      if (count <= 1) return [word];

      // Divide multi-syllable word cleanly
      const mid = Math.floor(word.length / 2);
      return [word.slice(0, mid) + '-', word.slice(mid)];
    }

    static getRhymeKey(word) {
      if (!word) return '';
      const clean = word.toLowerCase().replace(/[^a-z]/g, '');
      if (clean.length < 2) return clean;
      // Extract terminal vowel and coda
      const match = clean.match(/([aeiouy]+[^aeiouy]*)$/i);
      return match ? match[1] : clean.slice(-2);
    }
  }

  // 20 Non-colliding pastel highlighter colors matching VST3
  const PASTEL_PALETTE = [
    'rgba(134, 239, 172, 0.65)', // Mint
    'rgba(147, 197, 253, 0.65)', // Sky
    'rgba(254, 215, 170, 0.65)', // Peach
    'rgba(221, 214, 254, 0.65)', // Lavender
    'rgba(254, 240, 138, 0.65)', // Butter
    'rgba(254, 205, 211, 0.65)', // Rose
    'rgba(203, 213, 225, 0.65)', // Slate
    'rgba(167, 243, 208, 0.65)', // Seafoam
    'rgba(186, 230, 253, 0.65)', // Ice
    'rgba(253, 230, 138, 0.65)', // Sunflower
    'rgba(251, 207, 232, 0.65)', // Pink
    'rgba(199, 210, 254, 0.65)', // Periwinkle
    'rgba(233, 213, 255, 0.65)', // Lilac
    'rgba(245, 208, 254, 0.65)', // Orchid
    'rgba(254, 226, 226, 0.65)', // Coral
    'rgba(220, 252, 231, 0.65)', // Honeydew
    'rgba(207, 250, 254, 0.65)', // Cyan
    'rgba(254, 243, 199, 0.65)', // Sand
    'rgba(224, 231, 255, 0.65)', // Azure
    'rgba(243, 244, 246, 0.65)'  // Ash
  ];

  // =========================================================================
  // 3. Audio Metronome & Transport
  // =========================================================================
  class AudioEngine {
    constructor() {
      this.ctx = null;
      this.bpm = 120.0;
      this.isPlaying = false;
      this.clickEnabled = true;
      this.currentBar = 0;
      this.currentPulse = 0;
      this.nextPulseTime = 0.0;
      this.timerId = null;
      this.onTick = null;
    }

    ensureContext() {
      if (!this.ctx) {
        const AudioCtx = window.AudioContext || window.webkitAudioContext;
        this.ctx = new AudioCtx();
      }
      if (this.ctx.state === 'suspended') {
        this.ctx.resume();
      }
    }

    start(onTick) {
      this.ensureContext();
      this.onTick = onTick;
      this.isPlaying = true;
      this.currentBar = 0;
      this.currentPulse = 0;
      this.nextPulseTime = this.ctx.currentTime + 0.05;
      this.scheduler();
    }

    stop() {
      this.isPlaying = false;
      clearTimeout(this.timerId);
    }

    setBpm(bpm) {
      this.bpm = Math.max(20.0, Math.min(300.0, bpm));
    }

    scheduler() {
      if (!this.isPlaying) return;
      while (this.nextPulseTime < this.ctx.currentTime + 0.1) {
        this.playClick(this.nextPulseTime, this.currentPulse === 0);
        const bar = this.currentBar;
        const pulse = this.currentPulse;
        const delayMs = Math.max(0, (this.nextPulseTime - this.ctx.currentTime) * 1000);
        setTimeout(() => {
          if (this.isPlaying && this.onTick) this.onTick(bar, pulse);
        }, delayMs);

        // Advance pulse (default 4 beats per bar)
        const secPerPulse = (60.0 / this.bpm);
        this.nextPulseTime += secPerPulse;
        this.currentPulse++;
        if (this.currentPulse >= 4) {
          this.currentPulse = 0;
          this.currentBar++;
        }
      }
      this.timerId = setTimeout(() => this.scheduler(), 25);
    }

    playClick(time, isDownbeat) {
      if (!this.clickEnabled) return;
      const osc = this.ctx.createOscillator();
      const gain = this.ctx.createGain();
      // Downbeat 1600 Hz, offbeat 1000 Hz
      osc.type = 'sine';
      osc.frequency.setValueAtTime(isDownbeat ? 1600 : 1000, time);
      gain.gain.setValueAtTime(0.7, time);
      gain.gain.exponentialRampToValueAtTime(0.0001, time + 0.035);
      osc.connect(gain);
      gain.connect(this.ctx.destination);
      osc.start(time);
      osc.stop(time + 0.035);
    }
  }

  // =========================================================================
  // 4. Notebook Application Controller
  // =========================================================================
  class CompassCadenceApp {
    constructor() {
      this.audio = new AudioEngine();
      this.tabs = [];
      this.activeTabIdx = 0;
      this.globalNotation = new MetricNotation([4, 4, 4, 4], 4);
      this.rhymesEnabled = true;
      this.followPlayhead = true;
      this.darkMode = false;
      this.viewMode = 'scroll'; // 'scroll' or 'pages'
      this.currentPage = 0;
      this.activeRhymeMap = new Map();
      this.focusedCellCoords = null; // { barIdx, pulseIdx, sylIdx }
      this.customPresets = {};
      this.songProjects = {};

      this.initDefaultContent();
      this.loadStorage();
      this.cacheDOMElements();
      this.bindUIEvents();
      this.renderAll();
      this.drawSpiralCanvas();
      window.addEventListener('resize', () => this.drawSpiralCanvas());
    }

    initDefaultContent() {
      this.tabs = [
        {
          title: 'Verse 1',
          bars: this.createBars(16, this.globalNotation)
        }
      ];
    }

    createBars(count, notation) {
      const bars = [];
      for (let i = 0; i < count; i++) {
        bars.push(this.createSingleBar(notation, (i + 1) % 4 === 0));
      }
      return bars;
    }

    createSingleBar(notation, isStanza = false) {
      const notat = notation.clone();
      const totalSyls = notat.getTotalSyllables();
      const syllables = [];
      for (let p = 0; p < notat.pulseSubdivs.length; p++) {
        syllables.push(Array(notat.pulseSubdivs[p]).fill(null).map(() => ({
          text: '',
          bold: false,
          align: 'center',
          customColor: null
        })));
      }
      return {
        notation: notat,
        syllables: syllables,
        spokenSyllables: null, // null = auto
        isStanzaBreak: isStanza
      };
    }

    cacheDOMElements() {
      this.viewport = document.getElementById('notebook-viewport');
      this.pageContainer = document.getElementById('notebook-page');
      this.tabsStrip = document.getElementById('tabs-strip');
      this.spiralCanvas = document.getElementById('spiral-canvas');

      this.metricInput = document.getElementById('metric-grid-editor');
      this.pulsesVal = document.getElementById('pulses-count-val');
      this.beatsVal = document.getElementById('beats-count-val');
      this.presetBtn = document.getElementById('preset-combo-btn');
      this.savePresetBtn = document.getElementById('save-preset-btn');
      this.rhymeToggleBtn = document.getElementById('rhyme-toggle-btn');
      this.followToggleBtn = document.getElementById('follow-toggle-btn');
      this.copyBtn = document.getElementById('copy-btn');
      this.exportBtn = document.getElementById('export-btn');
      this.songsBtn = document.getElementById('songs-btn');

      this.darkModeBtn = document.getElementById('dark-mode-btn');
      this.viewModeBtn = document.getElementById('view-mode-btn');
      this.prevPageBtn = document.getElementById('prev-page-btn');
      this.nextPageBtn = document.getElementById('next-page-btn');
      this.pageLabel = document.getElementById('page-display-label');

      this.playBtn = document.getElementById('play-btn');
      this.metronomeBtn = document.getElementById('metronome-btn');
      this.decBpmBtn = document.getElementById('dec-bpm-btn');
      this.incBpmBtn = document.getElementById('inc-bpm-btn');
      this.bpmLabel = document.getElementById('bpm-val-label');
      this.dawStatusLabel = document.getElementById('daw-status-label');

      this.presetMenu = document.getElementById('preset-menu');
      this.exportMenu = document.getElementById('export-menu');
      this.songsMenu = document.getElementById('songs-menu');
      this.contextMenu = document.getElementById('context-menu');
    }

    bindUIEvents() {
      // Metric Notation Input
      this.metricInput.addEventListener('change', (e) => {
        this.globalNotation = MetricNotation.fromString(e.target.value);
        this.updateHeaderDisplay();
        // Update bars
        const tab = this.tabs[this.activeTabIdx];
        tab.bars.forEach(b => {
          b.notation = this.globalNotation.clone();
          this.reconcileBarSyllables(b);
        });
        this.renderAll();
      });

      // Pulse Steppers
      document.getElementById('dec-pulse-btn').addEventListener('click', () => {
        if (this.globalNotation.pulseSubdivs.length > 1) {
          this.globalNotation.pulseSubdivs.pop();
          this.applyGlobalNotation();
        }
      });
      document.getElementById('inc-pulse-btn').addEventListener('click', () => {
        this.globalNotation.pulseSubdivs.push(4);
        this.applyGlobalNotation();
      });

      // Beat Steppers
      document.getElementById('dec-beat-btn').addEventListener('click', () => {
        if (this.globalNotation.beatsPerBar > 1) {
          this.globalNotation.beatsPerBar--;
          this.applyGlobalNotation();
        }
      });
      document.getElementById('inc-beat-btn').addEventListener('click', () => {
        this.globalNotation.beatsPerBar++;
        this.applyGlobalNotation();
      });

      // Preset & Save Buttons
      this.presetBtn.addEventListener('click', (e) => this.showPresetMenu(e));
      this.savePresetBtn.addEventListener('click', () => this.promptSavePreset());

      // Rhyme & Follow Toggles
      this.rhymeToggleBtn.addEventListener('click', () => {
        this.rhymesEnabled = !this.rhymesEnabled;
        this.rhymeToggleBtn.classList.toggle('toggled', this.rhymesEnabled);
        this.rhymeToggleBtn.textContent = this.rhymesEnabled ? 'Rhymes: ON' : 'Rhymes: OFF';
        this.refreshRhymeColors();
      });

      this.followToggleBtn.addEventListener('click', () => {
        this.followPlayhead = !this.followPlayhead;
        this.followToggleBtn.classList.toggle('toggled', this.followPlayhead);
        this.followToggleBtn.textContent = this.followPlayhead ? 'Follow DAW: ON' : 'Follow DAW: OFF';
      });

      // Copy & Export & Songs
      this.copyBtn.addEventListener('click', () => this.copyLyrics());
      this.exportBtn.addEventListener('click', (e) => this.showExportMenu(e));
      this.songsBtn.addEventListener('click', (e) => this.showSongsMenu(e));

      // Row 2: Dark Mode & View Mode
      this.darkModeBtn.addEventListener('click', () => {
        this.darkMode = !this.darkMode;
        document.body.classList.toggle('dark-mode', this.darkMode);
        this.darkModeBtn.textContent = this.darkMode ? 'Dark: ON' : 'Dark: OFF';
        this.drawSpiralCanvas();
        this.saveStorage();
      });

      this.viewModeBtn.addEventListener('click', () => {
        this.viewMode = this.viewMode === 'scroll' ? 'pages' : 'scroll';
        this.viewModeBtn.textContent = this.viewMode === 'scroll' ? 'Scroll' : 'Pages';
        this.renderBars();
      });

      this.prevPageBtn.addEventListener('click', () => {
        if (this.viewMode === 'scroll') {
          this.viewport.scrollBy({ top: -600, behavior: 'smooth' });
        } else if (this.currentPage > 0) {
          this.currentPage--;
          this.renderBars();
        }
      });

      this.nextPageBtn.addEventListener('click', () => {
        if (this.viewMode === 'scroll') {
          this.viewport.scrollBy({ top: 600, behavior: 'smooth' });
        } else {
          this.currentPage++;
          this.renderBars();
        }
      });

      // Transport: Play, Click, BPM
      this.playBtn.addEventListener('click', () => this.togglePlayback());
      this.metronomeBtn.addEventListener('click', () => {
        this.audio.clickEnabled = !this.audio.clickEnabled;
        this.metronomeBtn.textContent = this.audio.clickEnabled ? 'Click: ON' : 'Click: OFF';
      });
      this.decBpmBtn.addEventListener('click', () => this.adjustBpm(-5));
      this.incBpmBtn.addEventListener('click', () => this.adjustBpm(5));

      // Global Dismiss Popup Menus
      document.addEventListener('click', (e) => {
        [this.presetMenu, this.exportMenu, this.songsMenu, this.contextMenu].forEach(m => {
          if (!m.contains(e.target)) m.style.display = 'none';
        });
      });

      // Global Shortcuts
      document.addEventListener('keydown', (e) => {
        if (e.code === 'Space' && !e.target.classList.contains('cell-active-input')) {
          e.preventDefault();
          this.togglePlayback();
        }
        if ((e.ctrlKey || e.metaKey) && e.key === 'c' && !e.target.classList.contains('cell-active-input')) {
          this.copyLyrics();
        }
      });
    }

    applyGlobalNotation() {
      this.updateHeaderDisplay();
      const tab = this.tabs[this.activeTabIdx];
      tab.bars.forEach(b => {
        b.notation = this.globalNotation.clone();
        this.reconcileBarSyllables(b);
      });
      this.renderBars();
    }

    updateHeaderDisplay() {
      this.metricInput.value = this.globalNotation.toString();
      this.pulsesVal.textContent = this.globalNotation.pulseSubdivs.length;
      this.beatsVal.textContent = this.globalNotation.beatsPerBar;
    }

    adjustBpm(delta) {
      this.audio.setBpm(this.audio.bpm + delta);
      this.bpmLabel.textContent = `${Math.round(this.audio.bpm)} BPM`;
      this.updateDawStatus();
    }

    updateDawStatus() {
      const state = this.audio.isPlaying ? '[PLAYING]' : '[STOPPED]';
      this.dawStatusLabel.innerHTML = `${this.audio.bpm.toFixed(1)} BPM &nbsp;|&nbsp; ${this.globalNotation.beatsPerBar}/4 &nbsp;|&nbsp; ${state}`;
    }

    togglePlayback() {
      if (this.audio.isPlaying) {
        this.audio.stop();
        this.playBtn.textContent = 'Play';
        this.playBtn.classList.remove('toggled');
        document.querySelectorAll('.playhead-arrow').forEach(el => el.classList.remove('active'));
      } else {
        this.audio.start((barIdx, pulseIdx) => {
          this.onPlaybackTick(barIdx, pulseIdx);
        });
        this.playBtn.textContent = 'Stop';
        this.playBtn.classList.add('toggled');
      }
      this.updateDawStatus();
    }

    onPlaybackTick(barIdx, pulseIdx) {
      const tab = this.tabs[this.activeTabIdx];
      const actualBar = barIdx % tab.bars.length;

      document.querySelectorAll('.playhead-arrow').forEach((el, idx) => {
        el.classList.toggle('active', idx === actualBar);
      });

      if (this.followPlayhead) {
        const rowEl = document.getElementById(`bar-row-${actualBar}`);
        if (rowEl) {
          rowEl.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
        }
      }
    }

    reconcileBarSyllables(bar) {
      const newSyls = [];
      for (let p = 0; p < bar.notation.pulseSubdivs.length; p++) {
        const count = bar.notation.pulseSubdivs[p];
        const existingPulse = bar.syllables[p] || [];
        const pulseArr = [];
        for (let s = 0; s < count; s++) {
          pulseArr.push(existingPulse[s] || { text: '', bold: false, align: 'center', customColor: null });
        }
        newSyls.push(pulseArr);
      }
      bar.syllables = newSyls;
    }

    // =========================================================================
    // 5. DOM Rendering & Incremental Updates
    // =========================================================================
    renderAll() {
      this.updateHeaderDisplay();
      this.bpmLabel.textContent = `${Math.round(this.audio.bpm)} BPM`;
      this.updateDawStatus();
      this.renderTabs();
      this.renderBars();
    }

    renderTabs() {
      this.tabsStrip.innerHTML = '';
      this.tabs.forEach((tab, idx) => {
        const tabEl = document.createElement('div');
        tabEl.className = `notebook-tab ${idx === this.activeTabIdx ? 'active' : ''}`;
        tabEl.innerHTML = `
          <span>${tab.title}</span>
          ${this.tabs.length > 1 ? '<span class="close-tab" title="Close tab">&times;</span>' : ''}
        `;
        tabEl.addEventListener('click', (e) => {
          if (e.target.classList.contains('close-tab')) {
            e.stopPropagation();
            this.closeTab(idx);
          } else {
            this.activeTabIdx = idx;
            this.renderAll();
          }
        });
        tabEl.addEventListener('dblclick', () => {
          const newName = prompt('Rename Tab:', tab.title);
          if (newName) { tab.title = newName; this.renderTabs(); }
        });
        this.tabsStrip.appendChild(tabEl);
      });

      const addBtn = document.createElement('button');
      addBtn.id = 'add-tab-btn';
      addBtn.title = 'Add new song or duplicate tab';
      addBtn.textContent = '+';
      addBtn.addEventListener('click', () => this.addNewTab());
      this.tabsStrip.appendChild(addBtn);
    }

    addNewTab(clone = false) {
      const title = clone ? `${this.tabs[this.activeTabIdx].title} (Copy)` : `Verse ${this.tabs.length + 1}`;
      const bars = clone ? JSON.parse(JSON.stringify(this.tabs[this.activeTabIdx].bars)) : this.createBars(16, this.globalNotation);
      this.tabs.push({ title, bars });
      this.activeTabIdx = this.tabs.length - 1;
      this.renderAll();
    }

    closeTab(idx) {
      if (this.tabs.length <= 1) return;
      this.tabs.splice(idx, 1);
      this.activeTabIdx = Math.min(this.activeTabIdx, this.tabs.length - 1);
      this.renderAll();
    }

    renderBars() {
      this.pageContainer.innerHTML = '';
      const tab = this.tabs[this.activeTabIdx];

      // Pagination calculation
      let startBar = 0;
      let endBar = tab.bars.length;
      if (this.viewMode === 'pages') {
        startBar = this.currentPage * 16;
        endBar = Math.min(tab.bars.length, startBar + 16);
        this.pageLabel.textContent = `Page ${this.currentPage + 1} / ${Math.ceil(tab.bars.length / 16)}`;
      } else {
        this.pageLabel.textContent = `Bars 1-${tab.bars.length}`;
      }

      for (let b = startBar; b < endBar; b++) {
        const bar = tab.bars[b];
        const row = document.createElement('div');
        row.id = `bar-row-${b}`;
        row.className = `bar-row ${b % 2 === 1 ? 'even-line' : ''} ${bar.isStanzaBreak ? 'stanza-break' : ''}`;

        // Playhead gutter marker (stationary deep copper arrow in margin)
        const gutter = document.createElement('div');
        gutter.className = 'gutter-col';
        gutter.innerHTML = `<span class="playhead-arrow">▶</span>`;
        row.appendChild(gutter);

        // Bar Number
        const num = document.createElement('span');
        num.className = 'bar-num-text';
        num.textContent = `${b + 1}`;
        row.appendChild(num);

        // Metric Notation Label
        const meter = document.createElement('span');
        meter.className = 'bar-metric-label';
        meter.textContent = bar.notation.toString();
        meter.title = 'Click to customize metric flow for this bar';
        meter.addEventListener('click', () => this.promptBarMeter(b));
        row.appendChild(meter);

        // Pulse Group Grid
        const grid = document.createElement('div');
        grid.className = 'bar-pulses-grid';

        bar.syllables.forEach((pulseArr, pIdx) => {
          const pulseBox = document.createElement('div');
          pulseBox.className = 'pulse-group-box';

          pulseArr.forEach((syl, sIdx) => {
            const cell = document.createElement('div');
            cell.className = 'syllable-cell';
            cell.id = `cell-${b}-${pIdx}-${sIdx}`;
            cell.dataset.bar = b;
            cell.dataset.pulse = pIdx;
            cell.dataset.syl = sIdx;

            // Apply highlight color
            this.applyCellHighlight(cell, syl);

            // Text display element
            const textSpan = document.createElement('span');
            textSpan.className = `cell-text align-${syl.align} ${syl.bold ? 'bold' : ''}`;
            textSpan.textContent = syl.text;
            cell.appendChild(textSpan);

            // Hover Alignment Controls
            const alignPanel = document.createElement('div');
            alignPanel.className = 'cell-align-panel';
            alignPanel.innerHTML = `
              <button class="align-arrow-left" title="Align Left">←</button>
              <div class="align-underline-bar" title="Align Center"></div>
              <button class="align-arrow-right" title="Align Right">→</button>
            `;
            alignPanel.querySelector('.align-arrow-left').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlignment(b, pIdx, sIdx, 'left');
            });
            alignPanel.querySelector('.align-underline-bar').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlignment(b, pIdx, sIdx, 'center');
            });
            alignPanel.querySelector('.align-arrow-right').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlignment(b, pIdx, sIdx, 'right');
            });
            cell.appendChild(alignPanel);

            // Click to start editing
            cell.addEventListener('click', () => {
              this.activateCellEditor(b, pIdx, sIdx);
            });

            // Right-click context menu
            cell.addEventListener('contextmenu', (e) => {
              e.preventDefault();
              this.showContextMenu(e.clientX, e.clientY, b, pIdx, sIdx);
            });

            pulseBox.appendChild(cell);
          });
          grid.appendChild(pulseBox);
        });
        row.appendChild(grid);

        // Syllable Counter
        const counterWrap = document.createElement('div');
        counterWrap.className = 'syl-counter-wrapper';
        const totalSubdivs = bar.notation.getTotalSyllables();
        const autoCount = bar.syllables.flat().filter(s => s.text.trim().length > 0).length;
        const displayCount = bar.spokenSyllables !== null ? bar.spokenSyllables : autoCount;
        const isCustom = bar.spokenSyllables !== null;

        counterWrap.innerHTML = `
          <button class="counter-btn dec">-</button>
          <span class="syl-count-badge ${isCustom ? 'customized' : ''}" title="Spoken syllables / Grid subdivisions. Double-click to reset">${displayCount}/${totalSubdivs}</span>
          <button class="counter-btn inc">+</button>
        `;
        counterWrap.querySelector('.dec').addEventListener('click', () => {
          bar.spokenSyllables = Math.max(0, (bar.spokenSyllables ?? autoCount) - 1);
          this.updateCounterDisplay(b);
        });
        counterWrap.querySelector('.inc').addEventListener('click', () => {
          bar.spokenSyllables = (bar.spokenSyllables ?? autoCount) + 1;
          this.updateCounterDisplay(b);
        });
        counterWrap.querySelector('.syl-count-badge').addEventListener('dblclick', () => {
          bar.spokenSyllables = null;
          this.updateCounterDisplay(b);
        });
        row.appendChild(counterWrap);

        // Row Hover "+" Button
        const addBtn = document.createElement('button');
        addBtn.className = 'row-add-button';
        addBtn.textContent = '+';
        addBtn.title = 'Add new bar or toggle stanza break';
        addBtn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.showRowAddMenu(e, b);
        });
        row.appendChild(addBtn);

        this.pageContainer.appendChild(row);
      }
    }

    // =========================================================================
    // 6. Fluid Syllable Inline Editor (Zero DOM Wipe on Keystroke!)
    // =========================================================================
    activateCellEditor(b, pIdx, sIdx, initialChar = null) {
      const cellEl = document.getElementById(`cell-${b}-${pIdx}-${sIdx}`);
      if (!cellEl) return;

      // Remove any existing active input
      this.deactivateActiveEditor();

      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      const textSpan = cellEl.querySelector('.cell-text');
      textSpan.style.display = 'none';
      cellEl.classList.add('focused');

      const input = document.createElement('input');
      input.type = 'text';
      input.className = `cell-active-input align-${syl.align} ${syl.bold ? 'bold' : ''}`;
      input.value = initialChar !== null ? initialChar : syl.text;
      cellEl.appendChild(input);

      input.focus();
      if (initialChar !== null) {
        input.setSelectionRange(1, 1);
      } else {
        input.select();
      }

      this.focusedCellCoords = { barIdx: b, pulseIdx: pIdx, sylIdx: sIdx };

      // Input Keystroke Handler
      input.addEventListener('input', (e) => {
        const val = e.target.value;
        // Check for multi-word or hyphenated auto-split
        if (val.includes(' ') || val.includes('-')) {
          this.handleWordSplitInput(b, pIdx, sIdx, val);
          return;
        }
        syl.text = val;
        this.updateCellAppearance(b, pIdx, sIdx);
        this.updateCounterDisplay(b);
      });

      // Keydown Navigation & Shortcuts
      input.addEventListener('keydown', (e) => {
        // Spacebar or Hyphen: advance to next syllable cell
        if (e.key === ' ' || e.key === '-') {
          e.preventDefault();
          let current = input.value.trim();
          if (e.key === '-' && current) current += '-';
          syl.text = current;
          this.updateCellAppearance(b, pIdx, sIdx);
          this.updateCounterDisplay(b);
          this.advanceCellFocus(b, pIdx, sIdx, 1);
          return;
        }

        // Backspace: if empty, retreat to previous cell
        if (e.key === 'Backspace' && input.value.length === 0) {
          e.preventDefault();
          this.advanceCellFocus(b, pIdx, sIdx, -1);
          return;
        }

        // Tab: navigate forward / Shift+Tab backward
        if (e.key === 'Tab') {
          e.preventDefault();
          this.advanceCellFocus(b, pIdx, sIdx, e.shiftKey ? -1 : 1);
          return;
        }

        // Enter: jump to next bar
        if (e.key === 'Enter') {
          e.preventDefault();
          this.jumpToNextBar(b);
          return;
        }

        // Left Arrow at beginning: previous cell
        if (e.key === 'ArrowLeft' && input.selectionStart === 0 && input.selectionEnd === 0) {
          e.preventDefault();
          this.advanceCellFocus(b, pIdx, sIdx, -1);
          return;
        }

        // Right Arrow at end: next cell
        if (e.key === 'ArrowRight' && input.selectionStart === input.value.length) {
          e.preventDefault();
          this.advanceCellFocus(b, pIdx, sIdx, 1);
          return;
        }

        // Up Arrow: jump to cell above
        if (e.key === 'ArrowUp') {
          e.preventDefault();
          this.jumpVerticalCell(b, pIdx, sIdx, -1);
          return;
        }

        // Down Arrow: jump to cell below
        if (e.key === 'ArrowDown') {
          e.preventDefault();
          this.jumpVerticalCell(b, pIdx, sIdx, 1);
          return;
        }

        // Ctrl+B: toggle bold
        if ((e.ctrlKey || e.metaKey) && e.key === 'b') {
          e.preventDefault();
          syl.bold = !syl.bold;
          input.classList.toggle('bold', syl.bold);
          this.updateCellAppearance(b, pIdx, sIdx);
          return;
        }

        // Ctrl+Left / Right / Up / Down: Alignment
        if ((e.ctrlKey || e.metaKey) && e.key === 'ArrowLeft') {
          e.preventDefault();
          this.setCellAlignment(b, pIdx, sIdx, 'left');
          input.className = `cell-active-input align-left ${syl.bold ? 'bold' : ''}`;
          return;
        }
        if ((e.ctrlKey || e.metaKey) && e.key === 'ArrowRight') {
          e.preventDefault();
          this.setCellAlignment(b, pIdx, sIdx, 'right');
          input.className = `cell-active-input align-right ${syl.bold ? 'bold' : ''}`;
          return;
        }
        if ((e.ctrlKey || e.metaKey) && (e.key === 'ArrowUp' || e.key === 'ArrowDown')) {
          e.preventDefault();
          this.setCellAlignment(b, pIdx, sIdx, 'center');
          input.className = `cell-active-input align-center ${syl.bold ? 'bold' : ''}`;
          return;
        }
      });

      input.addEventListener('blur', () => {
        // Commit text
        syl.text = input.value.trim();
        textSpan.textContent = syl.text;
        textSpan.style.display = 'flex';
        cellEl.classList.remove('focused');
        input.remove();
        this.updateCellAppearance(b, pIdx, sIdx);
        this.updateCounterDisplay(b);
        this.saveStorage();
      });
    }

    deactivateActiveEditor() {
      const activeInput = document.querySelector('.cell-active-input');
      if (activeInput) activeInput.blur();
    }

    advanceCellFocus(b, pIdx, sIdx, dir) {
      this.deactivateActiveEditor();
      const tab = this.tabs[this.activeTabIdx];
      const bar = tab.bars[b];

      let targetS = sIdx + dir;
      let targetP = pIdx;
      let targetB = b;

      if (targetS >= bar.syllables[targetP].length) {
        targetS = 0;
        targetP++;
        if (targetP >= bar.syllables.length) {
          targetP = 0;
          targetB++;
        }
      } else if (targetS < 0) {
        targetP--;
        if (targetP < 0) {
          targetB--;
          if (targetB >= 0) {
            targetP = tab.bars[targetB].syllables.length - 1;
            targetS = tab.bars[targetB].syllables[targetP].length - 1;
          }
        } else {
          targetS = bar.syllables[targetP].length - 1;
        }
      }

      if (targetB >= 0 && targetB < tab.bars.length) {
        this.activateCellEditor(targetB, targetP, targetS);
      }
    }

    jumpVerticalCell(b, pIdx, sIdx, barDelta) {
      this.deactivateActiveEditor();
      const tab = this.tabs[this.activeTabIdx];
      const targetB = b + barDelta;
      if (targetB >= 0 && targetB < tab.bars.length) {
        const targetBar = tab.bars[targetB];
        const targetP = Math.min(pIdx, targetBar.syllables.length - 1);
        const targetS = Math.min(sIdx, targetBar.syllables[targetP].length - 1);
        this.activateCellEditor(targetB, targetP, targetS);
      }
    }

    jumpToNextBar(b) {
      this.deactivateActiveEditor();
      const tab = this.tabs[this.activeTabIdx];
      if (b + 1 < tab.bars.length) {
        this.activateCellEditor(b + 1, 0, 0);
      }
    }

    handleWordSplitInput(b, pIdx, sIdx, text) {
      const parts = text.split(/[\s-]+/).filter(Boolean);
      const tab = this.tabs[this.activeTabIdx];
      const bar = tab.bars[b];

      let currP = pIdx;
      let currS = sIdx;

      parts.forEach((part, idx) => {
        if (currP < bar.syllables.length && currS < bar.syllables[currP].length) {
          const isSplitPart = parts.length > 1;
          const sylObj = bar.syllables[currP][currS];
          sylObj.text = (idx < parts.length - 1 && text.includes('-')) ? `${part}-` : part;
          if (isSplitPart) {
            if (idx === 0) sylObj.align = 'right';
            else if (idx === parts.length - 1) sylObj.align = 'left';
            else sylObj.align = 'center';
          }
          this.updateCellAppearance(b, currP, currS);

          currS++;
          if (currS >= bar.syllables[currP].length) {
            currS = 0;
            currP++;
          }
        }
      });

      this.updateCounterDisplay(b);
      this.advanceCellFocus(b, pIdx, sIdx, parts.length);
    }

    updateCellAppearance(b, pIdx, sIdx) {
      const cellEl = document.getElementById(`cell-${b}-${pIdx}-${sIdx}`);
      if (!cellEl) return;
      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      const textSpan = cellEl.querySelector('.cell-text');
      if (textSpan) {
        textSpan.textContent = syl.text;
        textSpan.className = `cell-text align-${syl.align} ${syl.bold ? 'bold' : ''}`;
      }
      this.applyCellHighlight(cellEl, syl);
    }

    applyCellHighlight(cellEl, syl) {
      if (syl.customColor) {
        cellEl.style.backgroundColor = syl.customColor;
      } else if (this.rhymesEnabled && syl.text.trim()) {
        const rhymeKey = SyllableSplitter.getRhymeKey(syl.text);
        if (rhymeKey) {
          if (!this.activeRhymeMap.has(rhymeKey)) {
            const color = PASTEL_PALETTE[this.activeRhymeMap.size % PASTEL_PALETTE.length];
            this.activeRhymeMap.set(rhymeKey, color);
          }
          cellEl.style.backgroundColor = this.activeRhymeMap.get(rhymeKey);
        } else {
          cellEl.style.backgroundColor = 'transparent';
        }
      } else {
        cellEl.style.backgroundColor = 'transparent';
      }
    }

    refreshRhymeColors() {
      this.activeRhymeMap.clear();
      const tab = this.tabs[this.activeTabIdx];
      tab.bars.forEach((bar, b) => {
        bar.syllables.forEach((pulse, p) => {
          pulse.forEach((syl, s) => {
            this.updateCellAppearance(b, p, s);
          });
        });
      });
    }

    setCellAlignment(b, pIdx, sIdx, align) {
      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      syl.align = align;
      this.updateCellAppearance(b, pIdx, sIdx);
    }

    updateCounterDisplay(b) {
      const rowEl = document.getElementById(`bar-row-${b}`);
      if (!rowEl) return;
      const bar = this.tabs[this.activeTabIdx].bars[b];
      const autoCount = bar.syllables.flat().filter(s => s.text.trim().length > 0).length;
      const displayCount = bar.spokenSyllables !== null ? bar.spokenSyllables : autoCount;
      const isCustom = bar.spokenSyllables !== null;
      const badge = rowEl.querySelector('.syl-count-badge');
      if (badge) {
        badge.textContent = `${displayCount}/${bar.notation.getTotalSyllables()}`;
        badge.classList.toggle('customized', isCustom);
      }
    }

    // =========================================================================
    // 7. Double-Wire Spiral Ring Renderer (Left Margin Binder)
    // =========================================================================
    drawSpiralCanvas() {
      const canvas = this.spiralCanvas;
      if (!canvas) return;
      const dpr = window.devicePixelRatio || 1;
      canvas.width = 38 * dpr;
      canvas.height = this.viewport.scrollHeight * dpr;
      canvas.style.width = '38px';
      canvas.style.height = `${this.viewport.scrollHeight}px`;

      const ctx = canvas.getContext('2d');
      ctx.scale(dpr, dpr);

      const pitch = 28.8;
      const totalRings = Math.ceil(this.viewport.scrollHeight / pitch);
      const spiralX = 18;

      ctx.clearRect(0, 0, 38, this.viewport.scrollHeight);

      for (let i = 0; i < totalRings; i++) {
        const y = i * pitch + 14;

        // Hole punch
        ctx.fillStyle = this.darkMode ? '#09090b' : '#dcd6c8';
        ctx.beginPath();
        ctx.arc(spiralX, y, 4.5, 0, Math.PI * 2);
        ctx.fill();

        // Metallic double wire
        ctx.strokeStyle = this.darkMode ? '#52525b' : '#8c857b';
        ctx.lineWidth = 1.8;
        ctx.beginPath();
        ctx.arc(spiralX - 4, y - 2, 7, -Math.PI / 2, Math.PI / 2);
        ctx.stroke();

        ctx.strokeStyle = this.darkMode ? '#71717a' : '#a8a29e';
        ctx.beginPath();
        ctx.arc(spiralX - 4, y + 2, 7, -Math.PI / 2, Math.PI / 2);
        ctx.stroke();
      }
    }

    // =========================================================================
    // 8. Popup Menus: Presets, Export, Songs, Context
    // =========================================================================
    showPresetMenu(e) {
      const rect = this.presetBtn.getBoundingClientRect();
      const menu = this.presetMenu;
      menu.style.left = `${rect.left}px`;
      menu.style.top = `${rect.bottom + 4}px`;
      menu.style.display = 'block';

      const templates = [
        { label: '4/4 Straight 16ths [4444]/4:4', notat: '[4444]/4:4' },
        { label: '3/4 Waltz [444]/3:3', notat: '[444]/3:3' },
        { label: '6/8 Shuffle [33]/2:2', notat: '[33]/2:2' },
        { label: 'Polymeter 4:3 Cross-flow [3333]/4:3', notat: '[3333]/4:3' },
        { label: 'Polymeter 6:4 Mixed [333222]/6:4', notat: '[333222]/6:4' },
        { label: '5/4 Quintuplet Cadence [55555]/5:5', notat: '[55555]/5:5' }
      ];

      let html = '<div class="popup-menu-header">Standard Flow Templates</div>';
      templates.forEach(t => {
        html += `<div class="popup-menu-item" data-notat="${t.notat}">${t.label}</div>`;
      });

      html += '<div class="popup-menu-divider"></div><div class="popup-menu-item" id="save-as-preset-action">+ Save Current As Preset...</div>';

      menu.innerHTML = html;

      menu.querySelectorAll('.popup-menu-item[data-notat]').forEach(item => {
        item.addEventListener('click', () => {
          this.globalNotation = MetricNotation.fromString(item.dataset.notat);
          this.applyGlobalNotation();
          menu.style.display = 'none';
        });
      });

      document.getElementById('save-as-preset-action')?.addEventListener('click', () => {
        menu.style.display = 'none';
        this.promptSavePreset();
      });
    }

    promptSavePreset() {
      const name = prompt('Enter name for custom metric preset:', 'My Custom Flow');
      if (name) {
        this.customPresets[name] = this.globalNotation.toString();
        this.savePresetBtn.textContent = 'Saved!';
        setTimeout(() => { this.savePresetBtn.textContent = 'Save'; }, 1200);
        this.saveStorage();
      }
    }

    showExportMenu(e) {
      const rect = this.exportBtn.getBoundingClientRect();
      const menu = this.exportMenu;
      menu.style.left = `${rect.left}px`;
      menu.style.top = `${rect.bottom + 4}px`;
      menu.style.display = 'block';

      menu.innerHTML = `
        <div class="popup-menu-item" id="export-copy-txt">Copy Formatted Text [xx] to Clipboard</div>
        <div class="popup-menu-item" id="export-save-txt">Open / Save Formatted Text (.txt)...</div>
        <div class="popup-menu-item" id="export-save-csv">Open / Save Spreadsheet Table (.csv)...</div>
        <div class="popup-menu-item" id="export-print-html">Open / Save Printable HTML Table (.html)...</div>
      `;

      document.getElementById('export-copy-txt').addEventListener('click', () => {
        this.copyLyrics();
        menu.style.display = 'none';
      });
      document.getElementById('export-save-txt').addEventListener('click', () => {
        this.downloadFile('lyrics.txt', this.generateFormattedText());
        menu.style.display = 'none';
      });
      document.getElementById('export-save-csv').addEventListener('click', () => {
        this.downloadFile('lyrics.csv', this.generateCsv());
        menu.style.display = 'none';
      });
      document.getElementById('export-print-html').addEventListener('click', () => {
        this.printHtmlTable();
        menu.style.display = 'none';
      });
    }

    showSongsMenu(e) {
      const rect = this.songsBtn.getBoundingClientRect();
      const menu = this.songsMenu;
      menu.style.left = `${rect.left}px`;
      menu.style.top = `${rect.bottom + 4}px`;
      menu.style.display = 'block';

      menu.innerHTML = `
        <div class="popup-menu-item" id="song-save-action">+ Save Current Song Project...</div>
        <div class="popup-menu-divider"></div>
        <div class="popup-menu-item" id="song-new-action">New Song (Clear Notepad)</div>
      `;

      document.getElementById('song-save-action').addEventListener('click', () => {
        const name = prompt('Enter Song Name:', this.tabs[this.activeTabIdx].title);
        if (name) {
          this.songProjects[name] = JSON.parse(JSON.stringify(this.tabs[this.activeTabIdx]));
          this.songsBtn.textContent = 'Saved!';
          setTimeout(() => { this.songsBtn.textContent = 'Songs ▼'; }, 1200);
          this.saveStorage();
        }
        menu.style.display = 'none';
      });

      document.getElementById('song-new-action').addEventListener('click', () => {
        if (confirm('Clear current song notepad?')) {
          this.tabs[this.activeTabIdx].bars = this.createBars(16, this.globalNotation);
          this.renderBars();
        }
        menu.style.display = 'none';
      });
    }

    showContextMenu(x, y, b, pIdx, sIdx) {
      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      const rhyme = SyllableSplitter.getRhymeKey(syl.text) || 'NONE';
      const menu = this.contextMenu;

      menu.style.left = `${Math.min(window.innerWidth - 240, x)}px`;
      menu.style.top = `${Math.min(window.innerHeight - 340, y)}px`;
      menu.style.display = 'block';

      menu.innerHTML = `
        <div class="popup-menu-header">Syllable: "${syl.text || ' '}" [${rhyme.toUpperCase()}]</div>
        <div class="swatch-row">
          ${PASTEL_PALETTE.slice(0, 7).map(c => `<div class="swatch-box" style="background:${c}" data-col="${c}"></div>`).join('')}
        </div>
        <div class="popup-menu-item" id="ctx-auto-color">Auto (Phoneme Rhyme Tint)</div>
        <div class="popup-menu-item" id="ctx-clear-color">Clear Highlight (None)</div>
        <div class="popup-menu-divider"></div>
        <div class="popup-menu-item" id="ctx-upper">UPPERCASE</div>
        <div class="popup-menu-item" id="ctx-lower">lowercase</div>
        <div class="popup-menu-item" id="ctx-capitalize">Capitalize Word</div>
        <div class="popup-menu-divider"></div>
        <div class="popup-menu-item" id="ctx-dup">Duplicate into Next Box</div>
        <div class="popup-menu-item" id="ctx-bold">Bold Emphasis (Ctrl+B)</div>
      `;

      menu.querySelectorAll('.swatch-box').forEach(sw => {
        sw.addEventListener('click', () => {
          syl.customColor = sw.dataset.col;
          this.updateCellAppearance(b, pIdx, sIdx);
          menu.style.display = 'none';
        });
      });

      document.getElementById('ctx-auto-color').addEventListener('click', () => {
        syl.customColor = null;
        this.updateCellAppearance(b, pIdx, sIdx);
        menu.style.display = 'none';
      });
      document.getElementById('ctx-clear-color').addEventListener('click', () => {
        syl.customColor = 'transparent';
        this.updateCellAppearance(b, pIdx, sIdx);
        menu.style.display = 'none';
      });
      document.getElementById('ctx-upper').addEventListener('click', () => {
        syl.text = syl.text.toUpperCase();
        this.updateCellAppearance(b, pIdx, sIdx);
        menu.style.display = 'none';
      });
      document.getElementById('ctx-lower').addEventListener('click', () => {
        syl.text = syl.text.toLowerCase();
        this.updateCellAppearance(b, pIdx, sIdx);
        menu.style.display = 'none';
      });
      document.getElementById('ctx-capitalize').addEventListener('click', () => {
        syl.text = syl.text.charAt(0).toUpperCase() + syl.text.slice(1).toLowerCase();
        this.updateCellAppearance(b, pIdx, sIdx);
        menu.style.display = 'none';
      });
      document.getElementById('ctx-dup').addEventListener('click', () => {
        this.advanceCellFocus(b, pIdx, sIdx, 1);
        if (this.focusedCellCoords) {
          const nextSyl = this.tabs[this.activeTabIdx].bars[this.focusedCellCoords.barIdx].syllables[this.focusedCellCoords.pulseIdx][this.focusedCellCoords.sylIdx];
          nextSyl.text = syl.text;
          nextSyl.bold = syl.bold;
          nextSyl.align = syl.align;
          nextSyl.customColor = syl.customColor;
          this.updateCellAppearance(this.focusedCellCoords.barIdx, this.focusedCellCoords.pulseIdx, this.focusedCellCoords.sylIdx);
        }
        menu.style.display = 'none';
      });
      document.getElementById('ctx-bold').addEventListener('click', () => {
        syl.bold = !syl.bold;
        this.updateCellAppearance(b, pIdx, sIdx);
        menu.style.display = 'none';
      });
    }

    showRowAddMenu(e, b) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      const menu = this.contextMenu;
      menu.style.left = `${e.clientX - 180}px`;
      menu.style.top = `${e.clientY - 60}px`;
      menu.style.display = 'block';

      menu.innerHTML = `
        <div class="popup-menu-item" id="row-add-line">Add new line (w same meter)</div>
        <div class="popup-menu-item" id="row-toggle-break">${bar.isStanzaBreak ? 'Remove stanza break' : 'Add stanza break'}</div>
      `;

      document.getElementById('row-add-line').addEventListener('click', () => {
        const newBar = this.createSingleBar(bar.notation, false);
        this.tabs[this.activeTabIdx].bars.splice(b + 1, 0, newBar);
        this.renderBars();
        menu.style.display = 'none';
      });

      document.getElementById('row-toggle-break').addEventListener('click', () => {
        bar.isStanzaBreak = !bar.isStanzaBreak;
        this.renderBars();
        menu.style.display = 'none';
      });
    }

    promptBarMeter(b) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      const val = prompt('Customize meter for this bar (e.g. [4444]/4:4, [333222]/6:4):', bar.notation.toString());
      if (val) {
        bar.notation = MetricNotation.fromString(val);
        this.reconcileBarSyllables(bar);
        this.renderBars();
      }
    }

    // =========================================================================
    // 9. Lyrics Export & Storage
    // =========================================================================
    copyLyrics() {
      const text = this.generateFormattedText();
      navigator.clipboard.writeText(text).then(() => {
        this.copyBtn.textContent = 'Copied!';
        setTimeout(() => { this.copyBtn.textContent = 'Copy'; }, 1200);
      });
    }

    generateFormattedText() {
      const tab = this.tabs[this.activeTabIdx];
      return tab.bars.map(bar => {
        const words = bar.syllables.flat().map(s => s.text).filter(Boolean).join(' ');
        const count = bar.spokenSyllables !== null ? bar.spokenSyllables : bar.syllables.flat().filter(s => s.text).length;
        const line = words ? `${words} [${count}]` : '';
        return bar.isStanzaBreak ? `${line}\n` : line;
      }).filter(Boolean).join('\n');
    }

    generateCsv() {
      const tab = this.tabs[this.activeTabIdx];
      const rows = ['Bar,Metric Schema,Spoken Syllables,Grid Syllables,Lyrics'];
      tab.bars.forEach((bar, idx) => {
        const words = bar.syllables.flat().map(s => s.text).filter(Boolean).join(' ');
        const spoken = bar.spokenSyllables !== null ? bar.spokenSyllables : bar.syllables.flat().filter(s => s.text).length;
        const grid = bar.notation.getTotalSyllables();
        rows.push(`${idx + 1},"${bar.notation.toString()}",${spoken},${grid},"${words.replace(/"/g, '""')}"`);
      });
      return rows.join('\n');
    }

    printHtmlTable() {
      const win = window.open('', '_blank');
      const text = this.generateFormattedText();
      win.document.write(`<html><head><title>Lyrics Export</title><style>body{font-family:sans-serif;padding:30px;white-space:pre-wrap;}</style></head><body><h1>${this.tabs[this.activeTabIdx].title}</h1><pre>${text}</pre></body></html>`);
      win.document.close();
      win.print();
    }

    downloadFile(filename, content) {
      const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = filename;
      a.click();
      URL.revokeObjectURL(url);
    }

    saveStorage() {
      try {
        localStorage.setItem('compass_cadence_vst_pwa_state', JSON.stringify({
          tabs: this.tabs,
          activeTabIdx: this.activeTabIdx,
          darkMode: this.darkMode,
          bpm: this.audio.bpm,
          customPresets: this.customPresets,
          songProjects: this.songProjects
        }));
      } catch (e) {
        console.warn('Storage save failed:', e);
      }
    }

    loadStorage() {
      try {
        const data = JSON.parse(localStorage.getItem('compass_cadence_vst_pwa_state'));
        if (data && data.tabs && data.tabs.length) {
          this.tabs = data.tabs.map(t => ({
            title: t.title,
            bars: t.bars.map(b => ({
              notation: new MetricNotation(b.notation.pulseSubdivs, b.notation.beatsPerBar),
              syllables: b.syllables,
              spokenSyllables: b.spokenSyllables,
              isStanzaBreak: !!b.isStanzaBreak
            }))
          }));
          this.activeTabIdx = data.activeTabIdx || 0;
          this.darkMode = !!data.darkMode;
          if (data.bpm) this.audio.setBpm(data.bpm);
          this.customPresets = data.customPresets || {};
          this.songProjects = data.songProjects || {};
          document.body.classList.toggle('dark-mode', this.darkMode);
        }
      } catch (e) {
        console.warn('Storage load skipped:', e);
      }
    }
  }

  // Launch on DOM Ready
  window.addEventListener('DOMContentLoaded', () => {
    window.cadenceApp = new CompassCadenceApp();

    if ('serviceWorker' in navigator) {
      navigator.serviceWorker.register('./sw.js').catch(err => {
        console.warn('SW registration skipped:', err);
      });
    }
  });
})();
