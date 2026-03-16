// Arpeggiator engine — PURE, no DOM, no browser APIs
// Cycles through zones based on play mode, timing, and ratchet settings

import { getScaleNotesInOctave } from './scales.js';

// Note divisions as fractions of a whole note (4 beats)
const DIVISIONS = {
  '1/64':  1/16,
  '1/32':  1/8,
  '1/32T': 1/12,
  '1/16':  1/4,
  '1/16T': 1/6,
  '1/8':   1/2,
  '1/8T':  1/3,
  '1/4':   1,
  '1/4T':  2/3,
  '1/2':   2,
  '1/2T':  4/3,
  '1/1':   4,
};

export const DIVISION_NAMES = Object.keys(DIVISIONS);

export const PLAY_MODES = [
  'first-placed',
  'up',
  'down',
  'random',
  'left-right',
  'right-left',
  'vertical-up',
  'vertical-down',
];

export const ARP_PATHS = [
  'sequential',
  'palindrome',
  'interleaved',
  'zigzag',
  'cross',
  'additive',
  'random-loop',
];

export const ARP_PATH_LABELS = [
  'Sequential',
  'Palindrome',
  'Interleaved',
  'Zigzag',
  'Cross',
  'Additive',
  'Random Loop',
];

/**
 * Calculate arp interval in milliseconds.
 * @param {string} division — key into DIVISIONS
 * @param {number} bpm — beats per minute
 * @returns {number} interval in ms
 */
export function divisionToMs(division, bpm) {
  const beats = DIVISIONS[division] || 1;
  const msPerBeat = 60000 / bpm;
  return beats * msPerBeat;
}

/**
 * Build a sorted sequence of zone indices based on play mode.
 * For 'random' mode, returns all indices (order doesn't matter).
 * @param {Array} zones — persistent.zones array
 * @param {string} playMode — one of PLAY_MODES
 * @returns {number[]} sorted zone indices
 */
export function buildSequence(zones, playMode) {
  if (zones.length === 0) return [];

  const indices = [];
  for (let i = 0; i < zones.length; i++) indices.push(i);

  switch (playMode) {
    case 'first-placed':
      // Sort by placementOrder (lower = earlier placed)
      indices.sort((a, b) => (zones[a].placementOrder || 0) - (zones[b].placementOrder || 0));
      break;

    case 'up':
      // Lowest MIDI note first
      indices.sort((a, b) => zones[a].midi - zones[b].midi);
      break;

    case 'down':
      // Highest MIDI note first
      indices.sort((a, b) => zones[b].midi - zones[a].midi);
      break;

    case 'left-right':
      // Leftmost (smallest cx) first
      indices.sort((a, b) => zones[a].cx - zones[b].cx);
      break;

    case 'right-left':
      // Rightmost (largest cx) first
      indices.sort((a, b) => zones[b].cx - zones[a].cx);
      break;

    case 'vertical-up':
      // Bottom-most (largest cy, since y increases downward) first
      indices.sort((a, b) => zones[b].cy - zones[a].cy);
      break;

    case 'vertical-down':
      // Top-most (smallest cy) first
      indices.sort((a, b) => zones[a].cy - zones[b].cy);
      break;

    case 'random':
      // No sort needed — random selection happens in getNextNote
      break;
  }

  return indices;
}

/**
 * Apply a path transformation to the base sequence.
 * @param {number[]} baseSeq — sorted zone indices
 * @param {string} path — one of ARP_PATHS
 * @param {number} loopLen — loop length for random-loop path
 * @param {function} prng — PRNG function
 * @returns {number[]} transformed sequence
 */
