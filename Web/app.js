/**
 * Compass Cadence — Full 1:1 VST3 Feature Engine & Notepad
 * Virtual Studio Technology (VST) Lyric & Cross-Rhythm Notepad
 */

(function () {
  'use strict';

  // =========================================================================
  // 1. Metric Cross-Rhythm Notation Model
  // =========================================================================
  class MetricNotation {
    constructor(pulseSubdivs = [3, 3, 3, 2, 2, 2], beats = 4) {
      this.pulseSubdivs = [...pulseSubdivs];
      this.beatsPerBar = beats;
    }

    static fromString(str, fallbackBeats = 4) {
      if (!str) return new MetricNotation([3, 3, 3, 2, 2, 2], 4);
      const match = str.trim().match(/^\[([0-9]+)\](?:\/([0-9]+):([0-9]+))?/);
      if (!match) return new MetricNotation([3, 3, 3, 2, 2, 2], 4);
      const digits = match[1].split('').map(Number);
      const beats = match[3] ? parseInt(match[3], 10) : (match[2] ? parseInt(match[2], 10) : fallbackBeats);
      return new MetricNotation(digits.length ? digits : [3, 3, 3, 2, 2, 2], beats || 4);
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
  // 2. Syllable & Phoneme Engine (CMUDict Rhyme Classifier)
  // =========================================================================
  class RhymeClassifier {
    static extractRhymeKey(word) {
      if (!word) return '';
      let clean = word.toLowerCase().replace(/[^a-z']/g, '');
      if (!clean) return '';
      if (clean.endsWith('-')) clean = clean.slice(0, -1);

      // Known dictionary mappings for rap lyrics
      const dict = {
        'to': 'UW', 'you': 'UW', 'through': 'UW', 'true': 'UW', 'lu': 'UW',
        'no': 'OW', 'so': 'OW', 'go': 'OW', 'pro': 'OW', 'bo': 'OW',
        'set': 'EH_T', 'let': 'EH_T', 'get': 'EH_T',
        'out': 'AW_T', 'about': 'AW_T', 'doubt': 'AW_T',
        'put': 'UH_T', 'foot': 'UH_T',
        'all': 'AO_L', 'fall': 'AO_L', 'call': 'AO_L',
        'down': 'AW_N', 'town': 'AW_N',
        'pa': 'AA', 'per': 'ER', 'were': 'ER', 'her': 'ER',
        'way': 'EY', 'say': 'EY', 'they': 'EY', 'lay': 'EY',
        'side': 'AY_D', 'hide': 'AY_D',
        'space': 'EY_S', 'place': 'EY_S',
        'hand': 'AE_N_D', 'stand': 'AE_N_D',
        'if': 'IH_F',
        'see': 'IY', 'be': 'IY', 'me': 'IY', 'we': 'IY',
        'great': 'EY_T', 'hate': 'EY_T',
        'prob': 'AA_B', 'ob': 'AA_B',
        'head': 'EH_D', 'bed': 'EH_D',
        'read': 'IY_D',
        'form': 'AO_R_M',
        'core': 'AO_R',
        'sub': 'AH_B',
        'box': 'AA_K_S',
        'sor': 'AO_R',
        'ry': 'IY',
        'worth': 'ER_TH',
        'saying': 'EY_IH_NG',
        'fra': 'EY',
        'ming': 'IH_NG',
        'cash': 'AE_SH',
        'drugs': 'AH_G_Z',
        'hands': 'AE_N_D_Z',
        'means': 'IY_N_Z',
        'fun': 'AH_N',
        'cap': 'AE_P'
      };

      if (dict[clean]) return dict[clean];

      // Fallback phonetic suffix
      const match = clean.match(/([aeiouy]+[^aeiouy]*)$/i);
      return match ? match[1].toUpperCase() : clean.slice(-2).toUpperCase();
    }
  }

  // 7 Stationery Pastel Swatches matching VST3 exactly
  const PASTEL_SWATCHES = [
    { name: 'Pastel Mint', color: 'rgba(134, 239, 172, 0.65)' },
    { name: 'Pastel Sky', color: 'rgba(147, 197, 253, 0.65)' },
    { name: 'Pastel Peach', color: 'rgba(254, 215, 170, 0.65)' },
    { name: 'Pastel Lavender', color: 'rgba(221, 214, 254, 0.65)' },
    { name: 'Pastel Butter', color: 'rgba(254, 240, 138, 0.65)' },
    { name: 'Pastel Rose', color: 'rgba(254, 205, 211, 0.65)' },
    { name: 'Slate Gray', color: 'rgba(203, 213, 225, 0.65)' }
  ];

  // =========================================================================
  // 3. Web Audio Standalone Metronome Synthesizer
  // =========================================================================
  class MetronomeEngine {
    constructor() {
      this.ctx = null;
      this.bpm = 111.0;
      this.isPlaying = false;
      this.timerId = null;
      this.clickEnabled = true;
      this.currentBar = 0;
      this.currentPulse = 0;
      this.onTick = null;
    }

    initAudio() {
      if (!this.ctx) {
        const AudioContext = window.AudioContext || window.webkitAudioContext;
        this.ctx = new AudioContext();
      }
      if (this.ctx.state === 'suspended') {
        this.ctx.resume();
      }
    }

    start(onTickCallback) {
      this.initAudio();
      this.isPlaying = true;
      this.onTick = onTickCallback;
      this.currentBar = 0;
      this.currentPulse = 0;
      this.scheduleNext();
    }

    stop() {
      this.isPlaying = false;
      if (this.timerId) clearTimeout(this.timerId);
      this.timerId = null;
    }

    playClick(isDownbeat) {
      if (!this.clickEnabled || !this.ctx) return;
      try {
        const osc = this.ctx.createOscillator();
        const gain = this.ctx.createGain();

        // 1600 Hz for downbeat (pulse 0), 1000 Hz for offbeat
        osc.frequency.value = isDownbeat ? 1600 : 1000;
        osc.type = 'sine';

        const now = this.ctx.currentTime;
        gain.gain.setValueAtTime(0.28, now);
        gain.gain.exponentialRampToValueAtTime(0.0001, now + 0.04);

        osc.connect(gain);
        gain.connect(this.ctx.destination);

        osc.start(now);
        osc.stop(now + 0.045);
      } catch (e) {}
    }

    scheduleNext() {
      if (!this.isPlaying) return;

      const isDownbeat = (this.currentPulse === 0);
      this.playClick(isDownbeat);

      if (this.onTick) {
        this.onTick(this.currentBar, this.currentPulse);
      }

      // 6 pulses across 4 beats at current BPM
      const beatDurationSec = 60.0 / this.bpm;
      const barDurationSec = beatDurationSec * 4.0;
      const pulseDurationMs = (barDurationSec / 6.0) * 1000.0;

      this.currentPulse++;
      if (this.currentPulse >= 6) {
        this.currentPulse = 0;
        this.currentBar++;
      }

      this.timerId = setTimeout(() => this.scheduleNext(), pulseDurationMs);
    }
  }

  // =========================================================================
  // 4. Initial Lyrics Seed (Exact VST Session from User Screenshot)
  // =========================================================================
  const INITIAL_RAP_LYRICS = [
    // Bar 01
    [
      [{ text: 'if' }, { text: 'i' }, { text: 'were' }],
      [{ text: 'to' }, { text: 'set' }, { text: 'out' }],
      [{ text: 'to' }, { text: 'put' }, { text: 'it' }],
      [{ text: 'all' }, { text: 'down' }],
      [{ text: 'to' }, { text: 'pa' }],
      [{ text: 'per' }, { text: 'to' }]
    ],
    // Bar 02
    [
      [{ text: 'let' }, { text: 'it' }, { text: 'all' }],
      [{ text: 'out', bold: true }, { text: 'in' }, { text: 'a' }],
      [{ text: 'way', bold: true }, { text: 'that' }, { text: 'was' }],
      [{ text: 'un', bold: true }, { text: 'der' }],
      [{ text: 'stand', bold: true }, { text: 'a' }],
      [{ text: 'ble', bold: true }, { text: 'and' }]
    ],
    // Bar 03
    [
      [{ text: 'put' }, { text: 'it' }, { text: 'to' }],
      [{ text: 'ca', bold: true }, { text: 'dence' }, { text: 'in' }],
      [{ text: 'side', bold: true }, { text: 'a' }, { text: 'space', bold: true }],
      [{ text: 'they' }, { text: 'could' }],
      [{ text: 'hand', bold: true }, { text: 'le' }],
      [{ text: 'and' }, { text: 'if', bold: true }]
    ],
    // Bar 04
    [
      [{ text: 'quick' }, { text: 'e' }, { text: 'nough' }],
      [{ text: 'brave' }, { text: 'e' }, { text: 'nough' }],
      [{ text: 'now' }, { text: 'with' }, { text: 'some' }],
      [{ text: 'thing' }, { text: 'to' }],
      [{ text: 'say' }, { text: 'to' }],
      [{ text: 'them' }, { text: 'all' }]
    ],
    // Bar 05 (Stanza Break before this)
    [
      [{ text: 'makes', bold: true }, { text: 'me' }, { text: 'so' }],
      [{ text: 'ang', bold: true }, { text: 'ry' }, { text: 'to' }],
      [{ text: 'sit', bold: true }, { text: 'in' }, { text: 'this' }],
      [{ text: 'place', bold: true }, { text: 'and' }],
      [{ text: 'think', bold: true }, { text: 'of' }],
      [{ text: 'all', bold: true }, { text: 'the' }]
    ],
    // Bar 06
    [
      [{ text: 'dis', bold: true }, { text: 'tance' }, { text: 'be' }],
      [{ text: 'tween', bold: true }, { text: 'me' }, { text: 'and' }],
      [{ text: 'eve', bold: true }, { text: 'ry' }, { text: 'thing' }],
      [{ text: 'great', bold: true }, { text: 'i' }],
      [{ text: 'hope' }, { text: 'to' }],
      [{ text: 'be', bold: true }, { text: 'but' }]
    ],
    // Bar 07
    [
      [{ text: 'if' }, { text: 'i' }, { text: 'could' }],
      [{ text: 'read', bold: true }, { text: 'it' }, { text: 'ob' }],
      [{ text: 'ject', bold: true }, { text: 'ive' }, { text: 'ly' }],
      [{ text: 'i' }, { text: 'would' }],
      [{ text: 'prob', bold: true }, { text: 'ly' }],
      [{ text: 'see', bold: true }, { text: 'that' }]
    ],
    // Bar 08
    [
      [{ text: 'all' }, { text: 'that' }, { text: 'is' }],
      [{ text: 'left', bold: true }, { text: 'for' }, { text: 'me' }],
      [{ text: 'now', bold: true }, { text: 'is' }, { text: 'the' }],
      [{ text: 'ex', bold: true }, { text: 'e' }],
      [{ text: 'cu', bold: true }, { text: 'tion' }],
      [{ text: 'toward' }, { text: 'the' }]
    ],
    // Bar 09
    [
      [{ text: 'ver', bold: true }, { text: 'y' }, { text: 'dir' }],
      [{ text: 'ec', bold: true }, { text: 'tion' }, { text: "i'm" }],
      [{ text: 'al', bold: true }, { text: 'read' }, { text: 'y' }],
      [{ text: 'head', bold: true }, { text: 'ed' }],
      [{ text: 'no' }, { text: 'pivot', bold: true }],
      [{ text: 'for', bold: true }, { text: 'it' }]
    ],
    // Bar 10
    [
      [{ text: 'i' }, { text: 'wish' }, { text: 'that' }],
      [{ text: 'it', bold: true }, { text: 'could' }, { text: 'be' }],
      [{ text: 'simp', bold: true }, { text: 'le' }, { text: 'to' }],
      [{ text: 'see' }, { text: 'my' }],
      [{ text: 'form', bold: true }, { text: 'at' }],
      [{ text: 'core', bold: true }, { text: 'as' }]
    ],
    // Bar 11
    [
      [{ text: 'simp', bold: true }, { text: 'ly' }, { text: 'in' }],
      [{ text: 'form', bold: true }, { text: 'ing' }, { text: 'my' }],
      [{ text: 'be', bold: true }, { text: 'ing' }, { text: 'in' }],
      [{ text: 'stead' }, { text: 'of' }],
      [{ text: 'for', bold: true }, { text: 'eign' }],
      [{ text: 'sub', bold: true }, { text: 'stance' }]
    ],
    // Bar 12 (14/15 syllables: last slot is empty!)
    [
      [{ text: 'sub', bold: true }, { text: 'stan' }, { text: 'ti' }],
      [{ text: 'a', bold: true }, { text: 'ting' }, { text: 'the' }],
      [{ text: 'suffe', bold: true }, { text: 'ring' }, { text: 'that' }],
      [{ text: 'i' }, { text: 'am' }],
      [{ text: 'sub', bold: true }, { text: 'ject' }],
      [{ text: 'to' }, { text: '' }]
    ],
    // Bar 13
    [
      [{ text: 'watch', bold: true }, { text: 'me' }, { text: 'dis' }],
      [{ text: 'man', bold: true }, { text: 'tle' }, { text: 'your' }],
      [{ text: 'box', bold: true }, { text: 'es' }, { text: "i'm" }],
      [{ text: 'sor', bold: true }, { text: 'ry' }],
      [{ text: 'lu', bold: true }, { text: 'pe' }],
      [{ text: 'but', bold: true }, { text: 'i' }]
    ],
    // Bar 14
    [
      [{ text: 'think', bold: true }, { text: "there's" }, { text: 'some' }],
      [{ text: 'shit', bold: true }, { text: "that's" }, { text: 'worth' }],
      [{ text: 'say', bold: true }, { text: 'ing' }, { text: 'be' }],
      [{ text: 'yond' }, { text: 're' }],
      [{ text: 'cur' }, { text: 'sive' }],
      [{ text: 'fra', bold: true }, { text: 'ming' }]
    ],
    // Bar 15
    [
      [{ text: 'if' }, { text: 'all' }, { text: 'we' }],
      [{ text: 'had', bold: true }, { text: 'was' }, { text: 'se' }],
      [{ text: 'man', bold: true }, { text: 'tic' }, { text: 'gram', bold: true }],
      [{ text: 'mar' }, { text: 'for' }],
      [{ text: 'mean', bold: true }, { text: 'ing' }],
      [{ text: 'mak', bold: true }, { text: 'ing' }]
    ],
    // Bar 16
    [
      [{ text: 'and' }, { text: 'all' }, { text: 'we' }],
      [{ text: 'were', bold: true }, { text: 'was' }, { text: 'cash' }],
      [{ text: 'vio', bold: true }, { text: 'lence' }, { text: 'sex', bold: true }],
      [{ text: 'drugs' }, { text: 'hy' }],
      [{ text: 'per', bold: true }, { text: 'bo' }],
      [{ text: 'le' }, { text: 'and' }]
    ],
    // Bar 17
    [
      [{ text: 'e' }, { text: 'mo', bold: true }, { text: 'tions' }],
      [{ text: 'or' }, { text: 'what', bold: true }, { text: 'you' }],
      [{ text: 'could' }, { text: 'count', bold: true }, { text: 'on' }],
      [{ text: 'two' }, { text: 'hands', bold: true }],
      [{ text: 'of' }, { text: 'what', bold: true }],
      [{ text: 'it' }, { text: 'means', bold: true }]
    ],
    // Bar 18
    [
      [{ text: 'to' }, { text: 'wake', bold: true }, { text: 'up' }],
      [{ text: 'a' }, { text: 'per', bold: true }, { text: 'son' }],
      [{ text: 'and' }, { text: 'go', bold: true }, { text: 'to' }],
      [{ text: 'bed', bold: true }, { text: 'as' }],
      [{ text: 'a' }, { text: 'hu', bold: true }],
      [{ text: 'man' }, { text: 'being', bold: true }]
    ],
    // Bar 19
    [
      [{ text: 'it' }, { text: 'might', bold: true }, { text: 'be' }],
      [{ text: 'more' }, { text: 'simp', bold: true }, { text: 'le' }],
      [{ text: 'to' }, { text: 'lay', bold: true }, { text: 'it' }],
      [{ text: 'out', bold: true }, { text: 'on' }],
      [{ text: 'a' }, { text: 'mi', bold: true }],
      [{ text: 'cro' }, { text: 'phone' }]
    ],
    // Bar 20
    [
      [{ text: 'but' }, { text: 'i' }, { text: 'find', bold: true }],
      [{ text: 'it' }, { text: 're' }, { text: 'duc', bold: true }],
      [{ text: 'tive' }, { text: 'to' }, { text: 'try' }],
      [{ text: 'to' }, { text: 'cap', bold: true }],
      [{ text: 'out' }, { text: 'the' }],
      [{ text: 'fun', bold: true }, { text: 'of' }]
    ]
  ];

  // =========================================================================
  // 5. Core VST Notepad App Controller
  // =========================================================================
  class CompassCadenceApp {
    constructor() {
      this.globalNotation = new MetricNotation([3, 3, 3, 2, 2, 2], 4);
      this.audio = new MetronomeEngine();
      this.totalBars = 320;
      this.barsPerPage = 16;
      this.currentPage = 0;
      this.viewMode = 'scroll'; // 'scroll' (default) or 'pages'
      this.darkMode = true;
      this.rhymesEnabled = false;
      this.followDAW = true;
      this.activeTabIdx = 0;

      this.tabs = [
        {
          title: 'Main',
          bars: []
        }
      ];

      this.undoStack = [];
      this.redoStack = [];
      this.selectedCells = new Set();
      this.activeEditor = null; // { b, pIdx, sIdx, inputEl }

      this.initBars();
      this.cacheDOMElements();
      this.bindUI();
      this.renderTabs();
      this.renderPage();
      this.drawSpiralCanvas();
      window.addEventListener('resize', () => this.drawSpiralCanvas());
    }

    initBars() {
      const bars = [];
      for (let b = 0; b < this.totalBars; b++) {
        const notat = this.globalNotation.clone();
        const syllables = [];

        // Build pulse boxes
        notat.pulseSubdivs.forEach((count) => {
          const pulseArr = [];
          for (let s = 0; s < count; s++) {
            pulseArr.push({
              text: '',
              bold: false,
              align: 'center',
              customColor: null
            });
          }
          syllables.push(pulseArr);
        });

        // Initial lyrics seeding removed to start with a clean slate

        bars.push({
          notation: notat,
          syllables: syllables,
          customSpokenCount: (b === 11) ? 14 : null, // Bar 12 has 14/15
          stanzaBreak: ((b + 1) % 4 === 0)
        });
      }
      this.tabs[0].bars = bars;
    }

    pushSnapshot() {
      const state = JSON.stringify(this.tabs[this.activeTabIdx].bars);
      this.undoStack.push(state);
      if (this.undoStack.length > 50) this.undoStack.shift();
      this.redoStack = [];
    }

    undo() {
      if (this.undoStack.length === 0) return;
      const current = JSON.stringify(this.tabs[this.activeTabIdx].bars);
      this.redoStack.push(current);
      const prev = this.undoStack.pop();
      this.tabs[this.activeTabIdx].bars = JSON.parse(prev);
      this.renderPage();
    }

    redo() {
      if (this.redoStack.length === 0) return;
      const current = JSON.stringify(this.tabs[this.activeTabIdx].bars);
      this.undoStack.push(current);
      const next = this.redoStack.pop();
      this.tabs[this.activeTabIdx].bars = JSON.parse(next);
      this.renderPage();
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
      this.dawStatusLabel = document.getElementById('daw-status-label');

      this.presetMenu = document.getElementById('preset-menu');
      this.exportMenu = document.getElementById('export-menu');
      this.songsMenu = document.getElementById('songs-menu');
      this.contextMenu = document.getElementById('context-menu');
      this.rowAddMenu = document.getElementById('row-add-menu');
    }

    bindUI() {
      // Metric Grid Text Input
      this.metricInput.addEventListener('change', (e) => {
        this.pushSnapshot();
        this.globalNotation = MetricNotation.fromString(e.target.value, this.globalNotation.beatsPerBar);
        this.updateHeader();
        this.applyGlobalNotation();
      });

      // Pulses Steppers
      document.getElementById('dec-pulse-btn').addEventListener('click', () => {
        if (this.globalNotation.pulseSubdivs.length > 1) {
          this.pushSnapshot();
          this.globalNotation.pulseSubdivs.pop();
          this.updateHeader();
          this.applyGlobalNotation();
        }
      });
      document.getElementById('inc-pulse-btn').addEventListener('click', () => {
        this.pushSnapshot();
        this.globalNotation.pulseSubdivs.push(3);
        this.updateHeader();
        this.applyGlobalNotation();
      });

      // Beats Steppers
      document.getElementById('dec-beat-btn').addEventListener('click', () => {
        if (this.globalNotation.beatsPerBar > 1) {
          this.pushSnapshot();
          this.globalNotation.beatsPerBar--;
          this.updateHeader();
          this.applyGlobalNotation();
        }
      });
      document.getElementById('inc-beat-btn').addEventListener('click', () => {
        this.pushSnapshot();
        this.globalNotation.beatsPerBar++;
        this.updateHeader();
        this.applyGlobalNotation();
      });

      // Presets & Save
      this.presetBtn.addEventListener('click', (e) => this.showPresetMenu(e));
      this.savePresetBtn.addEventListener('click', () => this.promptSavePreset());

      // DAW Status click toggles Play/Stop
      this.dawStatusLabel.addEventListener('click', () => this.togglePlayback());

      // Rhymes Toggle
      this.rhymeToggleBtn.addEventListener('click', () => {
        this.rhymesEnabled = !this.rhymesEnabled;
        this.rhymeToggleBtn.classList.toggle('toggled', this.rhymesEnabled);
        this.rhymeToggleBtn.textContent = this.rhymesEnabled ? 'Rhymes: ON' : 'Rhymes: OFF';
        this.renderPage();
      });

      // Follow DAW Toggle
      this.followToggleBtn.addEventListener('click', () => {
        this.followDAW = !this.followDAW;
        this.followToggleBtn.classList.toggle('toggled', this.followDAW);
        this.followToggleBtn.textContent = this.followDAW ? 'Follow DAW: ON' : 'Follow DAW: OFF';
      });

      // Copy & Export & Songs
      this.copyBtn.addEventListener('click', () => this.copyLyrics());
      this.exportBtn.addEventListener('click', (e) => this.showExportMenu(e));
      this.songsBtn.addEventListener('click', (e) => this.showSongsMenu(e));

      // Dark Mode Toggle
      this.darkModeBtn.addEventListener('click', () => {
        this.darkMode = !this.darkMode;
        document.body.classList.toggle('dark-mode', this.darkMode);
        this.darkModeBtn.classList.toggle('toggled', this.darkMode);
        this.darkModeBtn.textContent = this.darkMode ? 'Dark: ON' : 'Dark: OFF';
        this.drawSpiralCanvas();
      });

      // Scroll vs Pages Mode
      this.viewModeBtn.addEventListener('click', () => {
        this.viewMode = this.viewMode === 'scroll' ? 'pages' : 'scroll';
        this.viewModeBtn.textContent = this.viewMode === 'scroll' ? 'Scroll' : 'Pages';
        this.renderPage();
      });

      // Navigation Buttons < >
      this.prevPageBtn.addEventListener('click', () => {
        if (this.viewMode === 'scroll') {
          this.viewport.scrollBy({ top: -500, behavior: 'smooth' });
        } else if (this.currentPage > 0) {
          this.currentPage--;
          this.renderPage();
        }
      });
      this.nextPageBtn.addEventListener('click', () => {
        if (this.viewMode === 'scroll') {
          this.viewport.scrollBy({ top: 500, behavior: 'smooth' });
        } else {
          this.currentPage++;
          this.renderPage();
        }
      });

      // Global Dismiss Popup Menus on Click Outside
      document.addEventListener('click', (e) => {
        [this.presetMenu, this.exportMenu, this.songsMenu, this.contextMenu, this.rowAddMenu].forEach((m) => {
          if (m && !m.contains(e.target)) m.style.display = 'none';
        });
      });

      // Global Spacebar Transport Toggle (when not actively editing a cell)
      document.addEventListener('keydown', (e) => {
        if (e.code === 'Space' && !this.activeEditor) {
          e.preventDefault();
          this.togglePlayback();
        }
        if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
          if (!this.activeEditor) { e.preventDefault(); this.undo(); }
        }
        if (((e.ctrlKey || e.metaKey) && e.key === 'y') || ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'Z')) {
          if (!this.activeEditor) { e.preventDefault(); this.redo(); }
        }
      });
    }

    updateHeader() {
      this.metricInput.value = this.globalNotation.toString();
      this.pulsesVal.textContent = this.globalNotation.pulseSubdivs.length;
      this.beatsVal.textContent = this.globalNotation.beatsPerBar;
    }

    applyGlobalNotation() {
      const bars = this.tabs[this.activeTabIdx].bars;
      bars.forEach((bar) => {
        bar.notation = this.globalNotation.clone();
        // Reconcile pulse subdivisions
        const newSyllables = [];
        this.globalNotation.pulseSubdivs.forEach((subdivCount, p) => {
          const pulseArr = [];
          for (let s = 0; s < subdivCount; s++) {
            const oldSyl = (bar.syllables[p] && bar.syllables[p][s]) ? bar.syllables[p][s] : null;
            pulseArr.push({
              text: oldSyl ? oldSyl.text : '',
              bold: oldSyl ? oldSyl.bold : false,
              align: oldSyl ? oldSyl.align : 'center',
              customColor: oldSyl ? oldSyl.customColor : null
            });
          }
          newSyllables.push(pulseArr);
        });
        bar.syllables = newSyllables;
      });
      this.renderPage();
    }

    togglePlayback() {
      if (this.audio.isPlaying) {
        this.audio.stop();
        document.querySelectorAll('.playhead-arrow').forEach(el => el.classList.remove('active'));
      } else {
        this.audio.start((barIdx, pulseIdx) => {
          document.querySelectorAll('.playhead-arrow').forEach((el, idx) => {
            el.classList.toggle('active', idx === barIdx);
          });
          if (this.followDAW) {
            const rowEl = document.getElementById(`bar-row-${barIdx}`);
            if (rowEl) {
              rowEl.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
            }
          }
        });
      }
      const state = this.audio.isPlaying ? '[PLAYING]' : '[STOPPED]';
      this.dawStatusLabel.innerHTML = `${this.audio.bpm.toFixed(1)} BPM &nbsp;|&nbsp; ${this.globalNotation.beatsPerBar}/4 &nbsp;|&nbsp; ${state}`;
    }

    // =========================================================================
    // 6. Tabs View
    // =========================================================================
    renderTabs() {
      this.tabsStrip.innerHTML = '';
      this.tabs.forEach((tab, idx) => {
        const tabEl = document.createElement('div');
        tabEl.className = `notebook-tab ${idx === this.activeTabIdx ? 'active' : ''}`;
        tabEl.innerHTML = `
          <span>${tab.title}</span>
          ${this.tabs.length > 1 ? '<span class="close-tab">&times;</span>' : ''}
        `;
        tabEl.addEventListener('click', (e) => {
          if (e.target.classList.contains('close-tab')) {
            e.stopPropagation();
            this.closeTab(idx);
          } else {
            this.activeTabIdx = idx;
            this.renderTabs();
            this.renderPage();
          }
        });
        tabEl.addEventListener('dblclick', () => {
          const newName = prompt('Rename Tab:', tab.title);
          if (newName) { tab.title = newName.trim(); this.renderTabs(); }
        });
        this.tabsStrip.appendChild(tabEl);
      });

      const addBtn = document.createElement('button');
      addBtn.id = 'add-tab-btn';
      addBtn.textContent = '+';
      addBtn.title = 'Add new tab or duplicate';
      addBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        this.showTabMenu(addBtn);
      });
      this.tabsStrip.appendChild(addBtn);
    }

    showTabMenu(anchorBtn) {
      const rect = anchorBtn.getBoundingClientRect();
      this.rowAddMenu.style.left = `${rect.left}px`;
      this.rowAddMenu.style.top = `${rect.bottom + 4}px`;
      this.rowAddMenu.innerHTML = `
        <div class="popup-menu-item" id="tab-new-blank">New Blank Song / Tab</div>
        <div class="popup-menu-item" id="tab-duplicate">Duplicate Current Tab (Unsynced)</div>
      `;
      this.rowAddMenu.style.display = 'block';

      document.getElementById('tab-new-blank').onclick = () => {
        this.rowAddMenu.style.display = 'none';
        this.tabs.push({ title: `Song ${this.tabs.length + 1}`, bars: this.createEmptyBars() });
        this.activeTabIdx = this.tabs.length - 1;
        this.renderTabs();
        this.renderPage();
      };
      document.getElementById('tab-duplicate').onclick = () => {
        this.rowAddMenu.style.display = 'none';
        const clone = JSON.parse(JSON.stringify(this.tabs[this.activeTabIdx].bars));
        this.tabs.push({ title: `${this.tabs[this.activeTabIdx].title} (Alt)`, bars: clone });
        this.activeTabIdx = this.tabs.length - 1;
        this.renderTabs();
        this.renderPage();
      };
    }

    createEmptyBars() {
      const bars = [];
      for (let b = 0; b < this.totalBars; b++) {
        const notat = this.globalNotation.clone();
        const syllables = [];
        notat.pulseSubdivs.forEach((count) => {
          const pulseArr = [];
          for (let s = 0; s < count; s++) {
            pulseArr.push({ text: '', bold: false, align: 'center', customColor: null });
          }
          syllables.push(pulseArr);
        });
        bars.push({
          notation: notat,
          syllables: syllables,
          customSpokenCount: null,
          stanzaBreak: ((b + 1) % 4 === 0)
        });
      }
      return bars;
    }

    closeTab(idx) {
      if (this.tabs.length <= 1) return;
      this.tabs.splice(idx, 1);
      this.activeTabIdx = Math.min(this.activeTabIdx, this.tabs.length - 1);
      this.renderTabs();
      this.renderPage();
    }

    // =========================================================================
    // 7. Render Notepad Page & Rows
    // =========================================================================
    renderPage() {
      this.pageContainer.innerHTML = '';
      const tab = this.tabs[this.activeTabIdx];

      let startBar = 0;
      let endBar = tab.bars.length;
      if (this.viewMode === 'pages') {
        startBar = this.currentPage * this.barsPerPage;
        endBar = Math.min(tab.bars.length, startBar + this.barsPerPage);
        this.pageLabel.textContent = `Page ${this.currentPage + 1} / ${Math.ceil(tab.bars.length / this.barsPerPage)}`;
      } else {
        this.pageLabel.textContent = `Bars 1-${tab.bars.length}`;
      }

      for (let b = startBar; b < endBar; b++) {
        const bar = tab.bars[b];
        const row = document.createElement('div');
        row.id = `bar-row-${b}`;
        row.className = `bar-row ${b % 2 === 1 ? 'even-line' : ''} ${bar.stanzaBreak ? 'stanza-break' : ''}`;

        // 1. Gutter with Stationary Playhead Arrow ▶
        const gutter = document.createElement('div');
        gutter.className = 'gutter-col';
        gutter.innerHTML = `<span class="playhead-arrow">▶</span>`;
        row.appendChild(gutter);

        // 2. Bar Number: Two Digits (01, 02... 20)
        const num = document.createElement('span');
        num.className = 'bar-num-text';
        num.textContent = (b + 1).toString().padStart(2, '0');
        row.appendChild(num);

        // 3. Metric Cross-Rhythm Label
        const meter = document.createElement('span');
        meter.className = 'bar-metric-label';
        meter.textContent = bar.notation.toString();
        meter.title = 'Click to customize metric flow for this line';
        meter.addEventListener('click', () => this.promptLineMetric(b));
        row.appendChild(meter);

        // Column Divider
        const div1 = document.createElement('div');
        div1.className = 'col-divider';
        row.appendChild(div1);

        // 4. Pulse Group Grid
        const grid = document.createElement('div');
        grid.className = 'bar-pulses-grid';

        bar.syllables.forEach((pulseArr, pIdx) => {
          const pulseBox = document.createElement('div');
          pulseBox.className = 'pulse-group-box';

          pulseArr.forEach((syl, sIdx) => {
            const cell = document.createElement('div');
            cell.className = 'syllable-cell';
            cell.id = `cell-${b}-${pIdx}-${sIdx}`;

            // Rhyme or Custom Highlight Tint
            if (syl.customColor) {
              cell.style.backgroundColor = syl.customColor;
            } else if (this.rhymesEnabled && syl.text.trim()) {
              const rKey = RhymeClassifier.extractRhymeKey(syl.text.trim());
              const hash = Math.abs(rKey.split('').reduce((acc, ch) => acc + ch.charCodeAt(0), 0));
              cell.style.backgroundColor = PASTEL_SWATCHES[hash % PASTEL_SWATCHES.length].color;
            }

            // Cell text element
            const textSpan = document.createElement('span');
            textSpan.className = `cell-text align-${syl.align} ${syl.bold ? 'bold' : ''} ${!syl.text ? 'empty' : ''}`;
            textSpan.textContent = syl.text;
            cell.appendChild(textSpan);

            // Hover Alignment Controls (Bottom Left, Bottom Underline, Bottom Right)
            const alignPanel = document.createElement('div');
            alignPanel.className = 'cell-align-panel';
            alignPanel.innerHTML = `
              <button class="align-arrow-left ${syl.align === 'left' ? 'active' : ''}" title="Align Left">←</button>
              <div class="align-underline-bar ${syl.align === 'center' ? 'active' : ''}" title="Align Center / Down"></div>
              <button class="align-arrow-right ${syl.align === 'right' ? 'active' : ''}" title="Align Right">→</button>
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

            // Cell Click to Focus / Edit
            cell.addEventListener('click', (e) => {
              // If click happened on alignment controls, ignore
              if (e.target.closest('.cell-align-panel')) return;
              this.activateCellEditor(b, pIdx, sIdx);
            });

            // Cell Context Menu (Right Click)
            cell.addEventListener('contextmenu', (e) => {
              e.preventDefault();
              this.showCellContextMenu(e.clientX, e.clientY, b, pIdx, sIdx);
            });

            pulseBox.appendChild(cell);
          });
          grid.appendChild(pulseBox);
        });
        row.appendChild(grid);

        // Column Divider
        const div2 = document.createElement('div');
        div2.className = 'col-divider';
        row.appendChild(div2);

        // 5. Syllable Counter Controls: [-] [15/15] [+]
        const counterWrap = document.createElement('div');
        counterWrap.className = 'syl-counter-wrapper';
        const gridTotal = bar.notation.getTotalSyllables();
        const spokenCount = (bar.customSpokenCount !== null)
          ? bar.customSpokenCount
          : bar.syllables.flat().filter((s) => s.text.trim().length > 0).length;
        const isCustom = (bar.customSpokenCount !== null);

        counterWrap.innerHTML = `
          <button class="counter-btn dec">-</button>
          <span class="syl-count-badge ${isCustom ? 'customized' : ''}" title="Spoken syllables / Grid subdivisions. Double-click to reset">${spokenCount}/${gridTotal}</span>
          <button class="counter-btn inc">+</button>
        `;

        counterWrap.querySelector('.dec').addEventListener('click', () => {
          this.pushSnapshot();
          bar.customSpokenCount = Math.max(0, spokenCount - 1);
          this.updateRowCounter(b);
        });
        counterWrap.querySelector('.inc').addEventListener('click', () => {
          this.pushSnapshot();
          bar.customSpokenCount = spokenCount + 1;
          this.updateRowCounter(b);
        });
        counterWrap.querySelector('.syl-count-badge').addEventListener('dblclick', () => {
          this.pushSnapshot();
          bar.customSpokenCount = null;
          this.updateRowCounter(b);
        });

        row.appendChild(counterWrap);

        // 6. Row Hover "+" Button (Tucked in Bottom Right Corner)
        const addBtn = document.createElement('button');
        addBtn.className = 'row-add-button';
        addBtn.textContent = '+';
        addBtn.title = 'Row options: Add new line or toggle stanza break';
        addBtn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.showRowMenu(e.clientX, e.clientY, b);
        });
        row.appendChild(addBtn);

        this.pageContainer.appendChild(row);
      }
    }

    // =========================================================================
    // 8. Rock-Solid Inline Typing Engine
    // =========================================================================
    activateCellEditor(b, pIdx, sIdx, initialChar = null) {
      // If this exact cell is already being actively edited, don't recreate input!
      if (this.activeEditor &&
          this.activeEditor.b === b &&
          this.activeEditor.pIdx === pIdx &&
          this.activeEditor.sIdx === sIdx) {
        return;
      }

      // Commit and remove any existing active editor
      this.closeActiveEditor();

      const cellEl = document.getElementById(`cell-${b}-${pIdx}-${sIdx}`);
      if (!cellEl) return;

      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      const textSpan = cellEl.querySelector('.cell-text');
      textSpan.style.display = 'none';
      cellEl.classList.add('focused');

      const input = document.createElement('input');
      input.type = 'text';
      input.className = `cell-active-input align-${syl.align} ${syl.bold ? 'bold' : ''}`;
      input.value = (initialChar !== null) ? initialChar : syl.text;
      cellEl.appendChild(input);

      input.focus();
      if (initialChar !== null) {
        input.setSelectionRange(1, 1);
      } else {
        input.select();
      }

      this.activeEditor = { b, pIdx, sIdx, inputEl: input, syl };

      // Input Event: Live Text Update
      input.addEventListener('input', (e) => {
        syl.text = input.value;
        this.updateRowCounter(b);
      });

      // Keydown Navigation & Typing Flow
      input.addEventListener('keydown', (e) => {
        // Space or Hyphen: Advance to NEXT cell (Typing Flow)
        if (e.key === ' ' || e.key === '-') {
          e.preventDefault();
          let currentVal = input.value.trim();
          if (e.key === '-' && currentVal) currentVal += '-';
          syl.text = currentVal;
          this.closeActiveEditor();
          this.advanceFocus(b, pIdx, sIdx, 1);
          return;
        }

        // Backspace: if input empty, retreat to PREVIOUS cell
        if (e.key === 'Backspace' && input.value.length === 0) {
          e.preventDefault();
          this.closeActiveEditor();
          this.advanceFocus(b, pIdx, sIdx, -1);
          return;
        }

        // Enter: jump to first cell of NEXT bar
        if (e.key === 'Enter') {
          e.preventDefault();
          this.closeActiveEditor();
          this.jumpToBar(b + 1);
          return;
        }

        // Tab / Shift+Tab
        if (e.key === 'Tab') {
          e.preventDefault();
          this.closeActiveEditor();
          this.advanceFocus(b, pIdx, sIdx, e.shiftKey ? -1 : 1);
          return;
        }

        // ArrowLeft at start: previous cell
        if (e.key === 'ArrowLeft' && input.selectionStart === 0) {
          e.preventDefault();
          this.closeActiveEditor();
          this.advanceFocus(b, pIdx, sIdx, -1);
          return;
        }

        // ArrowRight at end: next cell
        if (e.key === 'ArrowRight' && input.selectionStart === input.value.length) {
          e.preventDefault();
          this.closeActiveEditor();
          this.advanceFocus(b, pIdx, sIdx, 1);
          return;
        }

        // ArrowUp: cell in bar above
        if (e.key === 'ArrowUp') {
          e.preventDefault();
          this.closeActiveEditor();
          this.jumpVertical(b, pIdx, sIdx, -1);
          return;
        }

        // ArrowDown: cell in bar below
        if (e.key === 'ArrowDown') {
          e.preventDefault();
          this.closeActiveEditor();
          this.jumpVertical(b, pIdx, sIdx, 1);
          return;
        }

        // Ctrl+B: Toggle Bold
        if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'b') {
          e.preventDefault();
          syl.bold = !syl.bold;
          input.classList.toggle('bold', syl.bold);
          textSpan.classList.toggle('bold', syl.bold);
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

      // Multi-word / Multi-syllable Paste Auto-Distribution
      input.addEventListener('paste', (e) => {
        const pasteData = (e.clipboardData || window.clipboardData).getData('text');
        if (pasteData && (pasteData.includes(' ') || pasteData.includes('\n') || pasteData.length > 5)) {
          e.preventDefault();
          this.distributePastedWords(b, pIdx, sIdx, pasteData);
        }
      });

      // Blur: synchronously finalize text
      input.addEventListener('blur', () => {
        // Wait 50ms in case blur was due to clicking alignment buttons
        setTimeout(() => {
          if (this.activeEditor && this.activeEditor.inputEl === input) {
            this.closeActiveEditor();
          }
        }, 50);
      });
    }

    closeActiveEditor() {
      if (!this.activeEditor) return;
      const { b, pIdx, sIdx, inputEl, syl } = this.activeEditor;
      syl.text = inputEl.value.trim();

      const cellEl = document.getElementById(`cell-${b}-${pIdx}-${sIdx}`);
      if (cellEl) {
        cellEl.classList.remove('focused');
        const textSpan = cellEl.querySelector('.cell-text');
        if (textSpan) {
          textSpan.textContent = syl.text;
          textSpan.className = `cell-text align-${syl.align} ${syl.bold ? 'bold' : ''} ${!syl.text ? 'empty' : ''}`;
          textSpan.style.display = 'flex';
        }
      }

      inputEl.remove();
      this.activeEditor = null;
      this.updateRowCounter(b);
    }

    advanceFocus(b, pIdx, sIdx, dir) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
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
            const prevBar = this.tabs[this.activeTabIdx].bars[targetB];
            targetP = prevBar.syllables.length - 1;
            targetS = prevBar.syllables[targetP].length - 1;
          }
        } else {
          targetS = bar.syllables[targetP].length - 1;
        }
      }

      if (targetB >= 0 && targetB < this.tabs[this.activeTabIdx].bars.length) {
        this.activateCellEditor(targetB, targetP, targetS);
      }
    }

    jumpVertical(b, pIdx, sIdx, barDelta) {
      const targetB = b + barDelta;
      const bars = this.tabs[this.activeTabIdx].bars;
      if (targetB >= 0 && targetB < bars.length) {
        const targetBar = bars[targetB];
        const targetP = Math.min(pIdx, targetBar.syllables.length - 1);
        const targetS = Math.min(sIdx, targetBar.syllables[targetP].length - 1);
        this.activateCellEditor(targetB, targetP, targetS);
      }
    }

    jumpToBar(targetB) {
      const bars = this.tabs[this.activeTabIdx].bars;
      if (targetB >= 0 && targetB < bars.length) {
        this.activateCellEditor(targetB, 0, 0);
      }
    }

    distributePastedWords(b, pIdx, sIdx, text) {
      this.pushSnapshot();
      const words = text.split(/[\s\r\n]+/).filter(Boolean);
      const bars = this.tabs[this.activeTabIdx].bars;

      let curB = b;
      let curP = pIdx;
      let curS = sIdx;

      words.forEach((w) => {
        if (curB < bars.length) {
          const bar = bars[curB];
          if (curP < bar.syllables.length && curS < bar.syllables[curP].length) {
            bar.syllables[curP][curS].text = w;
            curS++;
            if (curS >= bar.syllables[curP].length) {
              curS = 0;
              curP++;
              if (curP >= bar.syllables.length) {
                curP = 0;
                curB++;
              }
            }
          }
        }
      });

      this.renderPage();
      if (curB < bars.length) {
        this.activateCellEditor(curB, curP, curS);
      }
    }

    setCellAlignment(b, pIdx, sIdx, align) {
      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      syl.align = align;
      const cellEl = document.getElementById(`cell-${b}-${pIdx}-${sIdx}`);
      if (cellEl) {
        const span = cellEl.querySelector('.cell-text');
        if (span) span.className = `cell-text align-${align} ${syl.bold ? 'bold' : ''} ${!syl.text ? 'empty' : ''}`;
        const input = cellEl.querySelector('input.cell-active-input');
        if (input) input.className = `cell-active-input align-${align} ${syl.bold ? 'bold' : ''}`;
        const panel = cellEl.querySelector('.cell-align-panel');
        if (panel) {
          panel.querySelector('.align-arrow-left').classList.toggle('active', align === 'left');
          panel.querySelector('.align-underline-bar').classList.toggle('active', align === 'center');
          panel.querySelector('.align-arrow-right').classList.toggle('active', align === 'right');
        }
      }
    }

    updateRowCounter(b) {
      const row = document.getElementById(`bar-row-${b}`);
      if (!row) return;
      const bar = this.tabs[this.activeTabIdx].bars[b];
      const gridTotal = bar.notation.getTotalSyllables();
      const count = (bar.customSpokenCount !== null)
        ? bar.customSpokenCount
        : bar.syllables.flat().filter((s) => s.text.trim().length > 0).length;

      const badge = row.querySelector('.syl-count-badge');
      if (badge) {
        badge.textContent = `${count}/${gridTotal}`;
        badge.classList.toggle('customized', bar.customSpokenCount !== null);
      }
    }

    // =========================================================================
    // 9. Context Menus & Dialogs
    // =========================================================================
    showCellContextMenu(x, y, b, pIdx, sIdx) {
      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];
      const rhymeKey = RhymeClassifier.extractRhymeKey(syl.text);
      const headerTitle = syl.text ? `Syllable: "${syl.text}" [${rhymeKey}]` : 'Syllable: (empty)';

      this.contextMenu.style.left = `${Math.min(x, window.innerWidth - 220)}px`;
      this.contextMenu.style.top = `${Math.min(y, window.innerHeight - 380)}px`;

      this.contextMenu.innerHTML = `
        <div class="popup-menu-header">${headerTitle}</div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Highlight Color ▶</div>
          <div class="popup-submenu">
            <div class="popup-menu-item" id="ctx-col-auto">Auto (Phoneme Rhyme Tint)</div>
            <div class="popup-menu-separator"></div>
            ${PASTEL_SWATCHES.map((sw, idx) => `<div class="popup-menu-item" id="ctx-col-${idx}">${sw.name}</div>`).join('')}
            <div class="popup-menu-separator"></div>
            <div class="popup-menu-item" id="ctx-col-clear">Clear Highlight (None)</div>
          </div>
        </div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Transform Text ▶</div>
          <div class="popup-submenu">
            <div class="popup-menu-item" id="ctx-case-upper">UPPERCASE</div>
            <div class="popup-menu-item" id="ctx-case-lower">lowercase</div>
            <div class="popup-menu-item" id="ctx-case-title">Capitalize Word</div>
          </div>
        </div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Alignment ▶</div>
          <div class="popup-submenu">
            <div class="popup-menu-item" id="ctx-align-left">Align Left (←)</div>
            <div class="popup-menu-item" id="ctx-align-center">Align Center / Down (↓)</div>
            <div class="popup-menu-item" id="ctx-align-right">Align Right (→)</div>
          </div>
        </div>
        <div class="popup-menu-item" id="ctx-bold">Bold Emphasis (Ctrl+B)</div>
        <div class="popup-menu-item" id="ctx-duplicate">Duplicate into Next Box</div>
        <div class="popup-menu-separator"></div>
        <div class="popup-menu-item" id="ctx-cut">Cut (Ctrl+X)</div>
        <div class="popup-menu-item" id="ctx-copy">Copy (Ctrl+C)</div>
        <div class="popup-menu-item" id="ctx-clear">Clear Cell (Del)</div>
      `;
      this.contextMenu.style.display = 'block';

      // Color actions
      document.getElementById('ctx-col-auto').onclick = () => {
        syl.customColor = null;
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      PASTEL_SWATCHES.forEach((sw, idx) => {
        const el = document.getElementById(`ctx-col-${idx}`);
        if (el) {
          el.onclick = () => {
            syl.customColor = sw.color;
            this.contextMenu.style.display = 'none';
            this.renderPage();
          };
        }
      });
      document.getElementById('ctx-col-clear').onclick = () => {
        syl.customColor = 'transparent';
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };

      // Case transforms
      document.getElementById('ctx-case-upper').onclick = () => {
        syl.text = syl.text.toUpperCase();
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      document.getElementById('ctx-case-lower').onclick = () => {
        syl.text = syl.text.toLowerCase();
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      document.getElementById('ctx-case-title').onclick = () => {
        syl.text = syl.text.charAt(0).toUpperCase() + syl.text.slice(1).toLowerCase();
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };

      // Alignment
      document.getElementById('ctx-align-left').onclick = () => {
        this.setCellAlignment(b, pIdx, sIdx, 'left');
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-align-center').onclick = () => {
        this.setCellAlignment(b, pIdx, sIdx, 'center');
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-align-right').onclick = () => {
        this.setCellAlignment(b, pIdx, sIdx, 'right');
        this.contextMenu.style.display = 'none';
      };

      // Bold
      document.getElementById('ctx-bold').onclick = () => {
        syl.bold = !syl.bold;
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };

      // Duplicate
      document.getElementById('ctx-duplicate').onclick = () => {
        this.contextMenu.style.display = 'none';
        this.advanceFocus(b, pIdx, sIdx, 1);
        if (this.activeEditor) {
          this.activeEditor.inputEl.value = syl.text;
          this.activeEditor.syl.bold = syl.bold;
          this.activeEditor.syl.align = syl.align;
        }
      };

      // Clipboard
      document.getElementById('ctx-cut').onclick = () => {
        navigator.clipboard.writeText(syl.text);
        syl.text = '';
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      document.getElementById('ctx-copy').onclick = () => {
        navigator.clipboard.writeText(syl.text);
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-clear').onclick = () => {
        syl.text = '';
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
    }

    showRowMenu(x, y, b) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      this.rowAddMenu.style.left = `${Math.min(x - 180, window.innerWidth - 200)}px`;
      this.rowAddMenu.style.top = `${Math.min(y, window.innerHeight - 100)}px`;

      this.rowAddMenu.innerHTML = `
        <div class="popup-menu-item" id="row-add-line">Add new line (w same meter)</div>
        <div class="popup-menu-item" id="row-toggle-stanza">${bar.stanzaBreak ? 'Remove stanza break' : 'Add stanza break'}</div>
      `;
      this.rowAddMenu.style.display = 'block';

      document.getElementById('row-add-line').onclick = () => {
        this.pushSnapshot();
        const newBar = {
          notation: bar.notation.clone(),
          syllables: bar.notation.pulseSubdivs.map(c => Array(c).fill(null).map(() => ({ text: '', bold: false, align: 'center', customColor: null }))),
          customSpokenCount: null,
          stanzaBreak: false
        };
        this.tabs[this.activeTabIdx].bars.splice(b + 1, 0, newBar);
        this.rowAddMenu.style.display = 'none';
        this.renderPage();
      };

      document.getElementById('row-toggle-stanza').onclick = () => {
        this.pushSnapshot();
        bar.stanzaBreak = !bar.stanzaBreak;
        this.rowAddMenu.style.display = 'none';
        this.renderPage();
      };
    }

    showPresetMenu(e) {
      const rect = this.presetBtn.getBoundingClientRect();
      this.presetMenu.style.left = `${rect.left}px`;
      this.presetMenu.style.top = `${rect.bottom + 4}px`;

      const templates = [
        { name: 'Straight 16ths [4444]/4:4', notat: '[4444]/4:4' },
        { name: 'Straight 8ths [2222]/4:4', notat: '[2222]/4:4' },
        { name: 'Triplets [3333]/4:4', notat: '[3333]/4:4' },
        { name: 'Quintuplets [5555]/4:4', notat: '[5555]/4:4' },
        { name: 'Sextuplets [6666]/4:4', notat: '[6666]/4:4' },
        { name: 'Waltz [333]/3:3', notat: '[333]/3:3' },
        { name: 'Polyrhythm [333222]/6:4', notat: '[333222]/6:4' }
      ];

      const customPresets = JSON.parse(localStorage.getItem('cc_presets') || '[]');

      this.presetMenu.innerHTML = `
        <div class="popup-menu-header">Factory Templates</div>
        ${templates.map((t, i) => `<div class="popup-menu-item" id="preset-tpl-${i}">${t.name}</div>`).join('')}
        ${customPresets.length ? '<div class="popup-menu-separator"></div><div class="popup-menu-header">User Presets</div>' : ''}
        ${customPresets.map((t, i) => `<div class="popup-menu-item" id="preset-usr-${i}">${t.name}</div>`).join('')}
      `;
      this.presetMenu.style.display = 'block';

      templates.forEach((t, i) => {
        const el = document.getElementById(`preset-tpl-${i}`);
        if (el) {
          el.onclick = () => {
            this.pushSnapshot();
            this.globalNotation = MetricNotation.fromString(t.notat);
            this.updateHeader();
            this.applyGlobalNotation();
            this.presetMenu.style.display = 'none';
          };
        }
      });

      customPresets.forEach((t, i) => {
        const el = document.getElementById(`preset-usr-${i}`);
        if (el) {
          el.onclick = () => {
            this.pushSnapshot();
            this.globalNotation = MetricNotation.fromString(t.notat);
            this.updateHeader();
            this.applyGlobalNotation();
            this.presetMenu.style.display = 'none';
          };
        }
      });
    }

    showExportMenu(e) {
      const rect = this.exportBtn.getBoundingClientRect();
      this.exportMenu.style.left = `${rect.left}px`;
      this.exportMenu.style.top = `${rect.bottom + 4}px`;

      this.exportMenu.innerHTML = `
        <div class="popup-menu-item" id="exp-txt-clip">Copy Formatted Text [xx] to Clipboard</div>
        <div class="popup-menu-item" id="exp-txt-file">Download Formatted Text (.txt)</div>
        <div class="popup-menu-item" id="exp-csv-file">Download Spreadsheet Table (.csv)</div>
        <div class="popup-menu-item" id="exp-html-file">Open Printable HTML Table (.html)</div>
      `;
      this.exportMenu.style.display = 'block';

      document.getElementById('exp-txt-clip').onclick = () => {
        this.exportLyricsTxt(true);
        this.exportMenu.style.display = 'none';
      };
      document.getElementById('exp-txt-file').onclick = () => {
        this.downloadFile('lyrics_export.txt', this.exportLyricsTxt(false));
        this.exportMenu.style.display = 'none';
      };
      document.getElementById('exp-csv-file').onclick = () => {
        this.downloadFile('lyrics_table.csv', this.exportLyricsCsv());
        this.exportMenu.style.display = 'none';
      };
      document.getElementById('exp-html-file').onclick = () => {
        const html = this.exportLyricsHtml();
        const blob = new Blob([html], { type: 'text/html' });
        window.open(URL.createObjectURL(blob), '_blank');
        this.exportMenu.style.display = 'none';
      };
    }

    showSongsMenu(e) {
      const rect = this.songsBtn.getBoundingClientRect();
      this.songsMenu.style.left = `${rect.left}px`;
      this.songsMenu.style.top = `${rect.bottom + 4}px`;

      const savedSongs = JSON.parse(localStorage.getItem('cc_songs') || '{}');
      const songNames = Object.keys(savedSongs);

      this.songsMenu.innerHTML = `
        <div class="popup-menu-item" id="song-save">+ Save Current Song Project...</div>
        <div class="popup-menu-separator"></div>
        <div class="popup-menu-header">Load Song Project</div>
        ${songNames.length ? songNames.map((n, i) => `<div class="popup-menu-item" id="song-load-${i}">${n}</div>`).join('') : '<div class="popup-menu-item" style="opacity:0.5;">(No saved songs)</div>'}
        <div class="popup-menu-separator"></div>
        <div class="popup-menu-item" id="song-new">New Song (Clear Notepad)</div>
      `;
      this.songsMenu.style.display = 'block';

      document.getElementById('song-save').onclick = () => {
        const name = prompt('Enter Song Project Name:', 'My Song');
        if (name) {
          savedSongs[name.trim()] = this.tabs;
          localStorage.setItem('cc_songs', JSON.stringify(savedSongs));
          alert(`Saved song: ${name.trim()}`);
        }
        this.songsMenu.style.display = 'none';
      };

      songNames.forEach((n, i) => {
        const el = document.getElementById(`song-load-${i}`);
        if (el) {
          el.onclick = () => {
            this.pushSnapshot();
            const loadedTabs = savedSongs[n];
            loadedTabs.forEach(tab => {
              tab.bars.forEach(bar => {
                if (bar.notation && typeof bar.notation === 'object') {
                  bar.notation = new MetricNotation(bar.notation.pulseSubdivs, bar.notation.beatsPerBar);
                } else {
                  bar.notation = new MetricNotation([3, 3, 3, 2, 2, 2], 4);
                }
              });
            });
            this.tabs = loadedTabs;
            this.activeTabIdx = 0;
            this.renderTabs();
            this.renderPage();
            this.songsMenu.style.display = 'none';
          };
        }
      });

      document.getElementById('song-new').onclick = () => {
        if (confirm('Clear all lyrics and start a new song?')) {
          this.pushSnapshot();
          this.tabs = [{ title: 'Main', bars: this.createEmptyBars() }];
          this.activeTabIdx = 0;
          this.renderTabs();
          this.renderPage();
        }
        this.songsMenu.style.display = 'none';
      };
    }

    copyLyrics() {
      const bars = this.tabs[this.activeTabIdx].bars;
      const lines = [];
      bars.forEach((b) => {
        const text = b.syllables.flat().map(s => s.text).join(' ').replace(/\s+/g, ' ').trim();
        if (text) lines.push(text);
      });
      navigator.clipboard.writeText(lines.join('\n'));
      const orig = this.copyBtn.textContent;
      this.copyBtn.textContent = 'Copied!';
      setTimeout(() => { this.copyBtn.textContent = orig; }, 1200);
    }

    exportLyricsTxt(toClipboard = false) {
      const bars = this.tabs[this.activeTabIdx].bars;
      const lines = [];
      bars.forEach((b) => {
        const text = b.syllables.flat().map(s => s.text).join(' ').replace(/\s+/g, ' ').trim();
        const count = (b.customSpokenCount !== null) ? b.customSpokenCount : b.syllables.flat().filter(s => s.text).length;
        if (text) {
          lines.push(`${text} [${count}]`);
          if (b.stanzaBreak) lines.push('');
        }
      });
      const res = lines.join('\n');
      if (toClipboard) {
        navigator.clipboard.writeText(res);
        const orig = this.exportBtn.textContent;
        this.exportBtn.textContent = 'Copied!';
        setTimeout(() => { this.exportBtn.textContent = orig; }, 1200);
      }
      return res;
    }

    exportLyricsCsv() {
      const bars = this.tabs[this.activeTabIdx].bars;
      const lines = ['Bar,Metric Schema,Spoken Syllables,Grid Syllables,Lyrics'];
      bars.forEach((b, idx) => {
        const text = b.syllables.flat().map(s => s.text).join(' ').replace(/\s+/g, ' ').trim();
        const count = (b.customSpokenCount !== null) ? b.customSpokenCount : b.syllables.flat().filter(s => s.text).length;
        lines.push(`${idx + 1},"${b.notation.toString()}",${count},${b.notation.getTotalSyllables()},"${text.replace(/"/g, '""')}"`);
      });
      return lines.join('\n');
    }

    exportLyricsHtml() {
      const bars = this.tabs[this.activeTabIdx].bars;
      let rowsHtml = '';
      bars.forEach((b, idx) => {
        const text = b.syllables.flat().map(s => s.text).join(' ').replace(/\s+/g, ' ').trim();
        const count = (b.customSpokenCount !== null) ? b.customSpokenCount : b.syllables.flat().filter(s => s.text).length;
        if (text) {
          rowsHtml += `<tr><td style="padding:6px 12px;color:#d97706;font-weight:bold;">${(idx+1).toString().padStart(2,'0')}</td><td style="padding:6px 12px;font-family:monospace;">${b.notation.toString()}</td><td style="padding:6px 12px;font-size:15px;">${text}</td><td style="padding:6px 12px;font-family:monospace;font-weight:bold;color:#b45309;">${count}</td></tr>`;
        }
      });
      return `<!DOCTYPE html><html><head><meta charset="utf-8"><title>Compass Cadence Lyrics</title><style>body{font-family:Segoe UI,sans-serif;padding:30px;background:#18181b;color:#f1f5f9;}table{border-collapse:collapse;width:100%;}tr:nth-child(even){background:#222227;}th{text-align:left;padding:8px 12px;border-bottom:2px solid #3f3f46;color:#94a3b8;}</style></head><body><h1>Compass Cadence — Lyric Sheet</h1><table><thead><tr><th>Bar</th><th>Meter</th><th>Lyrics</th><th>Syllables</th></tr></thead><tbody>${rowsHtml}</tbody></table></body></html>`;
    }

    downloadFile(filename, content) {
      const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = filename;
      a.click();
    }

    promptLineMetric(b) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      const val = prompt(`Enter custom metric cross-rhythm for Bar ${b + 1}:`, bar.notation.toString());
      if (val) {
        this.pushSnapshot();
        bar.notation = MetricNotation.fromString(val, bar.notation.beatsPerBar);
        // Rebuild pulse boxes for this bar
        const newSyllables = [];
        bar.notation.pulseSubdivs.forEach((count, p) => {
          const pulseArr = [];
          for (let s = 0; s < count; s++) {
            const oldSyl = (bar.syllables[p] && bar.syllables[p][s]) ? bar.syllables[p][s] : null;
            pulseArr.push({
              text: oldSyl ? oldSyl.text : '',
              bold: oldSyl ? oldSyl.bold : false,
              align: oldSyl ? oldSyl.align : 'center',
              customColor: oldSyl ? oldSyl.customColor : null
            });
          }
          newSyllables.push(pulseArr);
        });
        bar.syllables = newSyllables;
        this.renderPage();
      }
    }

    promptSavePreset() {
      const curStr = this.globalNotation.toString();
      const name = prompt('Enter preset name:', `My Flow ${curStr}`);
      if (name) {
        const presets = JSON.parse(localStorage.getItem('cc_presets') || '[]');
        presets.push({ name: name.trim(), notat: curStr });
        localStorage.setItem('cc_presets', JSON.stringify(presets));
        alert(`Saved preset "${name.trim()}"`);
      }
    }

    drawSpiralCanvas() {
      if (!this.spiralCanvas) return;
      const ctx = this.spiralCanvas.getContext('2d');
      const dpr = window.devicePixelRatio || 1;
      const h = this.viewport.clientHeight || 800;
      this.spiralCanvas.width = 42 * dpr;
      this.spiralCanvas.height = h * dpr;
      this.spiralCanvas.style.width = '42px';
      this.spiralCanvas.style.height = `${h}px`;
      ctx.scale(dpr, dpr);

      ctx.clearRect(0, 0, 42, h);

      const pitch = 28.8;
      const spiralX = 18.0;
      const numRings = Math.ceil(h / pitch) + 2;

      for (let i = 0; i < numRings; i++) {
        const ringY = 16.0 + i * pitch;

        // Hole punch oval
        ctx.fillStyle = this.darkMode ? '#09090B' : 'rgba(0, 0, 0, 0.35)';
        ctx.beginPath();
        ctx.ellipse(spiralX - 2, ringY, 4.5, 4.5, 0, 0, Math.PI * 2);
        ctx.fill();

        // Double wire metallic loop 1 (Silver Highlight)
        ctx.strokeStyle = '#D4D4D8';
        ctx.lineWidth = 2.0;
        ctx.lineCap = 'round';
        ctx.beginPath();
        ctx.moveTo(0, ringY - 3.0);
        ctx.bezierCurveTo(spiralX * 0.7, ringY - 6.0, spiralX + 5.0, ringY - 6.0, spiralX + 1.0, ringY + 1.0);
        ctx.stroke();

        // Wire graphite shadow
        ctx.strokeStyle = '#71717A';
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        ctx.moveTo(0, ringY - 1.0);
        ctx.bezierCurveTo(spiralX * 0.7, ringY - 4.0, spiralX + 5.0, ringY - 4.0, spiralX + 1.0, ringY + 3.0);
        ctx.stroke();
      }
    }
  }

  // Initialize once DOM is ready
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', () => { window.__ccApp = new CompassCadenceApp(); });
  } else {
    window.__ccApp = new CompassCadenceApp();
  }
})();
