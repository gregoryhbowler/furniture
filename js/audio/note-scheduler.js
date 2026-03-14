// Consumes engine events, dispatches to all active output adapters
// Supports multiple adapters (e.g. web audio + MIDI output simultaneously)

export function createScheduler(...adapters) {
  return {
    processEvents(events, gateTime) {
      for (let i = 0; i < events.length; i++) {
        for (let a = 0; a < adapters.length; a++) {
          adapters[a].handleEvent(events[i], gateTime);
        }
      }
    },
    addAdapter(adapter) {
      adapters.push(adapter);
    },
  };
}
