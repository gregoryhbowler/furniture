// Zone editor panel — web-only
import { NOTE_NAMES, nameToMidi, midiToName } from '../engine/scales.js';

let currentZoneIndex = null;

export function showEditor(persistent, transient, zoneIndex, callbacks) {
  currentZoneIndex = zoneIndex;
  const panel = document.getElementById('zone-editor');
  panel.classList.add('open');

  const zone = persistent.zones[zoneIndex];
  const octave = Math.floor(zone.midi / 12) - 1;

  document.getElementById('zone-name').textContent = midiToName(zone.midi);
  document.getElementById('zone-note').value = zone.midi % 12;
  document.getElementById('zone-octave').value = octave;
  document.getElementById('zone-width').value = Math.round((zone.hw / 0.08) * 100);
  document.getElementById('zone-height').value = Math.round((zone.hh / 0.08) * 100);
}

export function hideEditor(transient) {
  currentZoneIndex = null;
  const panel = document.getElementById('zone-editor');
  panel.classList.remove('open');
  transient.selectedZone = null;
}

export function setupEditor(persistent, transient, callbacks) {
  // Prevent clicks on editor from propagating to canvas
  const panel = document.getElementById('zone-editor');
  panel.addEventListener('mousedown', (e) => e.stopPropagation());
  panel.addEventListener('touchstart', (e) => e.stopPropagation());

  const noteSelect = document.getElementById('zone-note');
  const octSelect = document.getElementById('zone-octave');
  const widthSlider = document.getElementById('zone-width');
  const heightSlider = document.getElementById('zone-height');
  const deleteBtn = document.getElementById('zone-delete');
  const closeBtn = document.getElementById('zone-close');

  noteSelect.addEventListener('change', () => {
    if (currentZoneIndex === null) return;
    const zone = persistent.zones[currentZoneIndex];
    const octave = Math.floor(zone.midi / 12) - 1;
    zone.midi = nameToMidi(NOTE_NAMES[Number(noteSelect.value)], octave);
    document.getElementById('zone-name').textContent = midiToName(zone.midi);
  });

  octSelect.addEventListener('change', () => {
    if (currentZoneIndex === null) return;
    const zone = persistent.zones[currentZoneIndex];
    const noteIdx = zone.midi % 12;
    zone.midi = nameToMidi(NOTE_NAMES[noteIdx], Number(octSelect.value));
    document.getElementById('zone-name').textContent = midiToName(zone.midi);
  });

  widthSlider.addEventListener('input', () => {
    if (currentZoneIndex === null) return;
    persistent.zones[currentZoneIndex].hw = (Number(widthSlider.value) / 100) * 0.08;
  });

  heightSlider.addEventListener('input', () => {
    if (currentZoneIndex === null) return;
    persistent.zones[currentZoneIndex].hh = (Number(heightSlider.value) / 100) * 0.08;
  });

  const dupeBtn = document.getElementById('zone-duplicate');
  dupeBtn.addEventListener('click', () => {
    if (currentZoneIndex === null) return;
    callbacks.onDuplicateZone(currentZoneIndex);
  });

  deleteBtn.addEventListener('click', () => {
    if (currentZoneIndex === null) return;
    callbacks.onDeleteZone(currentZoneIndex);
  });

  closeBtn.addEventListener('click', () => {
    hideEditor(transient);
  });
}
