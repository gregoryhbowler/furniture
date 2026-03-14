// Arpeggiator engine — PURE, no DOM, no browser APIs
// Cycles through zones based on play mode, timing, and ratchet settings

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
function maybeRebuildSequence(zones, playMode, arpState) {
  const count = zones.length;
  const hash = zoneHash(zones);

  if (arpState.needsRebuild || count !== arpState.lastZoneCount || hash !== arpState.lastZoneHash) {
    arpState.sequence = buildSequence(zones, playMode);
    arpState.lastZoneCount = count;
    arpState.lastZoneHash = hash;
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
  maybeRebuildSequence(persistent.zones, persistent.arpPlayMode, arpState);

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

  // Generate note event
  events.push({
    type: 'noteOn',
    midi: zone.midi,
    velocity: 0.8,
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
