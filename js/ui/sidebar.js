// Sidebar controls and bindings — web-only
import { createBalls } from '../engine/state.js';
import { NOTE_NAMES, SCALE_NAMES, quantizeToScale, getScaleNotes } from '../engine/scales.js';
import { synthParams, setMasterVolume } from '../audio/web-audio.js';
import * as midiOutput from '../audio/midi-output.js';

export function setupSidebar(persistent, transient, callbacks) {
  // Physics sliders
  bindSlider('gravity', persistent, 'gravity', 0, 100);
  bindSlider('bounce', persistent, 'bounce', 50, 100);
  bindSlider('friction', persistent, 'friction', 0, 50);
  bindSlider('speed', persistent, 'speed', 5, 100, () => {
    transient.balls = createBalls(persistent);
  });
  bindSlider('ballCount', persistent, 'ballCount', 1, 16, () => {
    transient.balls = createBalls(persistent);
  });
  bindSlider('ballSize', persistent, 'ballSize', 3, 18);
  bindSlider('minEnergy', persistent, 'minEnergy', 0, 80);
  bindSlider('momentum', persistent, 'momentum', 0, 100);
  bindSlider('jitter', persistent, 'jitter', 0, 100);

  // Fan
  bindSlider('fanAmount', persistent, 'fanAmount', 0, 100);
  bindSlider('fanSpeed', persistent, 'fanSpeed', 0, 100);
  const fanDirSel = document.getElementById('select-fanDirection');
  fanDirSel.value = persistent.fanDirection;
  fanDirSel.addEventListener('change', () => {
    persistent.fanDirection = fanDirSel.value;
  });

  // Toggles
  bindToggle('solidZones', persistent, 'solidZones');
  bindToggle('ballCollide', persistent, 'ballCollide');

  // Arpeggiator
  bindToggle('arpEnabled', persistent, 'arpEnabled');
  bindSlider('bpm', persistent, 'bpm', 20, 300);
  bindToggle('arpSync', persistent, 'arpSync', () => {
    updateArpSyncVisibility(persistent);
  });
  updateArpSyncVisibility(persistent);

  const arpDivSel = document.getElementById('select-arpDivision');
  arpDivSel.value = persistent.arpDivision;
  arpDivSel.addEventListener('change', () => {
    persistent.arpDivision = arpDivSel.value;
    if (transient.arpState) transient.arpState.needsRebuild = true;
  });

  bindSlider('arpRateMs', persistent, 'arpRateMs', 10, 4000);

  const arpModeSel = document.getElementById('select-arpPlayMode');
  arpModeSel.value = persistent.arpPlayMode;
  arpModeSel.addEventListener('change', () => {
    persistent.arpPlayMode = arpModeSel.value;
    if (transient.arpState) transient.arpState.needsRebuild = true;
  });

  bindToggle('arpPendulum', persistent, 'arpPendulum');
  bindSlider('arpRatchet', persistent, 'arpRatchet', 0, 16);

  // MIDI Output
  const midiDeviceSel = document.getElementById('select-midiDevice');
  const midiToggleBtn = document.getElementById('btn-midiEnabled');

  function refreshMidiDevices() {
    midiOutput.initMidi().then(outputs => {
      // Preserve current selection if still available
      const prev = midiDeviceSel.value;
      midiDeviceSel.innerHTML = '<option value="">— none —</option>';
      for (let i = 0; i < outputs.length; i++) {
        const opt = document.createElement('option');
        opt.value = outputs[i].id;
        opt.textContent = outputs[i].name;
        midiDeviceSel.appendChild(opt);
      }
      if (prev) midiDeviceSel.value = prev;
    });
  }

  midiDeviceSel.addEventListener('change', () => {
    midiOutput.selectOutput(midiDeviceSel.value);
  });

  if (midiToggleBtn) {
    midiToggleBtn.addEventListener('click', () => {
      const nowEnabled = !midiOutput.isEnabled();
      midiOutput.setEnabled(nowEnabled);
      midiToggleBtn.classList.toggle('active', nowEnabled);
      midiToggleBtn.textContent = nowEnabled ? 'On' : 'Off';
      if (nowEnabled) refreshMidiDevices();
    });
  }

  document.getElementById('btn-midiRefresh').addEventListener('click', refreshMidiDevices);

  midiOutput.setDeviceChangeCallback(refreshMidiDevices);

  // Synth controls
  const waveformSel = document.getElementById('select-waveform');
  waveformSel.value = synthParams.waveform;
  waveformSel.addEventListener('change', () => {
    synthParams.waveform = waveformSel.value;
  });

  bindSlider('masterVolume', synthParams, 'masterVolume', 0, 100, () => {
    setMasterVolume(synthParams.masterVolume);
  });
  bindSlider('synthVoices', synthParams, 'voices', 1, 8);
  bindSlider('filterCutoff', synthParams, 'filterCutoff', 60, 12000);
  bindSliderFloat('filterRes', synthParams, 'filterRes', 5, 200, 10); // stored *10 for int slider
  bindSlider('filterEnvAmt', synthParams, 'filterEnvAmt', 0, 8000);

  // Envelope
  bindSlider('attack', synthParams, 'attack', 1, 2000);
  bindSlider('decay', synthParams, 'decay', 1, 2000);
  bindSlider('sustain', synthParams, 'sustain', 0, 100);
  bindSlider('release', synthParams, 'release', 1, 3000);

  // Performance
  bindSlider('gateTime', persistent, 'gateTime', 20, 2000);

  // Default note/octave for new zones
  bindSelect('defaultNote', transient, 'defaultNote');
  bindSelect('defaultOctave', transient, 'defaultOctave');
  bindSlider('defaultWidth', transient, 'defaultWidth', 10, 100);
  bindSlider('defaultHeight', transient, 'defaultHeight', 10, 100);

  // Scale controls
  const scaleRootSel = document.getElementById('select-scaleRoot');
  scaleRootSel.value = persistent.scaleRoot;
  scaleRootSel.addEventListener('change', () => {
    persistent.scaleRoot = Number(scaleRootSel.value);
  });

  const scaleNameSel = document.getElementById('select-scaleName');
  for (let i = 0; i < SCALE_NAMES.length; i++) {
    const opt = document.createElement('option');
    opt.value = SCALE_NAMES[i];
    opt.textContent = SCALE_NAMES[i];
    scaleNameSel.appendChild(opt);
  }
  scaleNameSel.value = persistent.scaleName;
  scaleNameSel.addEventListener('change', () => {
    persistent.scaleName = scaleNameSel.value;
  });

  // Note variation
  bindSlider('randomPitch', persistent, 'randomPitch', 0, 100);
  bindSlider('randomOctaveChance', persistent, 'randomOctaveChance', 0, 100);
  bindSlider('randomOctaveAmount', persistent, 'randomOctaveAmount', 1, 3);
  bindSlider('randomVelocity', persistent, 'randomVelocity', 0, 100);

  document.getElementById('btn-quantize').addEventListener('click', () => {
    for (let i = 0; i < persistent.zones.length; i++) {
      persistent.zones[i].midi = quantizeToScale(
        persistent.zones[i].midi, persistent.scaleRoot, persistent.scaleName
      );
    }
    callbacks.onZonesChanged();
  });

  // Randomize
  document.getElementById('btn-rand-notes').addEventListener('click', () => {
    const scaleNotes = getScaleNotes(persistent.scaleRoot, persistent.scaleName, 2, 6);
    if (scaleNotes.length === 0) return;
    for (let i = 0; i < persistent.zones.length; i++) {
      persistent.zones[i].midi = scaleNotes[Math.floor(Math.random() * scaleNotes.length)];
    }
    callbacks.onZonesChanged();
  });

  document.getElementById('btn-rand-layout').addEventListener('click', () => {
    for (let i = 0; i < persistent.zones.length; i++) {
      const z = persistent.zones[i];
      z.hw = 0.02 + Math.random() * 0.06;
      z.hh = 0.02 + Math.random() * 0.06;
      z.cx = z.hw + Math.random() * (1 - 2 * z.hw);
      z.cy = z.hh + Math.random() * (1 - 2 * z.hh);
    }
    callbacks.onZonesChanged();
  });

  // Action buttons
  document.getElementById('btn-pause').addEventListener('click', () => {
    transient.running = !transient.running;
    document.getElementById('btn-pause').textContent = transient.running ? 'Pause' : 'Run';
  });

  document.getElementById('btn-reset-balls').addEventListener('click', () => {
    transient.balls = createBalls(persistent);
  });

  document.getElementById('btn-clear-zones').addEventListener('click', () => {
    persistent.zones.length = 0;
    transient.zoneFlash.length = 0;
    transient.zoneLockout.length = 0;
    transient.selectedZone = null;
    callbacks.onClearZones();
  });
}

