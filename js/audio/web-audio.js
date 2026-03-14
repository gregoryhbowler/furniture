// SH-101-style mono/poly synth — Web Audio API
// Web-only preview — will be replaced by MIDI output in VST

import { midiToFrequency } from '../engine/scales.js';

let audioCtx = null;
let masterGain = null;

// Synth parameters — mutated directly by sidebar bindings
export const synthParams = {
  masterVolume: 70,       // 0-100, master output level (0 = mute for MIDI-only use)
  waveform: 'sawtooth',   // 'sawtooth', 'square', 'triangle', 'sine'
  filterCutoff: 2400,     // Hz, 60-12000
  filterRes: 1.0,         // Q, 0.5-20
  filterEnvAmt: 3000,     // Hz, 0-8000 — how much envelope opens the filter
  attack: 5,              // ms, 1-2000
  decay: 200,             // ms, 1-2000
  sustain: 40,            // %, 0-100
  release: 300,           // ms, 1-3000
  voices: 4,              // 1-8 polyphony
};

// Voice pool for polyphony
let activeVoices = []; // { midi, osc, filter, gain, releaseTimer }

export function initAudio() {
  if (audioCtx) return audioCtx;
  audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  masterGain = audioCtx.createGain();
  masterGain.gain.setValueAtTime(synthParams.masterVolume / 100 * 0.5, audioCtx.currentTime);
  masterGain.connect(audioCtx.destination);
  return audioCtx;
}

export function setMasterVolume(vol) {
  if (masterGain && audioCtx) {
    masterGain.gain.setTargetAtTime(vol / 100 * 0.5, audioCtx.currentTime, 0.02);
  }
}

export function isAudioReady() {
  return audioCtx !== null && audioCtx.state === 'running';
}

export function resumeAudio() {
  if (audioCtx && audioCtx.state === 'suspended') {
    audioCtx.resume();
  }
}

function stealVoice() {
  // Steal oldest voice
  if (activeVoices.length === 0) return;
  const oldest = activeVoices.shift();
  cleanupVoice(oldest);
}

function cleanupVoice(voice) {
  if (voice.releaseTimer) clearTimeout(voice.releaseTimer);
  try {
    voice.gain.gain.cancelScheduledValues(audioCtx.currentTime);
    voice.gain.gain.setValueAtTime(voice.gain.gain.value, audioCtx.currentTime);
    voice.gain.gain.exponentialRampToValueAtTime(0.0001, audioCtx.currentTime + 0.01);
    voice.osc.stop(audioCtx.currentTime + 0.02);
  } catch (e) {
    // Voice already stopped
  }
}

export function playNote(midi, velocity, gateTime) {
  if (!audioCtx || audioCtx.state !== 'running') return;

  const p = synthParams;

  // Voice stealing — enforce polyphony limit
  while (activeVoices.length >= p.voices) {
    stealVoice();
  }

  const freq = midiToFrequency(midi);
  const now = audioCtx.currentTime;

  const atkSec = p.attack / 1000;
  const decSec = p.decay / 1000;
  const susLevel = p.sustain / 100;
  const relSec = p.release / 1000;
  const gateSec = gateTime / 1000;

  // Scale gain by voice count to prevent clipping
  const voiceGain = velocity * 0.18 / Math.sqrt(p.voices);

  // --- Oscillator ---
  const osc = audioCtx.createOscillator();
  osc.type = p.waveform;
  osc.frequency.setValueAtTime(freq, now);

  // --- Filter (resonant lowpass) ---
  const filter = audioCtx.createBiquadFilter();
  filter.type = 'lowpass';
  filter.Q.setValueAtTime(p.filterRes, now);

  // Filter envelope: starts at cutoff, sweeps up by envAmt during attack, decays back
  const filterBase = p.filterCutoff;
  const filterPeak = Math.min(18000, filterBase + p.filterEnvAmt * velocity);

  filter.frequency.setValueAtTime(filterBase, now);
  filter.frequency.linearRampToValueAtTime(filterPeak, now + atkSec);
  filter.frequency.exponentialRampToValueAtTime(
    Math.max(20, filterBase + (filterPeak - filterBase) * susLevel * 0.3),
    now + atkSec + decSec
  );

  // --- Amp envelope (ADSR) ---
  const gain = audioCtx.createGain();
  gain.gain.setValueAtTime(0.0001, now);

  // Attack
  gain.gain.linearRampToValueAtTime(voiceGain, now + atkSec);
  // Decay to sustain
  const susGain = Math.max(0.0001, voiceGain * susLevel);
  gain.gain.exponentialRampToValueAtTime(susGain, now + atkSec + decSec);

  // Hold at sustain until gate ends
  const noteOffTime = now + gateSec;
  gain.gain.setValueAtTime(susGain, noteOffTime);
  // Release
  gain.gain.exponentialRampToValueAtTime(0.0001, noteOffTime + relSec);

  // Filter release — close filter during release
  filter.frequency.setValueAtTime(
    Math.max(20, filterBase + (filterPeak - filterBase) * susLevel * 0.3),
    noteOffTime
  );
  filter.frequency.exponentialRampToValueAtTime(
    Math.max(20, filterBase * 0.5),
    noteOffTime + relSec
  );

  // --- Connect ---
  osc.connect(filter);
  filter.connect(gain);
  gain.connect(masterGain);

  osc.start(now);
  osc.stop(noteOffTime + relSec + 0.05);

  const voice = { midi, osc, filter, gain, releaseTimer: null };

  // Remove voice from pool after it finishes
  voice.releaseTimer = setTimeout(() => {
    const idx = activeVoices.indexOf(voice);
    if (idx >= 0) activeVoices.splice(idx, 1);
  }, (gateSec + relSec + 0.1) * 1000);

  activeVoices.push(voice);
}

export function handleEvent(event, gateTime) {
  if (event.type === 'noteOn') {
    playNote(event.midi, event.velocity, gateTime);
  }
}
