// State model definition, defaults, serialization
// PURE — no DOM, no browser APIs

import { nameToMidi } from './scales.js';
import { createPRNG } from './prng.js';
import { createArpState } from './arp.js';

export const ZONE_COLORS = [
  '#E24B4A','#D85A30','#EF9F27','#639922','#1D9E75','#378ADD','#534AB7',
  '#D4537E','#5DCAA5','#85B7EB','#AFA9EC','#F0997B','#ED93B1','#97C459',
  '#FAC775','#888780'
];

export function createDefaultPersistentState() {
  const defaultZones = [
    { cx: 0.15, cy: 0.30, hw: 0.06, hh: 0.04, midi: nameToMidi('C', 4), colorIndex: 0,  placementOrder: 0 },
    { cx: 0.35, cy: 0.20, hw: 0.03, hh: 0.07, midi: nameToMidi('E', 4), colorIndex: 1,  placementOrder: 1 },
    { cx: 0.55, cy: 0.35, hw: 0.05, hh: 0.03, midi: nameToMidi('G', 4), colorIndex: 2,  placementOrder: 2 },
    { cx: 0.75, cy: 0.25, hw: 0.04, hh: 0.05, midi: nameToMidi('C', 5), colorIndex: 3,  placementOrder: 3 },
    { cx: 0.25, cy: 0.55, hw: 0.07, hh: 0.03, midi: nameToMidi('D', 4), colorIndex: 4,  placementOrder: 4 },
    { cx: 0.50, cy: 0.60, hw: 0.03, hh: 0.08, midi: nameToMidi('F', 4), colorIndex: 5,  placementOrder: 5 },
    { cx: 0.70, cy: 0.55, hw: 0.05, hh: 0.05, midi: nameToMidi('A', 4), colorIndex: 6,  placementOrder: 6 },
    { cx: 0.15, cy: 0.75, hw: 0.04, hh: 0.06, midi: nameToMidi('G', 3), colorIndex: 7,  placementOrder: 7 },
    { cx: 0.40, cy: 0.80, hw: 0.06, hh: 0.03, midi: nameToMidi('B', 3), colorIndex: 8,  placementOrder: 8 },
    { cx: 0.65, cy: 0.78, hw: 0.03, hh: 0.05, midi: nameToMidi('B', 4), colorIndex: 9,  placementOrder: 9 },
    { cx: 0.85, cy: 0.50, hw: 0.04, hh: 0.09, midi: nameToMidi('E', 5), colorIndex: 10, placementOrder: 10 },
    { cx: 0.90, cy: 0.15, hw: 0.03, hh: 0.04, midi: nameToMidi('G', 5), colorIndex: 11, placementOrder: 11 },
    { cx: 0.10, cy: 0.12, hw: 0.05, hh: 0.03, midi: nameToMidi('C', 3), colorIndex: 12, placementOrder: 12 },
    { cx: 0.45, cy: 0.45, hw: 0.04, hh: 0.04, midi: nameToMidi('G', 4), colorIndex: 13, placementOrder: 13 },
  ];

  return {
    gravity: 20,
    bounce: 90,
    friction: 4,
    speed: 40,
    ballCount: 2,
    ballSize: 7,
    minEnergy: 25,
    solidZones: true,
    ballCollide: false,
    gateTime: 250,
    scaleRoot: 0,              // 0-11 index into NOTE_NAMES
    scaleName: 'Major (Ionian)',

    // Momentum — extra velocity multiplier on zone bounce (0=none, 100=2x)
    momentum: 0,

    // Note randomization
    randomPitch: 0,            // 0-100% chance of random scale note in same octave
    randomOctaveChance: 0,     // 0-100% probability of octave shift
    randomOctaveAmount: 1,     // 1-3 max octaves deviation
    randomVelocity: 0,         // 0-100% random velocity variation

    // Ball jitter — random velocity perturbation per tick
    jitter: 0,                 // 0-100

    // Fan — directional oscillating force field
    fanAmount: 0,              // 0-100
    fanSpeed: 30,              // 0-100
    fanDirection: 'north',     // 'north','south','east','west','random'

    // Arpeggiator
    arpEnabled: false,
    arpSync: true,             // true = sync to BPM divisions, false = free ms rate
    arpDivision: '1/8',       // sync mode division
    arpRateMs: 200,            // unsync mode rate in ms (10-4000)
    arpPlayMode: 'up',        // first-placed, up, down, random, left-right, right-left, vertical-up, vertical-down
    arpPendulum: false,        // true = bounce back and forth, false = loop
    arpRatchet: 0,             // 0-16 extra repeats per step
    bpm: 120,                  // tempo (web-only; VST gets this from host)

    zones: defaultZones,
  };
}

export function createTransientState(persistent) {
  return {
    running: true,
    balls: createBalls(persistent),
    zoneFlash: new Array(persistent.zones.length).fill(0),
    zoneLockout: new Array(persistent.zones.length).fill(0),
    selectedZone: null,
    audioStarted: false,
    defaultNote: 0,
    defaultOctave: 4,
    defaultWidth: 40,
    defaultHeight: 40,
    prng: createPRNG(42),      // seeded PRNG for deterministic randomness
    tickCount: 0,              // global tick counter for fan oscillation
    arpState: createArpState(),
    nextPlacementOrder: persistent.zones.length, // counter for new zone placement order
  };
}

export function createBalls(persistent) {
  const count = persistent.ballCount;
  const speedFactor = persistent.speed / 100;
  const balls = [];
  for (let i = 0; i < count; i++) {
    const angle = (Math.PI * 2 * i) / count + Math.PI * 0.25;
    const baseSpeed = 0.003 + speedFactor * 0.007;
    balls.push({
      x: 0.5,
      y: 0.35,
      vx: Math.cos(angle) * baseSpeed,
      vy: Math.sin(angle) * baseSpeed,
      trail: [],
    });
  }
  return balls;
}

export function serializeState(persistent) {
  return JSON.stringify(persistent);
}

export function deserializeState(json) {
  return JSON.parse(json);
}

export function nextColorIndex(zones) {
  if (zones.length === 0) return 0;
  const maxIndex = zones.reduce((m, z) => Math.max(m, z.colorIndex), -1);
  return (maxIndex + 1) % ZONE_COLORS.length;
}
