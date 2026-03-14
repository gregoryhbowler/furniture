// Web MIDI API output adapter
// Sends MIDI note on/off to a user-selected output port

let midiAccess = null;
let selectedOutput = null;
let midiEnabled = false;

// Track active notes so we can send note-off after gate time
const activeNotes = []; // { midi, port, timer }

/**
 * Request MIDI access and populate the device list.
 * Returns a promise that resolves with available output ports.
 */
export async function initMidi() {
  if (!navigator.requestMIDIAccess) {
    console.warn('Web MIDI API not supported in this browser');
    return [];
  }

  try {
    midiAccess = await navigator.requestMIDIAccess({ sysex: false });
    midiAccess.onstatechange = () => {
      // Refresh device list when devices connect/disconnect
      if (typeof onDeviceChange === 'function') onDeviceChange();
    };
    return getOutputs();
  } catch (err) {
    console.warn('MIDI access denied:', err);
    return [];
  }
}

/**
 * Get list of available MIDI output ports.
 * @returns {{ id: string, name: string }[]}
 */
export function getOutputs() {
  if (!midiAccess) return [];
  const outputs = [];
  for (const [id, port] of midiAccess.outputs) {
    outputs.push({ id, name: port.name || id });
  }
  return outputs;
}

/**
 * Select an output port by ID.
 */
export function selectOutput(portId) {
  if (!midiAccess) return;
  selectedOutput = midiAccess.outputs.get(portId) || null;
}

/**
 * Enable/disable MIDI output.
 */
export function setEnabled(enabled) {
  midiEnabled = enabled;
  if (!enabled) {
    allNotesOff();
  }
}

export function isEnabled() {
  return midiEnabled;
}

/**
 * Send a raw MIDI message.
 */
function send(bytes) {
  if (selectedOutput && midiEnabled) {
    selectedOutput.send(bytes);
  }
}

/**
 * Send note-on. Velocity is 0-1 float, converted to 0-127.
 */
function noteOn(midi, velocity, channel) {
  const ch = (channel || 0) & 0x0F;
  const vel = Math.max(1, Math.min(127, Math.round(velocity * 127)));
  send([0x90 | ch, midi & 0x7F, vel]);
}

/**
 * Send note-off.
 */
function noteOff(midi, channel) {
  const ch = (channel || 0) & 0x0F;
  send([0x80 | ch, midi & 0x7F, 0]);
}

/**
 * Kill all active notes immediately.
 */
export function allNotesOff() {
  for (let i = 0; i < activeNotes.length; i++) {
    clearTimeout(activeNotes[i].timer);
    noteOff(activeNotes[i].midi);
  }
  activeNotes.length = 0;
}

/**
 * Handle an event from the scheduler — same interface as web-audio.
 * @param {{ type: string, midi: number, velocity: number }} event
 * @param {number} gateTime — gate time in ms
 */
export function handleEvent(event, gateTime) {
  if (!midiEnabled || !selectedOutput) return;

  if (event.type === 'noteOn') {
    noteOn(event.midi, event.velocity);

    // Schedule note-off after gate time
    const timer = setTimeout(() => {
      noteOff(event.midi);
      const idx = activeNotes.findIndex(n => n.timer === timer);
      if (idx >= 0) activeNotes.splice(idx, 1);
    }, gateTime);

    activeNotes.push({ midi: event.midi, timer });
  }
}

// Optional callback for device hot-plug
let onDeviceChange = null;
export function setDeviceChangeCallback(cb) {
  onDeviceChange = cb;
}