function bindSlider(id, obj, key, min, max, onChange) {
  const slider = document.getElementById('slider-' + id);
  const readout = document.getElementById('val-' + id);
  if (!slider) return;
  slider.min = min;
  slider.max = max;
  slider.value = obj[key];
  if (readout) readout.textContent = obj[key];
  slider.addEventListener('input', () => {
    obj[key] = Number(slider.value);
    if (readout) readout.textContent = slider.value;
    if (onChange) onChange();
  });
}

function bindSliderFloat(id, obj, key, min, max, divisor) {
  const slider = document.getElementById('slider-' + id);
  const readout = document.getElementById('val-' + id);
  if (!slider) return;
  slider.min = min;
  slider.max = max;
  slider.value = Math.round(obj[key] * divisor);
  if (readout) readout.textContent = obj[key].toFixed(1);
  slider.addEventListener('input', () => {
    obj[key] = Number(slider.value) / divisor;
    if (readout) readout.textContent = obj[key].toFixed(1);
  });
}

function bindToggle(id, obj, key, onChange) {
  const btn = document.getElementById('btn-' + id);
  if (!btn) return;
  btn.classList.toggle('active', obj[key]);
  btn.textContent = obj[key] ? 'On' : 'Off';
  btn.addEventListener('click', () => {
    obj[key] = !obj[key];
    btn.classList.toggle('active', obj[key]);
    btn.textContent = obj[key] ? 'On' : 'Off';
    if (onChange) onChange();
  });
}

function updateArpSyncVisibility(persistent) {
  const divRow = document.getElementById('row-arpDivision');
  const rateRow = document.getElementById('row-arpRateMs');
  if (divRow) divRow.style.display = persistent.arpSync ? '' : 'none';
  if (rateRow) rateRow.style.display = persistent.arpSync ? 'none' : '';
}

function bindSelect(id, obj, key) {
  const sel = document.getElementById('select-' + id);
  if (!sel) return;
  sel.value = obj[key];
  sel.addEventListener('change', () => {
    obj[key] = Number(sel.value);
  });
}

const MAX_LOG_LINES = 30;
let logLines = [];

export function logMidi(message) {
  const el = document.getElementById('midi-log');
  if (!el) return;
  logLines.push(message);
  if (logLines.length > MAX_LOG_LINES) logLines.shift();
  el.textContent = logLines.join('\n');
  el.scrollTop = el.scrollHeight;
}
