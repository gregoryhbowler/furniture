// Musical scales, note names, MIDI utilities
// PURE — no DOM, no browser APIs

export const NOTE_NAMES = ['C','C#','D','D#','E','F','F#','G','G#','A','A#','B'];

export const SCALES = {
  // Major modes
  'Major (Ionian)':    [0,2,4,5,7,9,11],
  'Dorian':            [0,2,3,5,7,9,10],
  'Phrygian':          [0,1,3,5,7,8,10],
  'Lydian':            [0,2,4,6,7,9,11],
  'Mixolydian':        [0,2,4,5,7,9,10],
  'Minor (Aeolian)':   [0,2,3,5,7,8,10],
  'Locrian':           [0,1,3,5,6,8,10],

  // Melodic / harmonic minor
  'Harmonic Minor':    [0,2,3,5,7,8,11],
  'Melodic Minor':     [0,2,3,5,7,9,11],
  'Harmonic Major':    [0,2,4,5,7,8,11],

  // Pentatonics & blues
  'Pentatonic Major':  [0,2,4,7,9],
  'Pentatonic Minor':  [0,3,5,7,10],
  'Blues':              [0,3,5,6,7,10],
  'Blues Major':        [0,2,3,4,7,9],

  // Symmetric
  'Whole Tone':        [0,2,4,6,8,10],
  'Diminished (HW)':   [0,1,3,4,6,7,9,10],
  'Diminished (WH)':   [0,2,3,5,6,8,9,11],
  'Augmented':         [0,3,4,7,8,11],

  // Exotic
  'Hungarian Minor':   [0,2,3,6,7,8,11],
  'Phrygian Dominant': [0,1,4,5,7,8,10],
  'Double Harmonic':   [0,1,4,5,7,8,11],
  'Hirajoshi':         [0,2,3,7,8],
  'In Sen':            [0,1,5,7,10],
  'Yo':                [0,2,5,7,9],
  'Iwato':             [0,1,5,6,10],
  'Kumoi':             [0,2,3,7,9],
  'Bebop Dominant':    [0,2,4,5,7,9,10,11],
  'Bebop Major':       [0,2,4,5,7,8,9,11],
  'Prometheus':        [0,2,4,6,9,10],
  'Enigmatic':         [0,1,4,6,8,10,11],

  // Chromatic
  'Chromatic':         [0,1,2,3,4,5,6,7,8,9,10,11],
};

export function midiToName(midi) {
  const note = NOTE_NAMES[midi % 12];
  const octave = Math.floor(midi / 12) - 1;
  return note + octave;
}

export function nameToMidi(name, octave) {
  const idx = NOTE_NAMES.indexOf(name);
  if (idx === -1) return 60;
  return (octave + 1) * 12 + idx;
}

export function midiToFrequency(midi) {
  return 440 * Math.pow(2, (midi - 69) / 12);
}

export function getScaleNotes(root, scaleName, octaveLow, octaveHigh) {
  const intervals = SCALES[scaleName] || SCALES['Major (Ionian)'];
  const rootIdx = typeof root === 'number' ? root : NOTE_NAMES.indexOf(root);
  if (rootIdx === -1) return [];
  const notes = [];
  for (let oct = octaveLow; oct <= octaveHigh; oct++) {
    for (let i = 0; i < intervals.length; i++) {
      const midi = (oct + 1) * 12 + rootIdx + intervals[i];
      if (midi >= 0 && midi <= 127) {
        notes.push(midi);
      }
    }
  }
  return notes;
}

// Quantize a MIDI note to the nearest note in the given scale/root
export function quantizeToScale(midi, rootIdx, scaleName) {
  const intervals = SCALES[scaleName] || SCALES['Major (Ionian)'];
  // Build a set of all valid pitch classes
  const validPCs = new Set();
  for (let i = 0; i < intervals.length; i++) {
    validPCs.add((rootIdx + intervals[i]) % 12);
  }
  // Already in scale?
  if (validPCs.has(midi % 12)) return midi;
  // Search outward for nearest scale tone
  for (let offset = 1; offset <= 6; offset++) {
    const up = midi + offset;
    if (up <= 127 && validPCs.has(up % 12)) return up;
    const down = midi - offset;
    if (down >= 0 && validPCs.has(down % 12)) return down;
  }
  return midi;
}

// Get all scale notes within the same octave as the given MIDI note
export function getScaleNotesInOctave(midi, rootIdx, scaleName) {
  const intervals = SCALES[scaleName] || SCALES['Major (Ionian)'];
  const octave = Math.floor(midi / 12); // MIDI octave (C4 = octave 5 in MIDI terms)
  const notes = [];
  for (let i = 0; i < intervals.length; i++) {
    const note = octave * 12 + ((rootIdx + intervals[i]) % 12);
    if (note >= 0 && note <= 127) {
      notes.push(note);
    }
  }
  // Also check notes from one semitone below the octave boundary that belong
  // to the scale degree wrapping from the octave below
  return notes;
}

export const SCALE_NAMES = Object.keys(SCALES);
