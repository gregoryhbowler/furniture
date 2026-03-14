// Initialization, animation loop, glue — web-only
import { createDefaultPersistentState, createTransientState, createBalls, nextColorIndex } from './engine/state.js';
import { tick } from './engine/physics.js';
import { tickArp } from './engine/arp.js';
import { midiToName } from './engine/scales.js';
import * as webAudio from './audio/web-audio.js';
import * as midiOutput from './audio/midi-output.js';
import { createScheduler } from './audio/note-scheduler.js';
import { setupCanvas, render } from './ui/renderer.js';
import { setupInteraction } from './ui/interaction.js';
import { setupSidebar, logMidi } from './ui/sidebar.js';
import { setupEditor, showEditor, hideEditor } from './ui/zone-editor.js';

// State
const persistent = createDefaultPersistentState();
const transient = createTransientState(persistent);

// Audio
const scheduler = createScheduler(webAudio, midiOutput);

// Canvas
const canvas = document.getElementById('canvas');
let ctx = setupCanvas(canvas);

// Handle resize
window.addEventListener('resize', () => {
  ctx = setupCanvas(canvas);
});

// Callbacks shared between UI modules
const callbacks = {
  onStartAudio() {
    webAudio.initAudio();
    webAudio.resumeAudio();
  },
  onSelectZone(idx) {
    transient.selectedZone = idx;
    showEditor(persistent, transient, idx, callbacks);
  },
  onDuplicateZone(idx) {
    const src = persistent.zones[idx];
    const newZone = {
      cx: Math.min(1 - src.hw, src.cx + 0.04),
      cy: Math.min(1 - src.hh, src.cy + 0.04),
      hw: src.hw,
      hh: src.hh,
      midi: src.midi,
      colorIndex: nextColorIndex(persistent.zones),
      placementOrder: transient.nextPlacementOrder++,
    };
    persistent.zones.push(newZone);
    transient.zoneFlash.push(0);
    transient.zoneLockout.push(0);
    const newIdx = persistent.zones.length - 1;
    transient.selectedZone = newIdx;
    showEditor(persistent, transient, newIdx, callbacks);
  },
  onDeleteZone(idx) {
    persistent.zones.splice(idx, 1);
    transient.zoneFlash.splice(idx, 1);
    transient.zoneLockout.splice(idx, 1);
    if (transient.selectedZone === idx) {
      hideEditor(transient);
    } else if (transient.selectedZone !== null && transient.selectedZone > idx) {
      transient.selectedZone--;
    }
  },
  onDeselectZone() {
    hideEditor(transient);
  },
  onZoneResized(idx) {
    // Update the zone editor sliders to reflect canvas drag
    if (transient.selectedZone === idx) {
      showEditor(persistent, transient, idx, callbacks);
    }
  },
  onClearZones() {
    hideEditor(transient);
  },
  onZonesChanged() {
    // Re-show editor if a zone is selected (note may have changed)
    if (transient.selectedZone !== null && transient.selectedZone < persistent.zones.length) {
      showEditor(persistent, transient, transient.selectedZone, callbacks);
    }
  },
};

// Setup UI
setupInteraction(canvas, persistent, transient, callbacks);
setupSidebar(persistent, transient, callbacks);
setupEditor(persistent, transient, callbacks);

// Animation loop
const TICKS_PER_FRAME = 3;
let lastTime = 0;

function loop(timestamp) {
  requestAnimationFrame(loop);

  if (transient.running) {
    for (let i = 0; i < TICKS_PER_FRAME; i++) {
      const events = tick(persistent, transient, 1.0);

      if (transient.audioStarted) {
        scheduler.processEvents(events, persistent.gateTime);
      }

      // Log MIDI events
      for (let e = 0; e < events.length; e++) {
        if (events[e].type === 'noteOn') {
          logMidi(`${midiToName(events[e].midi)}  vel:${events[e].velocity.toFixed(2)}`);
        }
      }
    }

    // Arp runs on real time, not physics ticks
    if (persistent.arpEnabled && transient.audioStarted) {
      const arpEvents = tickArp(persistent, transient, performance.now());
      if (arpEvents.length > 0) {
        scheduler.processEvents(arpEvents, persistent.gateTime);
        for (let e = 0; e < arpEvents.length; e++) {
          if (arpEvents[e].type === 'noteOn') {
            logMidi(`ARP ${midiToName(arpEvents[e].midi)}  vel:${arpEvents[e].velocity.toFixed(2)}`);
          }
        }
      }
    }
  }

  ctx = setupCanvas(canvas);
  render(canvas, ctx, persistent, transient);
  lastTime = timestamp;
}

requestAnimationFrame(loop);