export function applyPath(baseSeq, path, loopLen, prng) {
  if (baseSeq.length === 0) return baseSeq;
  const n = baseSeq.length;

  switch (path) {
    case 'sequential':
      return baseSeq;

    case 'palindrome': {
      const result = [...baseSeq];
      for (let i = n - 1; i >= 0; i--)
        result.push(baseSeq[i]);
      return result;
    }

    case 'interleaved': {
      const half = Math.ceil(n / 2);
      const result = [];
      for (let i = 0; i < half; i++) {
        result.push(baseSeq[i]);
        const j = i + half;
        if (j < n) result.push(baseSeq[j]);
      }
      return result;
    }

    case 'zigzag': {
      const result = [];
      for (let g = 0; g < n; g += 4) {
        const end = Math.min(g + 4, n);
        const groupSize = end - g;
        if (groupSize >= 3) {
          result.push(baseSeq[g]);
          result.push(baseSeq[g + 2]);
          result.push(baseSeq[g + 1]);
          if (groupSize >= 4) result.push(baseSeq[g + 3]);
        } else {
          for (let i = g; i < end; i++) result.push(baseSeq[i]);
        }
      }
      return result;
    }

    case 'cross': {
      const half = Math.ceil(n / 2);
      const first = baseSeq.slice(0, half);
      const second = baseSeq.slice(half);
      const result = [];
      let fi = 0, si = 0;
      let useFirst = true;
      while (fi < first.length || si < second.length) {
        if (useFirst && fi < first.length)
          result.push(first[fi++]);
        else if (!useFirst && si < second.length)
          result.push(second[si++]);
        else if (fi < first.length)
          result.push(first[fi++]);
        else
          result.push(second[si++]);
        useFirst = !useFirst;
      }
      return result;
    }

    case 'additive': {
      const result = [];
      for (let len = 1; len <= n; len++)
        for (let i = 0; i < len; i++)
          result.push(baseSeq[i]);
      return result;
    }

    case 'random-loop': {
      const effectiveLen = Math.min(Math.max(loopLen, 2), n);
      const pool = [...baseSeq];
      const result = [];
      for (let i = 0; i < effectiveLen; i++) {
        const remaining = pool.length;
        const pick = Math.floor(prng() * remaining) % remaining;
        result.push(pool[pick]);
        pool.splice(pick, 1);
      }
      return result;
    }

    default:
      return baseSeq;
  }
}

/**
 * Create initial arp runtime state.
 */
export function createArpState() {
  return {
    sequencePos: 0,        // current position in sorted sequence
    direction: 1,          // 1 = forward, -1 = backward (for pendulum)
    ratchetCount: 0,       // how many ratchets remaining for current note
    lastNoteTimeMs: 0,     // timestamp of last note trigger
    sequence: [],          // current sorted zone indices
    lastRandomIndex: -1,   // prevent same-note-twice in random mode
    lastZoneCount: 0,      // track zone additions/removals
    lastZoneHash: '',      // track zone position/note changes
    lastPlayMode: '',      // track play mode changes
    lastArpPath: '',       // track path changes
    needsRebuild: true,    // flag to rebuild sequence
  };
}

/**
 * Compute a lightweight hash of zone positions and notes for change detection.
 */
function zoneHash(zones) {
  let h = '';
  for (let i = 0; i < zones.length; i++) {
    const z = zones[i];
    h += z.cx.toFixed(3) + z.cy.toFixed(3) + z.midi + ',';
  }
  return h;
}

/**
 * Check if zones have changed and rebuild sequence if needed.
 */
function maybeRebuildSequence(zones, playMode, arpPath, arpPathLoopLen, arpState, prng) {
  const count = zones.length;
  const hash = zoneHash(zones);

  if (arpState.needsRebuild || count !== arpState.lastZoneCount || hash !== arpState.lastZoneHash
      || playMode !== arpState.lastPlayMode || arpPath !== arpState.lastArpPath) {
    const base = buildSequence(zones, playMode);
    arpState.sequence = applyPath(base, arpPath, arpPathLoopLen, prng);
    arpState.lastZoneCount = count;
    arpState.lastZoneHash = hash;
    arpState.lastPlayMode = playMode;
    arpState.lastArpPath = arpPath;
    arpState.needsRebuild = false;

    // Clamp position to valid range
    if (arpState.sequence.length > 0) {
      arpState.sequencePos = arpState.sequencePos % arpState.sequence.length;
    } else {
      arpState.sequencePos = 0;
    }
  }
}

/**
 * Advance to the next note in the sequence.
 * Handles normal cycling and pendulum mode.
 * For random mode, picks a random note (never same twice unless ratcheting).
 */
function advancePosition(arpState, playMode, pendulum, prng) {
  const len = arpState.sequence.length;
  if (len <= 1) return;

  if (playMode === 'random') {
    // Pick random index, never same as last (unless only 1 zone)
    let next;
    do {
      next = Math.floor(prng() * len);
    } while (next === arpState.sequencePos && len > 1);
    arpState.sequencePos = next;
    return;
  }

  if (pendulum) {
    const nextPos = arpState.sequencePos + arpState.direction;
    if (nextPos >= len) {
      arpState.direction = -1;
      arpState.sequencePos = len - 2;
      if (arpState.sequencePos < 0) arpState.sequencePos = 0;
    } else if (nextPos < 0) {
      arpState.direction = 1;
      arpState.sequencePos = 1;
      if (arpState.sequencePos >= len) arpState.sequencePos = 0;
    } else {
      arpState.sequencePos = nextPos;
    }
  } else {
    arpState.sequencePos = (arpState.sequencePos + 1) % len;
  }
}

