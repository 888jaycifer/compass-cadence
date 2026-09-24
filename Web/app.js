/**
 * Compass Cadence — Full 1:1 VST3 Feature Engine & Notepad
 * Virtual Studio Technology (VST) Lyric & Cross-Rhythm Notepad
 */

(function () {
  'use strict';

  // =========================================================================
  // 0. Syllable Splitter Engine
  // =========================================================================
  class SyllableSplitter {
    static splitWord(word) {
        word = word.trim();
        if (word.length <= 3) return [word];
        const lower = word.toLowerCase();
        
        let syls = lower.match(/[^aeiouy]*[aeiouy]+(?:[^aeiouy]*$|[^aeiouy](?=[^aeiouy]))?/gi);
        if (!syls) return [word];
        
        // Correct silent E suffixes
        if (syls.length > 1) {
            let last = syls[syls.length-1];
            if (last.match(/e$/) || last.match(/ed$/) || last.match(/es$/)) {
                if (!last.match(/[lxcsz]es$/) && !last.match(/ches$/) && !last.match(/shes$/)) {
                    let prev = syls[syls.length-2];
                    syls[syls.length-2] = prev + syls.pop();
                }
            }
        }
        
        let idx = 0;
        let result = [];
        for (let s of syls) {
            result.push(word.slice(idx, idx + s.length));
            idx += s.length;
        }
        return result.length > 0 ? result : [word];
    }
  }

  // =========================================================================
  // 1. Metric Cross-Rhythm Notation Model
  // =========================================================================
  class MetricNotation {
    constructor(pulseSubdivs = [3, 3, 3, 2, 2, 2], beats = 4, pulseDurations = []) {
      this.pulseSubdivs = [...pulseSubdivs];
      this.beatsPerBar = Math.max(1, beats);
      this.pulseDurations = [...pulseDurations];
      if (this.pulseDurations.length > 0 && this.pulseDurations.length !== this.pulseSubdivs.length) {
        const defaultDur = this.beatsPerBar / this.pulseSubdivs.length;
        while (this.pulseDurations.length < this.pulseSubdivs.length) this.pulseDurations.push(defaultDur);
        this.pulseDurations = this.pulseDurations.slice(0, this.pulseSubdivs.length);
      }
    }

    static fromString(str, fallbackBeats = 4) {
      if (!str) return new MetricNotation([4, 4, 4, 4], 4);
      let text = str.trim();
      let parsedDurations = [];
      const angleMatch = text.match(/<([^>]+)>/);
      if (angleMatch) {
        const angleContent = angleMatch[1];
        text = text.replace(/<[^>]+>/, '').trim();
        parsedDurations = angleContent.split(/[,+\s]+/).map(s => parseFloat(s.trim())).filter(n => !isNaN(n) && n > 0);
      }

      let beats = fallbackBeats;
      let specifiedPulses = -1;
      let bracketPart = text;

      if (text.includes('/')) {
        const slashParts = text.split('/');
        bracketPart = slashParts[0].trim();
        const ratioPart = slashParts[1].trim();
        if (ratioPart.includes(':')) {
          const colonParts = ratioPart.split(':');
          specifiedPulses = parseInt(colonParts[0], 10);
          beats = parseInt(colonParts[1], 10);
        } else {
          beats = parseInt(ratioPart, 10);
        }
      } else if (text.includes(':')) {
        const colonParts = text.split(':');
        bracketPart = colonParts[0].trim();
        beats = parseInt(colonParts[1], 10);
      }

      if (isNaN(beats) || beats <= 0) beats = fallbackBeats || 4;

      const bracketMatch = bracketPart.match(/\[([^\]]+)\]/);
      const inner = bracketMatch ? bracketMatch[1].trim() : bracketPart.trim();
      let digits = [];
      if (inner.includes(',') || inner.includes('-') || inner.includes(' ')) {
        digits = inner.split(/[,-s\s]+/).map(s => parseInt(s.trim(), 10)).filter(n => !isNaN(n) && n > 0);
      } else {
        digits = inner.split('').map(Number).filter(n => !isNaN(n) && n > 0);
      }

      if (!digits.length) {
        digits = specifiedPulses > 0 ? new Array(specifiedPulses).fill(4) : [4, 4, 4, 4];
      }

      if (specifiedPulses > 0 && specifiedPulses !== digits.length) {
        if (specifiedPulses > digits.length) {
          const lastVal = digits[digits.length - 1];
          while (digits.length < specifiedPulses) digits.push(lastVal);
        } else {
          digits = digits.slice(0, specifiedPulses);
        }
      }

      if (parsedDurations.length > 0) {
        const sum = parsedDurations.reduce((a, b) => a + b, 0);
        if (sum > 0 && Math.abs(sum - beats) > 0.001) {
          const scale = beats / sum;
          parsedDurations = parsedDurations.map(d => d * scale);
        }
      }

      return new MetricNotation(digits, beats, parsedDurations);
    }

    getPulseDuration(pulseIndex) {
      if (pulseIndex >= 0 && pulseIndex < this.pulseDurations.length) {
        const d = this.pulseDurations[pulseIndex];
        if (d > 0) return d;
      }
      const numPulses = this.pulseSubdivs.length;
      if (numPulses <= 0) return 1.0;
      return this.beatsPerBar / numPulses;
    }

    getTotalDuration() {
      if (this.hasCustomPulseDurations()) {
        const sum = this.pulseDurations.reduce((a, b) => a + b, 0);
        if (sum > 0) return sum;
      }
      return this.beatsPerBar;
    }

    getPulseTimeProportion(pulseIndex) {
      const total = this.getTotalDuration();
      return total > 0 ? this.getPulseDuration(pulseIndex) / total : 0;
    }

    hasCustomPulseDurations() {
      return this.pulseDurations.length > 0 && this.pulseDurations.length === this.pulseSubdivs.length;
    }

    toString() {
      const multiDigit = this.pulseSubdivs.some(n => n >= 10);
      const digits = multiDigit ? this.pulseSubdivs.join(',') : this.pulseSubdivs.join('');
      let s = `[${digits}]`;
      if (this.hasCustomPulseDurations()) {
        s += `<${this.pulseDurations.map(d => Math.floor(d) === d ? d.toFixed(0) : d.toFixed(2)).join(',')}>`;
      }
      s += `/${this.pulseSubdivs.length}:${this.beatsPerBar}`;
      return s;
    }

    getTotalSyllables() {
      return this.pulseSubdivs.reduce((a, b) => a + b, 0);
    }

    clone() {
      return new MetricNotation(this.pulseSubdivs, this.beatsPerBar, this.pulseDurations);
    }
  }

  // =========================================================================
  // =========================================================================
  // 2. Syllable & Phoneme Engine (Pure Vowel Sound Rhyme Classifier)
  // =========================================================================
  const VOWEL_CATALOG = [
    { key: "EY",   label: "long Ay",       examples: "day, say, cake",    defaultColor: "rgba(147, 197, 253, 0.65)" }, // Pastel Sky
    { key: "AE",   label: "short Ah",      examples: "cat, rap, back",    defaultColor: "rgba(254, 215, 170, 0.65)" }, // Pastel Peach
    { key: "IY",   label: "long Ee",       examples: "see, beat, key",    defaultColor: "rgba(134, 239, 172, 0.65)" }, // Pastel Mint
    { key: "EH",   label: "short Eh",      examples: "bed, red, set",     defaultColor: "rgba(254, 205, 211, 0.65)" }, // Pastel Rose
    { key: "AY",   label: "long Eye",      examples: "my, ride, night",   defaultColor: "rgba(221, 214, 254, 0.65)" }, // Pastel Lavender
    { key: "IH",   label: "short Ih",      examples: "hit, spit, kick",   defaultColor: "rgba(254, 240, 138, 0.65)" }, // Pastel Butter
    { key: "OW",   label: "long Oh",       examples: "go, flow, soul",    defaultColor: "rgba(253, 186, 116, 0.65)" }, // Pastel Amber
    { key: "AO",   label: "short Aw",      examples: "all, call, law",    defaultColor: "rgba(167, 243, 208, 0.65)" }, // Pastel Emerald
    { key: "UW",   label: "long Oo",       examples: "true, blue, moon",  defaultColor: "rgba(199, 210, 254, 0.65)" }, // Pastel Indigo
    { key: "AH",   label: "short Uh",      examples: "cut, up, blood",    defaultColor: "rgba(254, 249, 195, 0.65)" }, // Pastel Canary
    { key: "AW",   label: "diphthong Ow",  examples: "out, loud, sound",  defaultColor: "rgba(240, 171, 252, 0.65)" }, // Pastel Fuchsia
    { key: "OY",   label: "diphthong Oy",  examples: "boy, joy, noise",   defaultColor: "rgba(165, 243, 252, 0.65)" }, // Pastel Cyan
    { key: "AA_R", label: "r-colored Ar",  examples: "car, star, hard",   defaultColor: "rgba(203, 213, 225, 0.65)" }, // Pastel Slate
    { key: "AO_R", label: "r-colored Or",  examples: "door, four, core",  defaultColor: "rgba(251, 207, 232, 0.65)" }, // Pastel Pink
    { key: "ER",   label: "r-colored Er",  examples: "bird, word, hurt",  defaultColor: "rgba(228, 228, 231, 0.65)" }  // Pastel Zinc
  ];

  class RhymeClassifier {
    static extractRhymeKey(word) {
      return this.extractRhymeKeyWithContext(word, '', '');
    }

    static extractRhymeKeyWithContext(word, prevSyl = '', nextSyl = '') {
      if (!word) return '';
      let clean = word.toLowerCase().replace(/[^a-z'-]/g, '');
      if (!clean) return '';
      
      // If it is a hyphenated prefix/stem that is not the end of a word, it doesn't rhyme by itself
      if (clean.endsWith('-')) return '';

      // Context-aware "a": distinguish unstressed schwa in "un-der-stand-a-ble" from standalone "a" ("long Ay")
      if (clean === 'a') {
        const hasSurroundingStem = (prevSyl && prevSyl.endsWith('-')) || (nextSyl && nextSyl.startsWith('-')) || (nextSyl && nextSyl.endsWith('-'));
        return hasSurroundingStem ? 'AH' : 'EY';
      }

      // Suffix schwa rules
      if (clean.endsWith('able') || clean.endsWith('ible') || clean.endsWith('al') || clean.endsWith('ful') || 
          clean.endsWith('less') || clean.endsWith('ness') || clean.endsWith('ment') || clean.endsWith('ous') ||
          clean.endsWith('tion') || clean.endsWith('sion') || clean.endsWith('ion')) {
        return 'AH';
      }

      // Standalone "I" / "i" and common "I-" contractions are always [AY] ("long Eye")
      if (clean === 'i' || clean === 'im' || clean === 'ive' || clean === 'id' ||
          word.toLowerCase() === "i'm" || word.toLowerCase() === "i've" || word.toLowerCase() === "i'd" || word.toLowerCase() === "i'll") {
        return 'AY';
      }

      // Explicit dictionary for irregulars/slang/common rap terms
      const dict = {
        'i': 'AY', 'im': 'AY', 'ive': 'AY', 'id': 'AY', 'hi': 'AY', 'wild': 'AY', 'child': 'AY',
        'it': 'IH', 'is': 'IH', 'in': 'IH', 'if': 'IH', 'with': 'IH', 'give': 'IH',
        'to': 'UW', 'you': 'UW', 'through': 'UW', 'true': 'UW', 'lu': 'UW', 'who': 'UW', 'do': 'UW', 'two': 'UW', 'shoe': 'UW',
        'no': 'OW', 'so': 'OW', 'go': 'OW', 'pro': 'OW', 'bo': 'OW', 'flow': 'OW', 'low': 'OW', 'show': 'OW', 'glow': 'OW', 'dough': 'OW',
        'set': 'EH', 'let': 'EH', 'get': 'EH', 'bet': 'EH', 'met': 'EH', 'net': 'EH', 'wet': 'EH', 'pet': 'EH',
        'out': 'AW', 'about': 'AW', 'doubt': 'AW', 'shout': 'AW', 'cloud': 'AW', 'proud': 'AW', 'round': 'AW', 'bound': 'AW', 'sound': 'AW', 'ground': 'AW',
        'put': 'UH', 'foot': 'UH', 'look': 'UH', 'book': 'UH', 'took': 'UH', 'cook': 'UH', 'good': 'UH', 'hood': 'UH', 'wood': 'UH',
        'all': 'AO', 'fall': 'AO', 'call': 'AO', 'ball': 'AO', 'tall': 'AO', 'wall': 'AO', 'small': 'AO', 'raw': 'AO', 'saw': 'AO', 'law': 'AO', 'flaw': 'AO',
        'down': 'AW', 'town': 'AW', 'brown': 'AW', 'crown': 'AW', 'drown': 'AW', 'frown': 'AW', 'noun': 'AW',
        'pa': 'AE', 'per': 'ER', 'were': 'ER', 'her': 'ER', 'sir': 'ER', 'fur': 'ER', 'purr': 'ER',
        'way': 'EY', 'say': 'EY', 'they': 'EY', 'lay': 'EY', 'pay': 'EY', 'may': 'EY', 'day': 'EY', 'stay': 'EY', 'play': 'EY', 'pray': 'EY',
        'side': 'AY', 'hide': 'AY', 'ride': 'AY', 'tide': 'AY', 'wide': 'AY', 'guide': 'AY', 'pride': 'AY',
        'night': 'AY', 'right': 'AY', 'bright': 'AY', 'fight': 'AY', 'light': 'AY', 'sight': 'AY', 'tight': 'AY', 'might': 'AY', 'white': 'AY',
        'space': 'EY', 'place': 'EY', 'face': 'EY', 'race': 'EY', 'case': 'EY', 'base': 'EY', 'chase': 'EY', 'grace': 'EY',
        'hand': 'AE', 'stand': 'AE', 'band': 'AE', 'land': 'AE', 'grand': 'AE', 'brand': 'AE', 'sand': 'AE',
        'if': 'IH', 'spit': 'IH', 'hit': 'IH', 'lit': 'IH', 'fit': 'IH', 'sit': 'IH', 'bit': 'IH', 'quit': 'IH',
        'see': 'IY', 'be': 'IY', 'me': 'IY', 'we': 'IY', 'he': 'IY', 'she': 'IY', 'tree': 'IY', 'free': 'IY', 'flee': 'IY', 'glee': 'IY',
        'great': 'EY', 'hate': 'EY', 'late': 'EY', 'mate': 'EY', 'fate': 'EY', 'date': 'EY', 'state': 'EY', 'rate': 'EY', 'weight': 'EY',
        'prob': 'AA', 'ob': 'AA', 'drop': 'AA', 'top': 'AA', 'stop': 'AA', 'pop': 'AA', 'cop': 'AA', 'hop': 'AA', 'lock': 'AA', 'rock': 'AA',
        'head': 'EH', 'bed': 'EH', 'dead': 'EH', 'red': 'EH', 'said': 'EH', 'bread': 'EH', 'lead': 'EH', 'spread': 'EH',
        'read': 'IY',
        'form': 'AO_R', 'core': 'AO_R', 'more': 'AO_R', 'door': 'AO_R', 'floor': 'AO_R', 'score': 'AO_R', 'store': 'AO_R', 'war': 'AO_R',
        'sub': 'AH', 'club': 'AH', 'rub': 'AH', 'tub': 'AH', 'love': 'AH', 'glove': 'AH', 'above': 'AH', 'dove': 'AH',
        'box': 'AA', 'fox': 'AA',
        'sor': 'AO_R',
        'ry': 'IY', 'city': 'IY', 'pretty': 'IY', 'busy': 'IY', 'easy': 'IY',
        'worth': 'ER', 'earth': 'ER', 'birth': 'ER', 'first': 'ER', 'worst': 'ER',
        'saying': 'EY', 'fra': 'EY', 'ming': 'IH',
        'cash': 'AE', 'flash': 'AE', 'trash': 'AE', 'dash': 'AE', 'smash': 'AE',
        'drugs': 'AH', 'thugs': 'AH', 'slugs': 'AH', 'bugs': 'AH', 'hugs': 'AH',
        'hands': 'AE', 'bands': 'AE',
        'means': 'IY', 'dreams': 'IY', 'teams': 'IY', 'schemes': 'IY',
        'fun': 'AH', 'run': 'AH', 'sun': 'AH', 'gun': 'AH', 'one': 'AH', 'done': 'AH',
        'cap': 'AE', 'rap': 'AE', 'trap': 'AE', 'map': 'AE', 'slap': 'AE',
        'hard': 'AA_R', 'card': 'AA_R', 'yard': 'AA_R', 'star': 'AA_R', 'far': 'AA_R', 'bar': 'AA_R', 'car': 'AA_R', 'dark': 'AA_R', 'park': 'AA_R',
        'boy': 'OY', 'toy': 'OY', 'joy': 'OY', 'coin': 'OY', 'join': 'OY', 'voice': 'OY', 'noise': 'OY', 'choice': 'OY'
      };

      if (dict[clean]) return dict[clean];

      // Phonetic pattern matching on word endings (pure vowel nucleus)
      if (/(?:ar|ard|ark|arm|art|ars|arch|arge)$/i.test(clean)) return 'AA_R';
      if (/(?:or|ore|oar|oor|ord|ork|orm|orn|ort|our|ours)$/i.test(clean)) return 'AO_R';
      if (/(?:er|ir|ur|ear|eer|ier|word|work|worm|burn|turn|hurt|bird|girl|shirt)$/i.test(clean)) return 'ER';

      // Diphthongs
      if (/(?:oy|oi|oin|oyz)$/i.test(clean)) return 'OY';
      if (/(?:ow|ou|ound|ount|oud|out|ouse|outh)$/i.test(clean)) return 'AW';

      // Long vowels
      if (/(?:igh|ight|y|ie|ine|ide|ime|ite|ike|ife|ire|ice|ise|ize)$/i.test(clean)) return 'AY';
      if (/(?:ee|ea|eat|eep|eak|eed|eel|eam|ean|ease|ieve|iece|ey)$/i.test(clean)) return 'IY';
      if (/(?:ay|ai|aid|ail|aim|ain|ait|ake|ame|ane|ape|ate|ave|aze)$/i.test(clean)) return 'EY';
      if (/(?:oa|oat|oak|oam|oan|oad|ose|oke|ole|ome|one|ope|ote|ove)$/i.test(clean)) return 'OW';
      if (/(?:oo|oon|oom|ool|oot|oop|oose|ooth|ue|uit|ute|une|ule|ube)$/i.test(clean)) return 'UW';

      // Short vowels / schwas
      if (/(?:ack|ash|at|ap|am|an|ag|ad|ax|ab|atch)$/i.test(clean)) return 'AE';
      if (/(?:eck|esh|et|ep|em|en|eg|ed|ex|eb|etch)$/i.test(clean)) return 'EH';
      if (/(?:ick|ish|it|ip|im|in|ig|id|ix|ib|itch)$/i.test(clean)) return 'IH';
      if (/(?:ock|osh|ot|op|om|on|og|od|ox|ob|otch)$/i.test(clean)) return 'AA';
      if (/(?:uck|ush|ut|up|um|un|ug|ud|ux|ub|utch)$/i.test(clean)) return 'AH';
      if (/(?:all|aw|alk|alt|aught|ought)$/i.test(clean)) return 'AO';

      // Last resort: inspect the last vowel letter
      const lastVowelMatch = clean.match(/[aeiouy](?=[^aeiouy]*$)/i);
      if (lastVowelMatch) {
        const v = lastVowelMatch[0].toLowerCase();
        if (v === 'a') return clean.endsWith('a') ? 'EY' : 'AE';
        if (v === 'e') return clean.endsWith('e') ? 'IY' : 'EH';
        if (v === 'i') return clean.endsWith('i') ? 'AY' : 'IH';
        if (v === 'o') return clean.endsWith('o') ? 'OW' : 'AO';
        if (v === 'u') return clean.endsWith('u') ? 'UW' : 'AH';
        if (v === 'y') return 'AY';
      }

      return 'AH';
    }
  }

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
      this.colorMode = 'rhymes'; // 'off' | 'rhymes' | 'repeats'
      this.minRepeatLength = 2; // 2, 3, or 4
      this.maxRepeatLineDistance = parseInt(localStorage.getItem('cc_max_repeat_line_dist') || '24', 10);
      this.hiddenColorCells = new Set(); // Set of "b-p-s" keys
      this.repeatSpans = new Map(); // key: "b-p-s" -> array of "b-p-s" in sequence
      this.followDAW = true;
      this.autoSplitEnabled = true;
      this.isDraggingSelection = false;
      this.dragStartCell = null;
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
      this.showAlignmentControls = false;
      this.customVowelColors = JSON.parse(localStorage.getItem('cc_vowel_colors') || '{}');
      this.rowHeight = parseInt(localStorage.getItem('cc_row_height') || '50', 10);
      this.tupletBracketMode = localStorage.getItem('cc_tuplet_mode') || 'all_on'; // 'all_on' | 'all_off' | 'active_line'

      this.initBars();
      this.cacheDOMElements();
      this.bindUI();
      this.setTupletBracketMode(this.tupletBracketMode);
      this.renderTabs();
      this.renderPage();
      this.drawSpiralCanvas();
      window.addEventListener('resize', () => this.drawSpiralCanvas());
    }

    setTupletBracketMode(mode) {
      this.tupletBracketMode = mode;
      localStorage.setItem('cc_tuplet_mode', mode);
      document.body.classList.toggle('tuplet-mode-off', mode === 'all_off');
      document.body.classList.toggle('tuplet-mode-active', mode === 'active_line');
    }

    getVowelColor(vowelKey) {
      if (this.customVowelColors && this.customVowelColors[vowelKey]) {
        return this.customVowelColors[vowelKey];
      }
      const item = VOWEL_CATALOG.find(v => v.key === vowelKey);
      return item ? item.defaultColor : 'rgba(203, 213, 225, 0.65)';
    }

    setVowelColor(vowelKey, color) {
      if (!this.customVowelColors) this.customVowelColors = {};
      this.customVowelColors[vowelKey] = color;
      localStorage.setItem('cc_vowel_colors', JSON.stringify(this.customVowelColors));
      this.renderPage();
    }

    resetVowelColorsToDefaults() {
      this.customVowelColors = {};
      localStorage.removeItem('cc_vowel_colors');
      this.renderPage();
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
      this.autoSplitToggleBtn = document.getElementById('auto-split-btn');
      this.copyBtn = document.getElementById('copy-btn');
      this.exportBtn = document.getElementById('export-btn');
      this.songsBtn = document.getElementById('songs-btn');
      this.alignToggleBtn = document.getElementById('align-toggle-btn');
      this.shortcutsBtn = document.getElementById('shortcuts-btn');
      this.shortcutsDialog = document.getElementById('shortcuts-dialog');

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
      this.vowelPaletteBtn = document.getElementById('vowel-palette-btn');
      this.vowelPaletteDialog = document.getElementById('vowel-palette-dialog');
    }

    bindUI() {
      if (this.vowelPaletteBtn) {
        this.vowelPaletteBtn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.openVowelPaletteDialog();
        });
      }

      if (this.alignToggleBtn) {
        this.alignToggleBtn.addEventListener('click', () => {
          this.showAlignmentControls = !this.showAlignmentControls;
          document.body.classList.toggle('show-align-controls', this.showAlignmentControls);
          this.alignToggleBtn.classList.toggle('toggled', this.showAlignmentControls);
          this.alignToggleBtn.textContent = this.showAlignmentControls ? 'Align: ON' : 'Align: OFF';
        });
      }

      if (this.shortcutsBtn) {
        this.shortcutsBtn.addEventListener('click', (e) => {
          e.stopPropagation();
          this.openShortcutsDialog();
        });
      }

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
      this.presetBtn.addEventListener('click', (e) => { e.stopPropagation(); this.showPresetMenu(e); });
      this.savePresetBtn.addEventListener('click', (e) => { e.stopPropagation(); this.promptSavePreset(); });

      // DAW Status click toggles Play/Stop
      this.dawStatusLabel.addEventListener('click', () => this.togglePlayback());

      // Color Mode Toggle (Rhymes -> Repeats -> Off)
      this.updateColorModeButton();
      this.rhymeToggleBtn.addEventListener('click', () => {
        if (this.colorMode === 'off') {
          this.colorMode = 'rhymes';
        } else if (this.colorMode === 'rhymes') {
          this.colorMode = 'repeats';
        } else {
          this.colorMode = 'off';
        }
        this.updateColorModeButton();
        this.renderPage();
      });

      this.rhymeToggleBtn.addEventListener('contextmenu', (e) => {
        e.preventDefault();
        this.showColorModeMenu(e);
      });

      // Follow DAW Toggle
      this.followToggleBtn.addEventListener('click', () => {
        this.followDAW = !this.followDAW;
        this.followToggleBtn.classList.toggle('toggled', this.followDAW);
        this.followToggleBtn.textContent = this.followDAW ? 'Follow DAW: ON' : 'Follow DAW: OFF';
      });

      // Auto Split Toggle
      if (this.autoSplitToggleBtn) {
        this.autoSplitToggleBtn.addEventListener('click', () => {
          this.autoSplitEnabled = !this.autoSplitEnabled;
          this.autoSplitToggleBtn.classList.toggle('toggled', this.autoSplitEnabled);
          this.autoSplitToggleBtn.textContent = this.autoSplitEnabled ? 'Auto Split: ON' : 'Auto Split: OFF';
        });
      }

      // Copy & Export & Songs
      this.copyBtn.addEventListener('click', (e) => { e.stopPropagation(); this.copyLyrics(); });
      this.exportBtn.addEventListener('click', (e) => { e.stopPropagation(); this.showExportMenu(e); });
      this.songsBtn.addEventListener('click', (e) => { e.stopPropagation(); this.showSongsMenu(e); });

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
      
      document.addEventListener('mouseup', () => {
        this.isDraggingSelection = false;
        this.dragStartCell = null;
      });

      // Global Spacebar Transport Toggle & Shortcuts
      document.addEventListener('keydown', (e) => {
        if (!this.activeEditor) {
          if (e.code === 'Space') {
            e.preventDefault();
            this.togglePlayback();
          }
          if ((e.ctrlKey || e.metaKey) && e.key === 'z' && !e.shiftKey) {
            e.preventDefault(); this.undo();
          }
          if (((e.ctrlKey || e.metaKey) && e.key === 'y') || ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'Z')) {
            e.preventDefault(); this.redo();
          }

          // Shortcuts Cheat Sheet Dialog (? or F1 or Ctrl+/)
          if (e.key === '?' || e.key === 'F1' || ((e.ctrlKey || e.metaKey) && e.key === '/')) {
            e.preventDefault();
            this.openShortcutsDialog();
            return;
          }
          
          // Selection Shortcuts
          if (this.selectedCells.size > 0) {
            if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'b') {
              e.preventDefault(); this.toggleBoldSelection();
            } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'c') {
              e.preventDefault(); this.copySelection();
            } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'x') {
              e.preventDefault(); this.cutSelection();
            } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'v') {
              e.preventDefault(); this.pasteIntoSelection();
            } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'j') {
              e.preventDefault(); this.joinSelection();
            } else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'k') {
              e.preventDefault(); this.splitSelection();
            } else if (e.altKey && e.key === 'ArrowLeft') {
              e.preventDefault();
              this.selectedCells.forEach(id => {
                const [cb, cp, cs] = id.split('-').map(Number);
                this.setCellAlignment(cb, cp, cs, 'left');
              });
              this.renderPage();
            } else if (e.altKey && e.key === 'ArrowRight') {
              e.preventDefault();
              this.selectedCells.forEach(id => {
                const [cb, cp, cs] = id.split('-').map(Number);
                this.setCellAlignment(cb, cp, cs, 'right');
              });
              this.renderPage();
            } else if (e.altKey && (e.key === 'ArrowUp' || e.key === 'ArrowDown')) {
              e.preventDefault();
              this.selectedCells.forEach(id => {
                const [cb, cp, cs] = id.split('-').map(Number);
                this.setCellAlignment(cb, cp, cs, 'center');
              });
              this.renderPage();
            } else if (e.altKey && (e.key === '=' || e.key === '+' || e.key === 'Insert')) {
              e.preventDefault();
              const firstId = Array.from(this.selectedCells)[0];
              const [cb, cp, cs] = firstId.split('-').map(Number);
              this.insertSyllableInBar(cb, cp, cs, !e.shiftKey);
            } else if (e.altKey && (e.key === '-' || e.key === 'Delete')) {
              e.preventDefault();
              const firstId = Array.from(this.selectedCells)[0];
              const [cb, cp, cs] = firstId.split('-').map(Number);
              this.deleteSyllableInBar(cb, cp, cs);
            } else if (e.key === 'Delete' || e.key === 'Backspace') {
              e.preventDefault(); this.clearSelection();
            } else if (e.key.length === 1 && !e.ctrlKey && !e.metaKey && !e.altKey) {
              // Missed First Letter Fix
              const firstId = Array.from(this.selectedCells)[0];
              const [b, pIdx, sIdx] = firstId.split('-').map(Number);
              this.selectedCells.clear();
              this.updateCellSelectionVisuals();
              this.activateCellEditor(b, pIdx, sIdx, e.key);
              e.preventDefault();
            }
          }
        }
      });
    }

    updateCellSelectionVisuals() {
      document.querySelectorAll('.syllable-cell.selected').forEach(el => el.classList.remove('selected'));
      this.selectedCells.forEach(idStr => {
        const el = document.getElementById(`cell-${idStr}`);
        if (el) el.classList.add('selected');
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
        if (bar.customSpokenCount != null && bar.customSpokenCount > bar.notation.getTotalSyllables()) {
          bar.customSpokenCount = bar.notation.getTotalSyllables();
        }
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

      // Repetition map for 'repeats' color mode (exact syllable sequences of length >= minRepeatLength)
      const repeatCellMap = new Map(); // key: `${b}-${pIdx}-${sIdx}` -> color string
      this.repeatSpans = new Map();

      if (this.colorMode === 'repeats') {
        const tokens = [];
        const cleanSyl = (str) => (str || '').toLowerCase().replace(/[^a-z0-9]/g, '');

        tab.bars.forEach((barObj, bIdx) => {
          barObj.syllables.forEach((pulseArr, pIdx) => {
            pulseArr.forEach((syl, sIdx) => {
              const cleaned = cleanSyl(syl.text);
              if (cleaned) {
                tokens.push({ b: bIdx, p: pIdx, s: sIdx, clean: cleaned, key: `${bIdx}-${pIdx}-${sIdx}` });
              }
            });
          });
        });

        const M = tokens.length;
        if (M >= 2) {
          const palette = [
            'rgba(245, 158, 11, 0.45)', // Amber
            'rgba(59, 130, 246, 0.45)',  // Blue
            'rgba(16, 185, 129, 0.45)', // Emerald
            'rgba(236, 72, 153, 0.45)', // Pink
            'rgba(139, 92, 246, 0.45)', // Purple
            'rgba(20, 184, 166, 0.45)',  // Teal
            'rgba(249, 115, 22, 0.45)', // Orange
            'rgba(6, 182, 212, 0.45)',  // Cyan
            'rgba(234, 179, 8, 0.45)',   // Yellow
            'rgba(168, 85, 247, 0.45)', // Violet
            'rgba(34, 197, 94, 0.45)',   // Green
            'rgba(244, 63, 94, 0.45)'    // Rose
          ];
          let nextColorIdx = 0;
          const phraseColours = new Map();

          for (let i = 0; i < M; ++i) {
            for (let j = i + 1; j < M; ++j) {
              const lineDistance = tokens[j].bar - tokens[i].bar;
              if (lineDistance > this.maxRepeatLineDistance) break;

              if (i > 0 && tokens[i - 1].clean === tokens[j - 1].clean) continue;

              let L = 0;
              while (j + L < M && i + L < j && tokens[i + L].clean === tokens[j + L].clean) {
                L++;
              }

              if (L >= this.minRepeatLength) {
                const spanI = [];
                const spanJ = [];
                let spanIHidden = false;
                let spanJHidden = false;

                for (let m = 0; m < L; ++m) {
                  const kI = tokens[i + m].key;
                  const kJ = tokens[j + m].key;
                  spanI.push(kI);
                  spanJ.push(kJ);
                  if (this.hiddenColorCells.has(kI)) spanIHidden = true;
                  if (this.hiddenColorCells.has(kJ)) spanJHidden = true;
                }

                // Always record span geometry for right-click toggling
                for (const k of spanI) this.repeatSpans.set(k, spanI);
                for (const k of spanJ) this.repeatSpans.set(k, spanJ);

                // Only color instances that are not hidden
                if (!spanIHidden && !spanJHidden) {
                  let phraseKey = '';
                  for (let m = 0; m < L; ++m) phraseKey += tokens[i + m].clean + '|';

                  let phraseCol = phraseColours.get(phraseKey);
                  if (!phraseCol) {
                    phraseCol = palette[nextColorIdx % palette.length];
                    phraseColours.set(phraseKey, phraseCol);
                    nextColorIdx++;
                  }

                  for (const k of spanI) {
                    if (!repeatCellMap.has(k)) repeatCellMap.set(k, phraseCol);
                  }
                  for (const k of spanJ) {
                    if (!repeatCellMap.has(k)) repeatCellMap.set(k, phraseCol);
                  }
                }
              }
            }
          }
        }
      }

      for (let b = startBar; b < endBar; b++) {
        const bar = tab.bars[b];
        const row = document.createElement('div');
        row.id = `bar-row-${b}`;
        const isLineSelected = this.selectedCells.size > 0 && Array.from(this.selectedCells).some(id => id.startsWith(`${b}-`));
        const isEditingLine = this.activeEditor && this.activeEditor.b === b;
        const isActiveLine = isLineSelected || isEditingLine;
        row.className = `bar-row ${b % 2 === 1 ? 'even-line' : ''} ${bar.stanzaBreak ? 'stanza-break' : ''} ${isActiveLine ? 'active-line' : ''} ${this.rowHeight < 36 ? 'compressed-row' : ''}`;
        if (this.rowHeight && this.rowHeight !== 50) {
          row.style.height = `${this.rowHeight}px`;
        }

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

        // Static DAW Structural Beat Grid Lines (Requirement 5)
        const bpb = bar.notation.beatsPerBar || 4;
        for (let beat = 1; beat < bpb; ++beat) {
          const beatLine = document.createElement('div');
          beatLine.className = 'daw-beat-line';
          beatLine.style.left = `${(beat / bpb) * 100}%`;
          grid.appendChild(beatLine);
        }

        const totalDuration = bar.notation.getTotalDuration();
        const numPulses = bar.syllables.length;

        bar.syllables.forEach((pulseArr, pIdx) => {
          const pulseBox = document.createElement('div');
          pulseBox.className = 'pulse-group-box';

          // Proportional Flex Mapping (Requirement 1)
          const pDur = bar.notation.getPulseDuration(pIdx);
          const flexRatio = totalDuration > 0 ? (pDur / totalDuration) : (1 / numPulses);
          pulseBox.style.flex = `${(flexRatio * 100).toFixed(4)} 1 0%`;

          // Tuplet Bracket (Requirement 2)
          const bracket = document.createElement('div');
          bracket.className = 'tuplet-bracket';
          const bracketLine = document.createElement('div');
          bracketLine.className = 'tuplet-bracket-line';
          const numeral = document.createElement('div');
          numeral.className = 'tuplet-numeral';
          numeral.textContent = pulseArr.length;
          bracket.appendChild(bracketLine);
          bracket.appendChild(numeral);
          pulseBox.appendChild(bracket);

          pulseArr.forEach((syl, sIdx) => {
            const cell = document.createElement('div');
            cell.className = 'syllable-cell' + (sIdx === 0 ? ' stress-cell' : '');
            const cellKey = `${b}-${pIdx}-${sIdx}`;
            cell.id = `cell-${cellKey}`;

            // Rhyme, Repeat, or Custom Highlight Tint
            if (this.colorMode === 'off') {
              // Strictly no cell highlights in Colors Off mode
            } else if (this.colorMode === 'repeats') {
              const repColor = repeatCellMap.get(cellKey);
              if (repColor && !this.hiddenColorCells.has(cellKey)) {
                cell.style.backgroundColor = repColor;
              }
            } else if (this.colorMode === 'rhymes') {
              if (syl.customColor && syl.customColor !== 'transparent') {
                cell.style.backgroundColor = syl.customColor;
              } else if (!syl.customColor && syl.text.trim() && !this.hiddenColorCells.has(cellKey)) {
                let prevSyl = '', nextSyl = '';
                const allSylsInBar = [];
                bar.syllables.forEach(p => p.forEach(s => allSylsInBar.push(s)));
                let curLinearIdx = -1;
                let counter = 0;
                bar.syllables.forEach((p, pi) => p.forEach((s, si) => {
                  if (pi === pIdx && si === sIdx) curLinearIdx = counter;
                  counter++;
                }));
                if (curLinearIdx > 0 && allSylsInBar[curLinearIdx - 1]) prevSyl = allSylsInBar[curLinearIdx - 1].text.trim();
                if (curLinearIdx + 1 < allSylsInBar.length && allSylsInBar[curLinearIdx + 1]) nextSyl = allSylsInBar[curLinearIdx + 1].text.trim();

                const rKey = RhymeClassifier.extractRhymeKeyWithContext(syl.text.trim(), prevSyl, nextSyl);
                if (rKey) {
                  cell.style.backgroundColor = this.getVowelColor(rKey);
                }
              }
            }

            // Cell text element
            const textSpan = document.createElement('span');
            textSpan.className = `cell-text align-${syl.align} ${syl.bold ? 'bold' : ''} ${!syl.text ? 'empty' : ''}`;
            textSpan.textContent = syl.text;
            cell.appendChild(textSpan);

            // Alignment Controls (Displayed concurrently across cells when showAlignmentControls is true)
            const alignPanel = document.createElement('div');
            alignPanel.className = 'cell-align-panel';
            alignPanel.innerHTML = `
              <button class="align-arrow-left ${syl.align === 'left' ? 'active' : ''}" title="Align Left">←</button>
              <div class="align-underline-wrapper"><div class="align-underline-bar ${syl.align === 'center' ? 'active' : ''}" title="Align Center / Down"></div></div>
              <button class="align-arrow-right ${syl.align === 'right' ? 'active' : ''}" title="Align Right">→</button>
            `;

            alignPanel.querySelector('.align-arrow-left').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlignment(b, pIdx, sIdx, 'left');
            });
            alignPanel.querySelector('.align-underline-wrapper').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlignment(b, pIdx, sIdx, 'center');
            });
            alignPanel.querySelector('.align-arrow-right').addEventListener('click', (e) => {
              e.stopPropagation();
              this.setCellAlignment(b, pIdx, sIdx, 'right');
            });

            cell.appendChild(alignPanel);

            // Corner Add Button (+ alters line meter at closest corner)
            const cornerAddBtn = document.createElement('button');
            cornerAddBtn.className = 'cell-corner-add-btn right';
            cornerAddBtn.textContent = '+';
            cornerAddBtn.title = 'Add syllable box to line meter (+1 subdivision)';
            cornerAddBtn.addEventListener('click', (e) => {
              e.stopPropagation();
              const insertBefore = cornerAddBtn.classList.contains('left');
              this.insertSyllableInBar(b, pIdx, sIdx, !insertBefore);
            });
            cell.appendChild(cornerAddBtn);

            // Centered Below-Border Minus Button (- removes cell from meter)
            const bottomRemoveBtn = document.createElement('button');
            bottomRemoveBtn.className = 'cell-bottom-remove-btn';
            bottomRemoveBtn.textContent = '-';
            bottomRemoveBtn.title = 'Remove syllable box from line meter (-1 subdivision)';
            bottomRemoveBtn.addEventListener('click', (e) => {
              e.stopPropagation();
              this.deleteSyllableInBar(b, pIdx, sIdx);
            });
            cell.appendChild(bottomRemoveBtn);

            // Mouse move updates corner + position to closest bottom corner
            cell.addEventListener('mousemove', (e) => {
              const rect = cell.getBoundingClientRect();
              const relX = e.clientX - rect.left;
              if (relX < rect.width / 2) {
                if (!cornerAddBtn.classList.contains('left')) {
                  cornerAddBtn.classList.remove('right');
                  cornerAddBtn.classList.add('left');
                  cornerAddBtn.title = 'Insert syllable box before (+1 subdivision)';
                }
              } else {
                if (!cornerAddBtn.classList.contains('right')) {
                  cornerAddBtn.classList.remove('left');
                  cornerAddBtn.classList.add('right');
                  cornerAddBtn.title = 'Insert syllable box after (+1 subdivision)';
                }
              }
            });

            // Cell Click & Drag Selection
            cell.addEventListener('mousedown', (e) => {
              if (e.target.closest('.cell-align-panel') || e.target.closest('.cell-corner-add-btn') || e.target.closest('.cell-bottom-remove-btn')) return;
              if (e.button !== 0) return; // Only left click
              this.isDraggingSelection = true;
              this.dragStartCell = { b, pIdx, sIdx };
              
              if (!e.shiftKey && !e.ctrlKey) {
                this.selectedCells.clear();
              }
              this.selectedCells.add(`${b}-${pIdx}-${sIdx}`);
              this.updateCellSelectionVisuals();
            });

            cell.addEventListener('mouseenter', (e) => {
              if (this.isDraggingSelection) {
                this.selectedCells.add(`${b}-${pIdx}-${sIdx}`);
                this.updateCellSelectionVisuals();
              }
            });

            cell.addEventListener('click', (e) => {
              if (e.target.closest('.cell-align-panel') || e.target.closest('.cell-corner-add-btn') || e.target.closest('.cell-bottom-remove-btn')) return;
              if (this.selectedCells.size <= 1) {
                this.activateCellEditor(b, pIdx, sIdx);
              }
            });

            // Cell Context Menu (Right Click)
            cell.addEventListener('contextmenu', (e) => {
              e.preventDefault();
              if (this.selectedCells.size <= 1 || !this.selectedCells.has(`${b}-${pIdx}-${sIdx}`)) {
                  this.selectedCells.clear();
                  this.selectedCells.add(`${b}-${pIdx}-${sIdx}`);
                  this.updateCellSelectionVisuals();
              }
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
        counterWrap.querySelector('.syl-count-badge').addEventListener('contextmenu', (e) => {
          e.preventDefault();
          this.showCounterMenu(e.clientX, e.clientY, b);
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

      // Input Event: Live Text Update (and Mobile Spacebar Fallback)
      input.addEventListener('input', (e) => {
        const val = input.value;

        // Mobile Paste via IME suggestion bar (doesn't trigger native 'paste' event)
        const isPaste = e.inputType === 'insertFromPaste' || 
                        (val.trim().includes(' ') && Math.abs(val.length - (syl.text || '').length) > 1);
        if (isPaste && val.length > 1) {
          syl.text = ''; // clear it out
          this.closeActiveEditor();
          this.distributePastedWords(b, pIdx, sIdx, val);
          return;
        }
        
        // Mobile IME check for Space/Hyphen
        if (val.endsWith(' ') || val.endsWith('-')) {
          const isHyphen = val.endsWith('-');
          let cleanVal = val.slice(0, -1).trim();
          if (isHyphen && cleanVal) cleanVal += '-';
          
          syl.text = cleanVal;
          this.closeActiveEditor();
          this.advanceFocus(b, pIdx, sIdx, 1);
          return;
        }

        syl.text = val;
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

        // Ctrl+X (Cut): Guarantee text is copied to clipboard AND immediately removed from input and cell
        if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === 'x') {
          e.preventDefault();
          const start = input.selectionStart;
          const end = input.selectionEnd;
          let cutText = '';
          if (start !== end) {
            cutText = input.value.substring(start, end);
            input.value = input.value.substring(0, start) + input.value.substring(end);
            input.setSelectionRange(start, start);
          } else {
            cutText = input.value;
            input.value = '';
          }
          syl.text = input.value;
          this.updateRowCounter(b);
          if (cutText) {
            navigator.clipboard.writeText(cutText).catch(() => {});
          }
          return;
        }

        // Alt+Left / Right / Up / Down OR Ctrl+Left / Right / Up / Down: Alignment
        if (((e.ctrlKey || e.metaKey) || e.altKey) && e.key === 'ArrowLeft') {
          e.preventDefault();
          this.setCellAlignment(b, pIdx, sIdx, 'left');
          input.className = `cell-active-input align-left ${syl.bold ? 'bold' : ''}`;
          return;
        }
        if (((e.ctrlKey || e.metaKey) || e.altKey) && e.key === 'ArrowRight') {
          e.preventDefault();
          this.setCellAlignment(b, pIdx, sIdx, 'right');
          input.className = `cell-active-input align-right ${syl.bold ? 'bold' : ''}`;
          return;
        }
        if (((e.ctrlKey || e.metaKey) || e.altKey) && (e.key === 'ArrowUp' || e.key === 'ArrowDown')) {
          e.preventDefault();
          this.setCellAlignment(b, pIdx, sIdx, 'center');
          input.className = `cell-active-input align-center ${syl.bold ? 'bold' : ''}`;
          return;
        }

        // Alt+= / Alt+Shift+= / Alt+- / Insert / Delete: Direct Meter Altering
        if (e.altKey && (e.key === '=' || e.key === '+' || e.key === 'Insert')) {
          e.preventDefault();
          const insertBefore = e.shiftKey;
          this.closeActiveEditor();
          this.insertSyllableInBar(b, pIdx, sIdx, !insertBefore);
          return;
        }
        if (e.altKey && (e.key === '-' || e.key === 'Delete')) {
          e.preventDefault();
          this.closeActiveEditor();
          this.deleteSyllableInBar(b, pIdx, sIdx);
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
      let itemsToDistribute = [];
      if (this.autoSplitEnabled) {
          words.forEach(w => { itemsToDistribute.push(...SyllableSplitter.splitWord(w)); });
      } else {
          itemsToDistribute = words;
      }
      
      const bars = this.tabs[this.activeTabIdx].bars;

      let curB = b;
      let curP = pIdx;
      let curS = sIdx;

      itemsToDistribute.forEach((w) => {
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
    // 9. Selection Utility Commands
    // =========================================================================
    getSortedSelection() {
       return Array.from(this.selectedCells).map(id => {
           const parts = id.split('-');
           return { b: parseInt(parts[0]), p: parseInt(parts[1]), s: parseInt(parts[2]) };
       }).sort((a, b) => {
           if (a.b !== b.b) return a.b - b.b;
           if (a.p !== b.p) return a.p - b.p;
           return a.s - b.s;
       });
    }

    toggleBoldSelection() {
       if (this.selectedCells.size === 0) return;
       this.pushSnapshot();
       const cells = this.getSortedSelection();
       const targetBold = !this.tabs[this.activeTabIdx].bars[cells[0].b].syllables[cells[0].p][cells[0].s].bold;
       cells.forEach(c => {
           this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].bold = targetBold;
       });
       this.renderPage();
    }

    copySelection() {
       if (this.selectedCells.size === 0) return;
       const cells = this.getSortedSelection();
       const text = cells.map(c => this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].text).join(' ');
       navigator.clipboard.writeText(text);
    }

    cutSelection() {
       if (this.selectedCells.size === 0) return;
       this.pushSnapshot();
       const cells = this.getSortedSelection();
       const text = cells.map(c => this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].text).filter(t => t.length > 0).join(' ');
       cells.forEach(c => {
           this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].text = '';
       });
       if (text) {
         navigator.clipboard.writeText(text).catch(() => {});
       }
       this.renderPage();
    }

    pasteIntoSelection() {
       if (this.selectedCells.size === 0) return;
       const first = this.getSortedSelection()[0];
       navigator.clipboard.readText().then(text => {
           if (text) this.distributePastedWords(first.b, first.p, first.s, text);
       });
    }

    clearSelection() {
       if (this.selectedCells.size === 0) return;
       this.pushSnapshot();
       this.getSortedSelection().forEach(c => {
           this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].text = '';
       });
       this.renderPage();
    }

    joinSelection() {
       if (this.selectedCells.size <= 1) return;
       this.pushSnapshot();
       const cells = this.getSortedSelection();
       const first = cells.shift();
       const bar = this.tabs[this.activeTabIdx].bars[first.b];
       
       let combinedText = bar.syllables[first.p][first.s].text || '';
       cells.forEach(c => {
           combinedText += this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].text || '';
           this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s].text = '';
       });
       bar.syllables[first.p][first.s].text = combinedText;
       this.renderPage();
    }

    splitSelection() {
       if (this.selectedCells.size === 0) return;
       this.pushSnapshot();
       const cells = this.getSortedSelection();
       cells.forEach(c => {
           const sylObj = this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s];
           if (sylObj.text) {
               const split = SyllableSplitter.splitWord(sylObj.text);
               if (split.length > 1) {
                   sylObj.text = ''; // Clear original
                   this.distributePastedWords(c.b, c.p, c.s, split.join(' '));
               }
           }
       });
    }

    // =========================================================================
    // 10. Context Menus & Dialogs
    // =========================================================================
    showCellContextMenu(x, y, b, pIdx, sIdx) {
      const syl = this.tabs[this.activeTabIdx].bars[b].syllables[pIdx][sIdx];

      // Context for syllable
      let prevSyl = '', nextSyl = '';
      const allSylsInBar = [];
      const bar = this.tabs[this.activeTabIdx].bars[b];
      bar.syllables.forEach(p => p.forEach(s => allSylsInBar.push(s)));
      let curLinearIdx = -1;
      let counter = 0;
      bar.syllables.forEach((p, pi) => p.forEach((s, si) => {
        if (pi === pIdx && si === sIdx) curLinearIdx = counter;
        counter++;
      }));
      if (curLinearIdx > 0 && allSylsInBar[curLinearIdx - 1]) prevSyl = allSylsInBar[curLinearIdx - 1].text.trim();
      if (curLinearIdx + 1 < allSylsInBar.length && allSylsInBar[curLinearIdx + 1]) nextSyl = allSylsInBar[curLinearIdx + 1].text.trim();

      const rhymeKey = RhymeClassifier.extractRhymeKeyWithContext(syl.text, prevSyl, nextSyl);
      const catEntry = VOWEL_CATALOG.find(v => v.key === rhymeKey);
      const headerTitle = syl.text 
        ? (catEntry ? `Syllable: "${syl.text}" [${catEntry.label} - ${catEntry.examples}]` : `Syllable: "${syl.text}" [${rhymeKey}]`)
        : 'Syllable: (empty)';

      this.contextMenu.style.left = `${Math.min(x, window.innerWidth - 220)}px`;
      this.contextMenu.style.top = `${Math.min(y, window.innerHeight - 380)}px`;

      const cellKey = `${b}-${pIdx}-${sIdx}`;
      const isSeqHidden = this.hiddenColorCells.has(cellKey);
      const canDeleteBox = bar.notation.pulseSubdivs[pIdx] > 1;
      const VIVID_SWATCHES = ['#EF4444', '#F97316', '#F59E0B', '#84CC16', '#22C55E', '#10B981', '#06B6D4', '#3B82F6', '#6366F1', '#8B5CF6', '#D946EF', '#F43F5E'];
      this.contextMenu.innerHTML = `
        <div class="popup-menu-header">${headerTitle}</div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Highlight Color ▶</div>
          <div class="popup-submenu" style="max-height: 280px; overflow-y: auto;">
            <div class="popup-menu-item" id="ctx-col-auto">Auto (Phonemic Vowel Tint)</div>
            <div class="popup-menu-separator"></div>
            ${VOWEL_CATALOG.map((entry, idx) => `
              <div class="popup-menu-item" id="ctx-vowel-col-${idx}" style="display:flex;align-items:center;gap:8px;">
                <span style="display:inline-block;width:14px;height:14px;border-radius:3px;background-color:${this.getVowelColor(entry.key)};border:1px solid rgba(0,0,0,0.25);flex-shrink:0;"></span>
                <span>${entry.label} (${entry.examples})</span>
              </div>
            `).join('')}
            <div class="popup-menu-separator"></div>
            <div class="popup-menu-item" id="ctx-col-clear">Clear Highlight (None)</div>
            <div class="popup-menu-item" id="ctx-col-clear-all">Clear All Custom Colors in Song</div>
            <div class="popup-menu-item" id="ctx-vowel-customizer">Customize Vowel Color Scheme...</div>
          </div>
        </div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Custom Color ▶</div>
          <div class="popup-submenu" style="width: 140px; transform: translateY(-30px);">
             <div class="palette-grid">
               ${VIVID_SWATCHES.map((hex, idx) => `<div class="palette-swatch" id="ctx-cust-col-${idx}" style="background-color: ${hex}" title="${hex}"></div>`).join('')}
             </div>
          </div>
        </div>
        <div class="popup-menu-item ${isSeqHidden ? 'active-item' : ''}" id="ctx-hide-seq">
          ${isSeqHidden ? '✓ ' : '&nbsp;&nbsp;'}Hide Rhyme Color Pair for This Sequence
        </div>
        ${this.hiddenColorCells.size > 0 ? `<div class="popup-menu-item" id="ctx-unhide-all">Unhide All Rhyme Color Pairs</div>` : ''}
        <div class="popup-menu-separator"></div>
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
        <div class="popup-menu-item" id="ctx-paste">Paste (Ctrl+V)</div>
        <div class="popup-menu-separator"></div>
        <div class="popup-menu-item" id="ctx-syl-ins-before">Insert Syllable Before (+Meter)</div>
        <div class="popup-menu-item" id="ctx-syl-ins-after">Insert Syllable After (+Meter)</div>
        <div class="popup-menu-item ${canDeleteBox ? '' : 'disabled'}" id="ctx-syl-delete" ${canDeleteBox ? '' : 'style="opacity:0.4;pointer-events:none;"'}>Delete Syllable Box (-Meter)</div>
        <div class="popup-menu-separator"></div>
        <div class="popup-menu-item" id="ctx-join">Join Cells (Ctrl+J)</div>
        <div class="popup-menu-item" id="ctx-split">Split Syllables (Ctrl+K)</div>
        <div class="popup-menu-item" id="ctx-clear">Clear Cell (Del)</div>
        <div class="popup-menu-separator"></div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Tuplet Brackets ▶</div>
          <div class="popup-submenu">
            <div class="popup-menu-item" id="ctx-tuplet-all-on">${this.tupletBracketMode === 'all_on' ? '✓ ' : '&nbsp;&nbsp;'}All Lines (Always On)</div>
            <div class="popup-menu-item" id="ctx-tuplet-active-line">${this.tupletBracketMode === 'active_line' ? '✓ ' : '&nbsp;&nbsp;'}Active Line Only</div>
            <div class="popup-menu-item" id="ctx-tuplet-all-off">${this.tupletBracketMode === 'all_off' ? '✓ ' : '&nbsp;&nbsp;'}All Off (Hidden)</div>
          </div>
        </div>
      `;
      this.contextMenu.style.display = 'block';

      const applyToSelection = (fn) => {
        if (this.selectedCells.size > 1) {
           this.getSortedSelection().forEach(c => fn(this.tabs[this.activeTabIdx].bars[c.b].syllables[c.p][c.s], c.b, c.p, c.s));
        } else {
           fn(syl, b, pIdx, sIdx);
        }
      };

      const hideSeqBtn = document.getElementById('ctx-hide-seq');
      if (hideSeqBtn) {
        hideSeqBtn.onclick = () => {
          this.pushSnapshot();
          const span = this.repeatSpans.get(cellKey) || [cellKey];
          if (isSeqHidden) {
            span.forEach(k => this.hiddenColorCells.delete(k));
          } else {
            span.forEach(k => this.hiddenColorCells.add(k));
          }
          this.contextMenu.style.display = 'none';
          this.renderPage();
        };
      }

      const unhideAllBtn = document.getElementById('ctx-unhide-all');
      if (unhideAllBtn) {
        unhideAllBtn.onclick = () => {
          this.pushSnapshot();
          this.hiddenColorCells.clear();
          this.contextMenu.style.display = 'none';
          this.renderPage();
        };
      }

      // Color actions
      document.getElementById('ctx-col-auto').onclick = () => {
        this.pushSnapshot();
        applyToSelection(s => s.customColor = null);
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      VOWEL_CATALOG.forEach((entry, idx) => {
        const el = document.getElementById(`ctx-vowel-col-${idx}`);
        if (el) el.onclick = () => {
            this.pushSnapshot();
            applyToSelection(s => s.customColor = this.getVowelColor(entry.key));
            this.contextMenu.style.display = 'none';
            this.renderPage();
        };
      });
      const custBtn = document.getElementById('ctx-vowel-customizer');
      if (custBtn) custBtn.onclick = () => {
        this.contextMenu.style.display = 'none';
        this.openVowelPaletteDialog();
      };
      VIVID_SWATCHES.forEach((hex, idx) => {
        const el = document.getElementById(`ctx-cust-col-${idx}`);
        if (el) el.onclick = () => {
            this.pushSnapshot();
            applyToSelection(s => s.customColor = hex);
            this.contextMenu.style.display = 'none';
            this.renderPage();
        };
      });
      document.getElementById('ctx-col-clear').onclick = () => {
        this.pushSnapshot();
        applyToSelection(s => s.customColor = 'transparent');
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      const clearAllSongBtn = document.getElementById('ctx-col-clear-all');
      if (clearAllSongBtn) clearAllSongBtn.onclick = () => {
        this.contextMenu.style.display = 'none';
        this.clearAllCustomColors();
      };

      // Syllable insertion & deletion (meter alteration)
      document.getElementById('ctx-syl-ins-before').onclick = () => {
        this.contextMenu.style.display = 'none';
        this.insertSyllableInBar(b, pIdx, sIdx, false);
      };
      document.getElementById('ctx-syl-ins-after').onclick = () => {
        this.contextMenu.style.display = 'none';
        this.insertSyllableInBar(b, pIdx, sIdx, true);
      };
      const delBoxBtn = document.getElementById('ctx-syl-delete');
      if (delBoxBtn && canDeleteBox) {
        delBoxBtn.onclick = () => {
          this.contextMenu.style.display = 'none';
          this.deleteSyllableInBar(b, pIdx, sIdx);
        };
      }

      // Case transforms
      document.getElementById('ctx-case-upper').onclick = () => {
        this.pushSnapshot();
        applyToSelection(s => s.text = s.text.toUpperCase());
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      document.getElementById('ctx-case-lower').onclick = () => {
        this.pushSnapshot();
        applyToSelection(s => s.text = s.text.toLowerCase());
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      document.getElementById('ctx-case-title').onclick = () => {
        this.pushSnapshot();
        applyToSelection(s => s.text = s.text.charAt(0).toUpperCase() + s.text.slice(1).toLowerCase());
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };

      // Alignment
      document.getElementById('ctx-align-left').onclick = () => {
        this.pushSnapshot();
        applyToSelection((s, cb, cp, cs) => this.setCellAlignment(cb, cp, cs, 'left'));
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-align-center').onclick = () => {
        this.pushSnapshot();
        applyToSelection((s, cb, cp, cs) => this.setCellAlignment(cb, cp, cs, 'center'));
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-align-right').onclick = () => {
        this.pushSnapshot();
        applyToSelection((s, cb, cp, cs) => this.setCellAlignment(cb, cp, cs, 'right'));
        this.contextMenu.style.display = 'none';
      };

      // Bold
      document.getElementById('ctx-bold').onclick = () => {
        this.toggleBoldSelection();
        this.contextMenu.style.display = 'none';
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
        this.cutSelection();
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-copy').onclick = () => {
        this.copySelection();
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-paste').onclick = () => {
        this.pasteIntoSelection();
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-join').onclick = () => {
        this.joinSelection();
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-split').onclick = () => {
        this.splitSelection();
        this.contextMenu.style.display = 'none';
      };
      document.getElementById('ctx-clear').onclick = () => {
        this.clearSelection();
        this.contextMenu.style.display = 'none';
      };

      const tAllOn = document.getElementById('ctx-tuplet-all-on');
      if (tAllOn) tAllOn.onclick = () => { this.setTupletBracketMode('all_on'); this.contextMenu.style.display = 'none'; };
      const tActLine = document.getElementById('ctx-tuplet-active-line');
      if (tActLine) tActLine.onclick = () => { this.setTupletBracketMode('active_line'); this.contextMenu.style.display = 'none'; };
      const tAllOff = document.getElementById('ctx-tuplet-all-off');
      if (tAllOff) tAllOff.onclick = () => { this.setTupletBracketMode('all_off'); this.contextMenu.style.display = 'none'; };
    }

    showCounterMenu(x, y, b) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      this.contextMenu.style.left = `${Math.min(x, window.innerWidth - 200)}px`;
      this.contextMenu.style.top = `${Math.min(y, window.innerHeight - 100)}px`;

      this.contextMenu.innerHTML = `
        <div class="popup-menu-item" id="ctx-reset-counter">Reset to Calculated Count (X)</div>
      `;
      this.contextMenu.style.display = 'block';

      document.getElementById('ctx-reset-counter').onclick = () => {
        this.pushSnapshot();
        bar.customSpokenCount = null;
        this.updateRowCounter(b);
        this.contextMenu.style.display = 'none';
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

    openVowelPaletteDialog() {
      if (!this.vowelPaletteDialog) return;

      const colorToHex = (col) => {
        if (!col) return '#cbd5e1';
        if (col.startsWith('#')) {
          if (col.length === 4) return '#' + col[1] + col[1] + col[2] + col[2] + col[3] + col[3];
          return col.slice(0, 7);
        }
        const m = col.match(/rgba?\((\d+),\s*(\d+),\s*(\d+)/);
        if (m) {
          const r = parseInt(m[1], 10).toString(16).padStart(2, '0');
          const g = parseInt(m[2], 10).toString(16).padStart(2, '0');
          const b = parseInt(m[3], 10).toString(16).padStart(2, '0');
          return `#${r}${g}${b}`;
        }
        return '#cbd5e1';
      };

      const hexToRgba = (hex, alpha = 0.65) => {
        let c = hex.replace('#', '');
        if (c.length === 3) c = c[0] + c[0] + c[1] + c[1] + c[2] + c[2];
        const num = parseInt(c, 16);
        const r = (num >> 16) & 255;
        const g = (num >> 8) & 255;
        const b = num & 255;
        return `rgba(${r}, ${g}, ${b}, ${alpha})`;
      };

      const renderBody = () => {
        const bodyEl = document.getElementById('vowel-palette-body');
        if (!bodyEl) return;
        bodyEl.innerHTML = VOWEL_CATALOG.map((entry) => {
          const curCol = this.getVowelColor(entry.key);
          const hexCol = colorToHex(curCol);
          return `
            <div class="vowel-row">
              <div class="vowel-info">
                <span class="vowel-label">${entry.label}</span>
                <span class="vowel-examples">"${entry.examples}"</span>
              </div>
              <div class="vowel-swatch-picker">
                <div class="vowel-swatch-box" id="vowel-swatch-box-${entry.key}" style="background-color: ${curCol};" title="Click to choose color"></div>
                <input type="color" class="vowel-color-input" id="vowel-picker-${entry.key}" value="${hexCol}">
              </div>
            </div>
          `;
        }).join('');

        VOWEL_CATALOG.forEach((entry) => {
          const picker = document.getElementById(`vowel-picker-${entry.key}`);
          const box = document.getElementById(`vowel-swatch-box-${entry.key}`);
          if (box && picker) {
            box.onclick = () => picker.click();
          }
          if (picker) {
            const updateColor = (e) => {
              const newRgba = hexToRgba(e.target.value, 0.65);
              if (box) box.style.backgroundColor = newRgba;
              this.setVowelColor(entry.key, newRgba);
            };
            picker.addEventListener('input', updateColor);
            picker.addEventListener('change', updateColor);
          }
        });
      };

      this.vowelPaletteDialog.className = 'modal-overlay';
      this.vowelPaletteDialog.style.display = 'flex';
      this.vowelPaletteDialog.innerHTML = `
        <div class="modal-content" onclick="event.stopPropagation()">
          <div class="modal-header">
            <h3>Customize Color Palette & Repetition Rules</h3>
            <button class="vst-btn" id="vowel-palette-close-btn" style="font-size:16px;line-height:1;padding:2px 8px;">&times;</button>
          </div>
          <div class="vowel-palette-repeats-row" style="display:flex;align-items:center;justify-content:space-between;padding:8px 14px;border-bottom:1px solid var(--notebook-rule);">
            <span style="font-weight:600;font-size:13px;color:var(--text-main);" title="Minimum syllable count for repeated sequence detection (1 to 32).">Repeats Minimum Syllables:</span>
            <div style="display:flex;align-items:center;gap:6px;">
              <input type="range" id="vowel-rep-syl-slider" min="1" max="32" value="${this.minRepeatLength}" style="width:110px;cursor:pointer;">
              <input type="number" id="vowel-rep-syl-input" min="1" max="32" value="${this.minRepeatLength}" style="width:48px;height:24px;font-size:12px;font-weight:600;text-align:center;border:1px solid var(--btn-border);border-radius:3px;background:var(--btn-bg);color:var(--graphite);">
              <span id="vowel-rep-syl-label" style="font-size:12px;font-weight:600;width:35px;color:var(--text-main);">syl</span>
            </div>
          </div>
          <div class="vowel-palette-distance-row" style="display:flex;align-items:center;justify-content:space-between;padding:8px 14px;border-bottom:1px solid var(--notebook-rule);margin-bottom:8px;">
            <span style="font-weight:600;font-size:13px;color:var(--text-main);" title="Maximum lines apart for a repeated sequence match. 0 = same line only, 1 = couplet, 24 = default baseline, up to 256.">Repeats Max Line Distance:</span>
            <div style="display:flex;align-items:center;gap:6px;">
              <input type="range" id="vowel-rep-dist-slider" min="0" max="256" value="${this.maxRepeatLineDistance}" style="width:110px;cursor:pointer;">
              <input type="number" id="vowel-rep-dist-input" min="0" max="256" value="${this.maxRepeatLineDistance}" style="width:48px;height:24px;font-size:12px;font-weight:600;text-align:center;border:1px solid var(--btn-border);border-radius:3px;background:var(--btn-bg);color:var(--graphite);">
              <span id="vowel-rep-dist-label" style="font-size:12px;font-weight:600;width:35px;color:var(--text-main);">lines</span>
            </div>
          </div>
          <div class="modal-body" id="vowel-palette-body"></div>
          <div class="modal-footer">
            <button class="vst-btn" id="vowel-palette-reset-btn">Reset to Defaults</button>
            <button class="vst-btn toggled" id="vowel-palette-done-btn">Done</button>
          </div>
        </div>
      `;

      const setupRepeatInputs = () => {
        const sylSlider = document.getElementById('vowel-rep-syl-slider');
        const sylInput = document.getElementById('vowel-rep-syl-input');
        const setSyl = (val) => {
          val = Math.max(1, Math.min(32, parseInt(val, 10) || 1));
          this.minRepeatLength = val;
          if (sylSlider) sylSlider.value = val;
          if (sylInput) sylInput.value = val;
          this.updateColorModeButton();
          this.renderPage();
        };
        if (sylSlider) sylSlider.oninput = (e) => setSyl(e.target.value);
        if (sylInput) sylInput.onchange = (e) => setSyl(e.target.value);

        const distSlider = document.getElementById('vowel-rep-dist-slider');
        const distInput = document.getElementById('vowel-rep-dist-input');
        const setDist = (val) => {
          val = Math.max(0, Math.min(512, parseInt(val, 10) || 0));
          this.maxRepeatLineDistance = val;
          localStorage.setItem('cc_max_repeat_line_dist', val.toString());
          if (distSlider) distSlider.value = val;
          if (distInput) distInput.value = val;
          this.renderPage();
        };
        if (distSlider) distSlider.oninput = (e) => setDist(e.target.value);
        if (distInput) distInput.onchange = (e) => setDist(e.target.value);
      };
      setupRepeatInputs();

      renderBody();

      this.vowelPaletteDialog.onclick = (e) => {
        if (e.target === this.vowelPaletteDialog) {
          this.vowelPaletteDialog.style.display = 'none';
        }
      };

      const closeBtn = document.getElementById('vowel-palette-close-btn');
      if (closeBtn) closeBtn.onclick = () => {
        this.vowelPaletteDialog.style.display = 'none';
      };

      const doneBtn = document.getElementById('vowel-palette-done-btn');
      if (doneBtn) doneBtn.onclick = () => {
        this.vowelPaletteDialog.style.display = 'none';
      };

      const resetBtn = document.getElementById('vowel-palette-reset-btn');
      if (resetBtn) resetBtn.onclick = () => {
        this.resetVowelColorsToDefaults();
        renderBody();
      };
    }

    openShortcutsDialog() {
      if (!this.shortcutsDialog) return;

      const shortcutsData = [
        {
          category: 'Navigation & Typing Flow',
          items: [
            { key: 'Space', desc: 'Toggle Transport Play/Stop (or advance typing flow in box)' },
            { key: 'Hyphen (-)', desc: 'Append hyphen and advance to next syllable box' },
            { key: 'Tab / Shift+Tab', desc: 'Jump forward / backward through syllable boxes' },
            { key: 'Enter', desc: 'Jump to first syllable box of next line' },
            { key: 'Arrow Keys', desc: 'Navigate text cursor or jump to adjacent boxes' },
            { key: 'Backspace (empty box)', desc: 'Retreat to previous syllable box' }
          ]
        },
        {
          category: 'Meter Altering (Line Subdivisions)',
          items: [
            { key: 'Alt + = / Alt + Insert', desc: 'Insert syllable box after (+1 subdivision to line meter)' },
            { key: 'Alt + Shift + =', desc: 'Insert syllable box before (+1 subdivision to line meter)' },
            { key: 'Alt + - / Alt + Delete', desc: 'Remove syllable box from line meter (-1 subdivision)' },
            { key: 'Hover Bottom Corner (+)', desc: 'Hover bottom-left or bottom-right corner to insert box' },
            { key: 'Below Border (-)', desc: 'Centered button below bottom border to remove cell from meter' }
          ]
        },
        {
          category: 'Formatting & Selection',
          items: [
            { key: 'Alt + Left / Ctrl + Left', desc: 'Align cell text Left' },
            { key: 'Alt + Down / Ctrl + Down', desc: 'Align cell text Center / Down' },
            { key: 'Alt + Right / Ctrl + Right', desc: 'Align cell text Right' },
            { key: 'Ctrl + B', desc: 'Toggle Bold Emphasis notation' },
            { key: 'Click + Drag', desc: 'Select rectangular block of syllable cells' },
            { key: 'Ctrl + J', desc: 'Join selected syllable cells into a single word' },
            { key: 'Ctrl + K', desc: 'Split selected multi-word cell into multiple cells' }
          ]
        },
        {
          category: 'Clipboard & History',
          items: [
            { key: 'Ctrl + X', desc: 'Cut text from box or selection (copies and removes text)' },
            { key: 'Ctrl + C', desc: 'Copy selected cell(s) or full lyric line to clipboard' },
            { key: 'Ctrl + V', desc: 'Paste text with automatic multi-syllable distribution' },
            { key: 'Ctrl + Z', desc: 'Undo last edit or meter alteration' },
            { key: 'Ctrl + Y / Ctrl + Shift + Z', desc: 'Redo last undone action' },
            { key: 'Delete / Backspace', desc: 'Clear text in selected cell(s)' }
          ]
        },
        {
          category: 'View & Controls',
          items: [
            { key: 'Align: OFF / ON', desc: 'Header toggle to concurrently show/hide alignment buttons' },
            { key: '? / F1', desc: 'Open this Keyboard Shortcuts cheat sheet' }
          ]
        }
      ];

      this.shortcutsDialog.className = 'modal-overlay';
      this.shortcutsDialog.style.display = 'flex';
      this.shortcutsDialog.innerHTML = `
        <div class="modal-content" onclick="event.stopPropagation()">
          <div class="modal-header">
            <h3>Keyboard Shortcuts</h3>
            <button class="vst-btn" id="shortcuts-close-btn" style="font-size:16px;line-height:1;padding:2px 8px;">&times;</button>
          </div>
          <div class="modal-body">
            <div class="shortcuts-table">
              ${shortcutsData.map(cat => `
                <div class="shortcut-category">
                  <div class="shortcut-category-title">${cat.category}</div>
                  ${cat.items.map(it => `
                    <div class="shortcut-row">
                      <span class="shortcut-description">${it.desc}</span>
                      <span class="shortcut-key-badge">${it.key}</span>
                    </div>
                  `).join('')}
                </div>
              `).join('')}
            </div>
          </div>
          <div class="modal-footer">
            <button class="vst-btn" id="shortcuts-reset-btn" title="Reset all shortcuts and controls to defaults">Reset to Defaults</button>
            <button class="vst-btn" id="shortcuts-done-btn" style="background:var(--copper-accent);color:#fff;">Close</button>
          </div>
        </div>
      `;

      const closeDialog = () => {
        this.shortcutsDialog.style.display = 'none';
      };

      this.shortcutsDialog.onclick = (e) => {
        if (e.target === this.shortcutsDialog) closeDialog();
      };
      const closeBtn = document.getElementById('shortcuts-close-btn');
      if (closeBtn) closeBtn.onclick = closeDialog;
      const doneBtn = document.getElementById('shortcuts-done-btn');
      if (doneBtn) doneBtn.onclick = closeDialog;

      const resetBtn = document.getElementById('shortcuts-reset-btn');
      if (resetBtn) {
        resetBtn.onclick = () => {
          this.showAlignmentControls = false;
          document.body.classList.remove('show-align-controls');
          if (this.alignToggleBtn) {
            this.alignToggleBtn.classList.remove('toggled');
            this.alignToggleBtn.textContent = 'Align: OFF';
          }
          this.renderPage();
          closeDialog();
        };
      }
    }

    updateColorModeButton() {
      if (!this.rhymeToggleBtn) return;
      if (this.colorMode === 'rhymes') {
        this.rhymeToggleBtn.classList.add('toggled');
        this.rhymeToggleBtn.textContent = 'Rhymes: ON';
        this.rhymeToggleBtn.title = 'Color Mode: Phonetic Rhymes (Click to cycle, right-click for menu)';
      } else if (this.colorMode === 'repeats') {
        this.rhymeToggleBtn.classList.add('toggled');
        this.rhymeToggleBtn.textContent = `Repeats: ${this.minRepeatLength}+`;
        this.rhymeToggleBtn.title = `Color Mode: Exact Syllable Repetitions (${this.minRepeatLength}+) (Click to cycle, right-click for menu)`;
      } else {
        this.rhymeToggleBtn.classList.remove('toggled');
        this.rhymeToggleBtn.textContent = 'Colors: OFF';
        this.rhymeToggleBtn.title = 'Color Mode: Off (Click to cycle, right-click for menu)';
      }
    }

    showColorModeMenu(e) {
      if (!this.contextMenu) return;
      const rect = this.rhymeToggleBtn.getBoundingClientRect();
      const x = e ? e.clientX : rect.left;
      const y = e ? e.clientY : rect.bottom + 4;
      this.contextMenu.style.left = `${Math.min(x, window.innerWidth - 240)}px`;
      this.contextMenu.style.top = `${Math.min(y, window.innerHeight - 200)}px`;

      this.contextMenu.innerHTML = `
        <div class="popup-menu-header">Highlight Color Mode</div>
        <div class="popup-menu-item ${this.colorMode === 'rhymes' ? 'active-item' : ''}" id="cm-rhymes">
          ${this.colorMode === 'rhymes' ? '✓ ' : '&nbsp;&nbsp;'}Rhymes (Phonetic Vowels)
        </div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item ${this.colorMode === 'repeats' ? 'active-item' : ''}">
            ${this.colorMode === 'repeats' ? '✓ ' : '&nbsp;&nbsp;'}Repeats (Exact Matches) ▶
          </div>
          <div class="popup-submenu" style="width: 200px;">
            <div class="popup-menu-item ${this.colorMode === 'repeats' && this.minRepeatLength === 1 ? 'active-item' : ''}" id="cm-rep-1">
              ${this.colorMode === 'repeats' && this.minRepeatLength === 1 ? '✓ ' : '&nbsp;&nbsp;'}1+ Syllables
            </div>
            <div class="popup-menu-item ${this.colorMode === 'repeats' && this.minRepeatLength === 2 ? 'active-item' : ''}" id="cm-rep-2">
              ${this.colorMode === 'repeats' && this.minRepeatLength === 2 ? '✓ ' : '&nbsp;&nbsp;'}2+ Syllables
            </div>
            <div class="popup-menu-item ${this.colorMode === 'repeats' && this.minRepeatLength === 3 ? 'active-item' : ''}" id="cm-rep-3">
              ${this.colorMode === 'repeats' && this.minRepeatLength === 3 ? '✓ ' : '&nbsp;&nbsp;'}3+ Syllables
            </div>
            <div class="popup-menu-item ${this.colorMode === 'repeats' && this.minRepeatLength === 4 ? 'active-item' : ''}" id="cm-rep-4">
              ${this.colorMode === 'repeats' && this.minRepeatLength === 4 ? '✓ ' : '&nbsp;&nbsp;'}4+ Syllables
            </div>
            <div class="popup-menu-item ${this.colorMode === 'repeats' && this.minRepeatLength === 5 ? 'active-item' : ''}" id="cm-rep-5">
              ${this.colorMode === 'repeats' && this.minRepeatLength === 5 ? '✓ ' : '&nbsp;&nbsp;'}5+ Syllables
            </div>
            <div class="popup-menu-item ${this.colorMode === 'repeats' && this.minRepeatLength === 6 ? 'active-item' : ''}" id="cm-rep-6">
              ${this.colorMode === 'repeats' && this.minRepeatLength === 6 ? '✓ ' : '&nbsp;&nbsp;'}6+ Syllables
            </div>
            <div class="popup-menu-item" id="cm-rep-custom">
              &nbsp;&nbsp;Custom Syllable Count (${this.minRepeatLength}+)...
            </div>
            <div class="popup-menu-separator"></div>
            <div class="popup-submenu-container">
              <div class="popup-menu-item">
                Max Distance (${this.maxRepeatLineDistance}L) ▶
              </div>
              <div class="popup-submenu" style="width: 210px;">
                ${[
                  { d: 0, l: '0 Lines (Same Line)' },
                  { d: 1, l: '1 Line (Couplets)' },
                  { d: 2, l: '2 Lines' },
                  { d: 4, l: '4 Lines (1 Stanza)' },
                  { d: 8, l: '8 Lines (2 Stanzas)' },
                  { d: 12, l: '12 Lines' },
                  { d: 16, l: '16 Lines (1 Page)' },
                  { d: 24, l: '24 Lines (Default)' },
                  { d: 32, l: '32 Lines (2 Pages)' },
                  { d: 48, l: '48 Lines' },
                  { d: 64, l: '64 Lines (Full Song)' },
                  { d: 256, l: '256 Lines (Unlimited)' }
                ].map(opt => `
                  <div class="popup-menu-item ${this.maxRepeatLineDistance === opt.d ? 'active-item' : ''}" id="cm-dist-${opt.d}">
                    ${this.maxRepeatLineDistance === opt.d ? '✓ ' : '&nbsp;&nbsp;'}${opt.l}
                  </div>
                `).join('')}
                <div class="popup-menu-separator"></div>
                <div class="popup-menu-item" id="cm-dist-custom">
                  &nbsp;&nbsp;Custom Line Distance (${this.maxRepeatLineDistance} lines)...
                </div>
              </div>
            </div>
          </div>
        </div>
        <div class="popup-menu-separator"></div>
        <div class="popup-menu-item ${this.colorMode === 'off' ? 'active-item' : ''}" id="cm-off">
          ${this.colorMode === 'off' ? '✓ ' : '&nbsp;&nbsp;'}Colors Off
        </div>
        <div class="popup-menu-separator"></div>
        ${this.hiddenColorCells.size > 0 ? `
          <div class="popup-menu-item" id="cm-unhide-all">
            Unhide All Rhyme Color Pairs / Sequences
          </div>
        ` : ''}
        <div class="popup-menu-item" id="cm-clear-all">
          Clear All Custom Colors (Reset to Auto)
        </div>
        <div class="popup-menu-separator"></div>
        <div class="popup-submenu-container">
          <div class="popup-menu-item">Tuplet Brackets ▶</div>
          <div class="popup-submenu">
            <div class="popup-menu-item" id="cm-tuplet-all-on">${this.tupletBracketMode === 'all_on' ? '✓ ' : '&nbsp;&nbsp;'}All Lines (Always On)</div>
            <div class="popup-menu-item" id="cm-tuplet-active-line">${this.tupletBracketMode === 'active_line' ? '✓ ' : '&nbsp;&nbsp;'}Active Line Only</div>
            <div class="popup-menu-item" id="cm-tuplet-all-off">${this.tupletBracketMode === 'all_off' ? '✓ ' : '&nbsp;&nbsp;'}All Off (Hidden)</div>
          </div>
        </div>
      `;
      this.contextMenu.style.display = 'block';

      const selectMode = (mode, minLen = null, dist = null) => {
        this.colorMode = mode;
        if (minLen !== null) this.minRepeatLength = minLen;
        if (dist !== null) {
          this.maxRepeatLineDistance = dist;
          localStorage.setItem('cc_max_repeat_line_dist', dist.toString());
        }
        this.updateColorModeButton();
        this.renderPage();
        this.contextMenu.style.display = 'none';
      };

      const rhymesEl = document.getElementById('cm-rhymes');
      if (rhymesEl) rhymesEl.onclick = () => selectMode('rhymes');

      [1, 2, 3, 4, 5, 6].forEach(len => {
        const el = document.getElementById(`cm-rep-${len}`);
        if (el) el.onclick = () => selectMode('repeats', len);
      });

      const repCustomEl = document.getElementById('cm-rep-custom');
      if (repCustomEl) {
        repCustomEl.onclick = () => {
          this.contextMenu.style.display = 'none';
          const input = prompt('Enter minimum syllable count for repetition match detection (1 to 32):', this.minRepeatLength);
          if (input !== null) {
            const val = parseInt(input.trim(), 10);
            if (!isNaN(val) && val >= 1) {
              selectMode('repeats', Math.min(32, val));
            }
          }
        };
      }

      [0, 1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 256].forEach(d => {
        const el = document.getElementById(`cm-dist-${d}`);
        if (el) el.onclick = () => selectMode('repeats', null, d);
      });

      const distCustomEl = document.getElementById('cm-dist-custom');
      if (distCustomEl) {
        distCustomEl.onclick = () => {
          this.contextMenu.style.display = 'none';
          const input = prompt('Enter maximum line distance for repetition matches (0 = same line, 1 = couplet, 24 = default, up to 256):', this.maxRepeatLineDistance);
          if (input !== null) {
            const val = parseInt(input.trim(), 10);
            if (!isNaN(val) && val >= 0) {
              selectMode('repeats', null, Math.min(512, val));
            }
          }
        };
      }

      const offEl = document.getElementById('cm-off');
      if (offEl) offEl.onclick = () => selectMode('off');
      const unhideAllEl = document.getElementById('cm-unhide-all');
      if (unhideAllEl) unhideAllEl.onclick = () => {
        this.pushSnapshot();
        this.hiddenColorCells.clear();
        this.contextMenu.style.display = 'none';
        this.renderPage();
      };
      const clearAllEl = document.getElementById('cm-clear-all');
      if (clearAllEl) clearAllEl.onclick = () => {
        this.contextMenu.style.display = 'none';
        this.clearAllCustomColors();
      };

      const cmTAllOn = document.getElementById('cm-tuplet-all-on');
      if (cmTAllOn) cmTAllOn.onclick = () => { this.setTupletBracketMode('all_on'); this.contextMenu.style.display = 'none'; };
      const cmTActLine = document.getElementById('cm-tuplet-active-line');
      if (cmTActLine) cmTActLine.onclick = () => { this.setTupletBracketMode('active_line'); this.contextMenu.style.display = 'none'; };
      const cmTAllOff = document.getElementById('cm-tuplet-all-off');
      if (cmTAllOff) cmTAllOff.onclick = () => { this.setTupletBracketMode('all_off'); this.contextMenu.style.display = 'none'; };
    }

    insertSyllableInBar(b, pIdx, sIdx, insertAfter = false) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      if (!bar) return;

      this.pushSnapshot();

      // 1. Increment subdivision count in pulseSubdivs
      bar.notation.pulseSubdivs[pIdx]++;

      // 2. Insert empty syllable in bar.syllables[pIdx]
      const insertAt = insertAfter ? (sIdx + 1) : sIdx;
      bar.syllables[pIdx].splice(insertAt, 0, {
        text: '',
        bold: false,
        align: 'center',
        customColor: null
      });

      // 3. Clear/set selection
      this.selectedCells.clear();
      this.selectedCells.add(`${b}-${pIdx}-${insertAt}`);

      // 4. Render and activate cell editor
      this.renderPage();
      this.activateCellEditor(b, pIdx, insertAt);
    }

    deleteSyllableInBar(b, pIdx, sIdx) {
      const bar = this.tabs[this.activeTabIdx].bars[b];
      if (!bar) return;

      if (bar.notation.pulseSubdivs[pIdx] <= 1) return; // Minimum 1 subdivision per pulse

      this.pushSnapshot();

      // 1. Decrement subdivision count in pulseSubdivs
      bar.notation.pulseSubdivs[pIdx]--;

      // 2. Remove syllable from bar.syllables[pIdx]
      bar.syllables[pIdx].splice(sIdx, 1);

      // 3. Clamp customSpokenCount if needed
      if (bar.customSpokenCount != null && bar.customSpokenCount > bar.notation.getTotalSyllables()) {
        bar.customSpokenCount = bar.notation.getTotalSyllables();
      }

      // 4. Update selection
      this.selectedCells.clear();
      const newSIdx = Math.min(sIdx, bar.syllables[pIdx].length - 1);
      if (newSIdx >= 0) {
        this.selectedCells.add(`${b}-${pIdx}-${newSIdx}`);
      }

      // 5. Render
      this.renderPage();
    }

    clearAllCustomColors() {
      this.pushSnapshot();
      this.tabs.forEach(tab => {
        tab.bars.forEach(bar => {
          bar.syllables.forEach(pulseArr => {
            pulseArr.forEach(s => {
              s.customColor = null;
            });
          });
        });
      });
      this.renderPage();
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
      return `<!DOCTYPE html><html><head><meta charset="utf-8"><title>compass4cadence Lyrics</title><style>body{font-family:Segoe UI,sans-serif;padding:30px;background:#18181b;color:#f1f5f9;}table{border-collapse:collapse;width:100%;}tr:nth-child(even){background:#222227;}th{text-align:left;padding:8px 12px;border-bottom:2px solid #3f3f46;color:#94a3b8;}</style></head><body><h1>compass4cadence — Lyric Sheet</h1><table><thead><tr><th>Bar</th><th>Meter</th><th>Lyrics</th><th>Syllables</th></tr></thead><tbody>${rowsHtml}</tbody></table></body></html>`;
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
        if (bar.customSpokenCount != null && bar.customSpokenCount > bar.notation.getTotalSyllables()) {
          bar.customSpokenCount = bar.notation.getTotalSyllables();
        }
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