/**
 * Main arp tick — called from animation loop.
 * Returns an array of noteOn events (same format as physics.js events).
 *
 * @param {object} persistent — persistent state (includes arp settings + zones)
 * @param {object} transient — transient state (includes arpState)
 * @param {number} currentTimeMs — current time from performance.now()
 * @returns {Array} events
 */
export function tickArp(persistent, transient, currentTimeMs) {
  const events = [];

  if (!persistent.arpEnabled || persistent.zones.length === 0) {
    return events;
  }

  const arpState = transient.arpState;

  // Rebuild sequence if zones changed
  maybeRebuildSequence(persistent.zones, persistent.arpPlayMode,
                       persistent.arpPath || 'sequential',
                       persistent.arpPathLoopLen || 4,
                       arpState, transient.prng);

  if (arpState.sequence.length === 0) return events;

  // Calculate interval
  const intervalMs = persistent.arpSync
    ? divisionToMs(persistent.arpDivision, persistent.bpm)
    : persistent.arpRateMs;

  // Ratchet: subdivide the interval
  const ratchetTotal = persistent.arpRatchet; // 0 = no ratchet (1 hit per step)
  const hitsPerStep = ratchetTotal + 1;
  const hitInterval = intervalMs / hitsPerStep;

  // Check if it's time for the next hit
  const elapsed = currentTimeMs - arpState.lastNoteTimeMs;
  if (elapsed < hitInterval) return events;

  // Update timestamp
  arpState.lastNoteTimeMs = currentTimeMs;

  // Get current zone
  const zoneIndex = arpState.sequence[arpState.sequencePos];
  const zone = persistent.zones[zoneIndex];
  if (!zone) return events;

  // Generate note event with optional randomization
  let outMidi = zone.midi;
  let outVelocity = 0.8;

  if (persistent.arpUseRandomization) {
    const prng = transient.prng;

    // Random pitch
    if (persistent.randomPitch > 0 && prng() * 100 < persistent.randomPitch) {
      const scaleNotes = getScaleNotesInOctave(outMidi, persistent.scaleRoot, persistent.scaleName);
      if (scaleNotes.length > 0) {
        outMidi = scaleNotes[Math.floor(prng() * scaleNotes.length)];
      }
    }

    // Random octave
    if (persistent.randomOctaveChance > 0 && prng() * 100 < persistent.randomOctaveChance) {
      const maxShift = persistent.randomOctaveAmount;
      let shift = Math.floor(prng() * maxShift * 2 + 1) - maxShift;
      if (shift === 0) shift = prng() < 0.5 ? -1 : 1;
      const shifted = outMidi + shift * 12;
      if (shifted >= 12 && shifted <= 120) outMidi = shifted;
    }

    // Random velocity
    if (persistent.randomVelocity > 0) {
      const velRange = persistent.randomVelocity / 100;
      outVelocity = outVelocity * (1 - velRange) + prng() * outVelocity * velRange * 2;
    }
  }

  // Apply velocity floor
  const floor = (persistent.velocityFloor || 0) / 100;
  outVelocity = Math.max(floor, Math.min(1.0, outVelocity));

  events.push({
    type: 'noteOn',
    midi: outMidi,
    velocity: outVelocity,
    zoneIndex: zoneIndex,
  });

  // Flash the zone
  if (transient.zoneFlash) {
    transient.zoneFlash[zoneIndex] = 1.0;
  }

  // Handle ratchet vs advance
  if (ratchetTotal > 0) {
    arpState.ratchetCount++;
    if (arpState.ratchetCount >= ratchetTotal) {
      // Done ratcheting, advance to next note
      arpState.ratchetCount = 0;
      advancePosition(arpState, persistent.arpPlayMode, persistent.arpPendulum, transient.prng);
    }
  } else {
    // No ratchet, advance every hit
    advancePosition(arpState, persistent.arpPlayMode, persistent.arpPendulum, transient.prng);
  }

  return events;
}
