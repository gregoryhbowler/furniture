// Mouse/touch handlers — web-only
import { nextColorIndex } from '../engine/state.js';
import { nameToMidi, NOTE_NAMES } from '../engine/scales.js';

const HANDLE_SIZE_PX = 7; // half-size of resize handle hit area in pixels

export function setupInteraction(canvas, persistent, transient, callbacks) {
  // drag modes: 'move', 'resize-l', 'resize-r', 'resize-t', 'resize-b',
  //             'resize-tl', 'resize-tr', 'resize-bl', 'resize-br'
  let dragging = null;

  function canvasCoords(e) {
    const rect = canvas.getBoundingClientRect();
    const clientX = e.touches ? e.touches[0].clientX : e.clientX;
    const clientY = e.touches ? e.touches[0].clientY : e.clientY;
    return {
      x: (clientX - rect.left) / rect.width,
      y: (clientY - rect.top) / rect.height,
      px: clientX - rect.left,
      py: clientY - rect.top,
    };
  }

  function zoneEdgesPx(zone) {
    const rect = canvas.getBoundingClientRect();
    const w = rect.width;
    const h = rect.height;
    return {
      left:   (zone.cx - zone.hw) * w,
      right:  (zone.cx + zone.hw) * w,
      top:    (zone.cy - zone.hh) * h,
      bottom: (zone.cy + zone.hh) * h,
    };
  }

  // Returns which handle/edge the cursor is on, or null
  function hitTestHandle(px, py, zone) {
    const e = zoneEdgesPx(zone);
    const hs = HANDLE_SIZE_PX;

    const nearL = Math.abs(px - e.left)  <= hs;
    const nearR = Math.abs(px - e.right) <= hs;
    const nearT = Math.abs(py - e.top)   <= hs;
    const nearB = Math.abs(py - e.bottom) <= hs;
    const inX = px >= e.left - hs && px <= e.right + hs;
    const inY = py >= e.top - hs && py <= e.bottom + hs;

    // Corners first
    if (nearL && nearT) return 'resize-tl';
    if (nearR && nearT) return 'resize-tr';
    if (nearL && nearB) return 'resize-bl';
    if (nearR && nearB) return 'resize-br';
    // Edges
    if (nearL && inY) return 'resize-l';
    if (nearR && inY) return 'resize-r';
    if (nearT && inX) return 'resize-t';
    if (nearB && inX) return 'resize-b';
    return null;
  }

  function hitTestZone(nx, ny) {
    const zones = persistent.zones;
    for (let i = zones.length - 1; i >= 0; i--) {
      const z = zones[i];
      if (nx >= z.cx - z.hw && nx <= z.cx + z.hw &&
          ny >= z.cy - z.hh && ny <= z.cy + z.hh) {
        return i;
      }
    }
    return -1;
  }

  function cursorForHandle(handle) {
    switch (handle) {
      case 'resize-l':  case 'resize-r':  return 'ew-resize';
      case 'resize-t':  case 'resize-b':  return 'ns-resize';
      case 'resize-tl': case 'resize-br': return 'nwse-resize';
      case 'resize-tr': case 'resize-bl': return 'nesw-resize';
      default: return 'crosshair';
    }
  }

  function onPointerDown(e) {
    if (e.button === 2) return; // let contextmenu handle right-click

    // Start audio on first interaction
    if (!transient.audioStarted) {
      callbacks.onStartAudio();
      transient.audioStarted = true;
    }

    const { x, y, px, py } = canvasCoords(e);

    // Check resize handles on selected zone first
    if (transient.selectedZone !== null) {
      const selZone = persistent.zones[transient.selectedZone];
      if (selZone) {
        const handle = hitTestHandle(px, py, selZone);
        if (handle) {
          dragging = {
            mode: handle,
            zoneIndex: transient.selectedZone,
            startX: x, startY: y,
            origCx: selZone.cx, origCy: selZone.cy,
            origHw: selZone.hw, origHh: selZone.hh,
          };
          canvas.style.cursor = cursorForHandle(handle);
          return;
        }
      }
    }

    // Check all zones for edge handles (not just selected)
    for (let i = persistent.zones.length - 1; i >= 0; i--) {
      const zone = persistent.zones[i];
      const handle = hitTestHandle(px, py, zone);
      if (handle) {
        transient.selectedZone = i;
        callbacks.onSelectZone(i);
        dragging = {
          mode: handle,
          zoneIndex: i,
          startX: x, startY: y,
          origCx: zone.cx, origCy: zone.cy,
          origHw: zone.hw, origHh: zone.hh,
        };
        canvas.style.cursor = cursorForHandle(handle);
        return;
      }
    }

    const hitIdx = hitTestZone(x, y);

    if (hitIdx >= 0) {
      const zone = persistent.zones[hitIdx];
      dragging = {
        mode: 'move',
        zoneIndex: hitIdx,
        offsetX: x - zone.cx,
        offsetY: y - zone.cy,
        startX: x,
        startY: y,
        moved: false,
      };
      transient.selectedZone = hitIdx;
      callbacks.onSelectZone(hitIdx);
      canvas.style.cursor = 'grab';
    } else {
      // Deselect and create new zone
      transient.selectedZone = null;
      callbacks.onDeselectZone();

      const midi = nameToMidi(
        NOTE_NAMES[transient.defaultNote],
        transient.defaultOctave
      );
      const hw = (transient.defaultWidth / 100) * 0.08;
      const hh = (transient.defaultHeight / 100) * 0.08;
      const colorIndex = nextColorIndex(persistent.zones);

      const newZone = {
        cx: Math.max(hw, Math.min(1 - hw, x)),
        cy: Math.max(hh, Math.min(1 - hh, y)),
        hw, hh, midi, colorIndex,
        placementOrder: transient.nextPlacementOrder++,
      };
      persistent.zones.push(newZone);
      transient.zoneFlash.push(0);
      transient.zoneLockout.push(0);

      const idx = persistent.zones.length - 1;
      transient.selectedZone = idx;
      callbacks.onSelectZone(idx);
    }
  }

  function onPointerMove(e) {
    const { x, y, px, py } = canvasCoords(e);

    // Update cursor on hover
    if (!dragging) {
      let cursor = 'crosshair';

      // Check selected zone handles first
      if (transient.selectedZone !== null) {
        const selZone = persistent.zones[transient.selectedZone];
        if (selZone) {
          const handle = hitTestHandle(px, py, selZone);
          if (handle) {
            cursor = cursorForHandle(handle);
            canvas.style.cursor = cursor;
            return;
          }
        }
      }

      // Check other zone edges
      for (let i = persistent.zones.length - 1; i >= 0; i--) {
        const handle = hitTestHandle(px, py, persistent.zones[i]);
        if (handle) {
          cursor = cursorForHandle(handle);
          break;
        }
      }

      if (cursor === 'crosshair') {
        const hitIdx = hitTestZone(x, y);
        if (hitIdx >= 0) cursor = 'grab';
      }
      canvas.style.cursor = cursor;
      return;
    }

    e.preventDefault();
    const zone = persistent.zones[dragging.zoneIndex];
    if (!zone) return;

    if (dragging.mode === 'move') {
      const dx = x - dragging.startX;
      const dy = y - dragging.startY;
      if (Math.abs(dx) > 0.005 || Math.abs(dy) > 0.005) {
        dragging.moved = true;
      }
      if (dragging.moved) {
        zone.cx = Math.max(zone.hw, Math.min(1 - zone.hw, x - dragging.offsetX));
        zone.cy = Math.max(zone.hh, Math.min(1 - zone.hh, y - dragging.offsetY));
        canvas.style.cursor = 'grabbing';
      }
    } else {
      // Resize
      const dx = x - dragging.startX;
      const dy = y - dragging.startY;
      const mode = dragging.mode;
      const MIN_HW = 0.01;
      const MIN_HH = 0.01;

      let newCx = dragging.origCx;
      let newCy = dragging.origCy;
      let newHw = dragging.origHw;
      let newHh = dragging.origHh;

      if (mode.includes('l')) {
        // Dragging left edge right means shrinking, left means growing
        newHw = Math.max(MIN_HW, dragging.origHw - dx / 2);
        newCx = dragging.origCx + (dragging.origHw - newHw);
      }
      if (mode.includes('r')) {
        newHw = Math.max(MIN_HW, dragging.origHw + dx / 2);
        newCx = dragging.origCx + (newHw - dragging.origHw);
      }
      if (mode.includes('t')) {
        newHh = Math.max(MIN_HH, dragging.origHh - dy / 2);
        newCy = dragging.origCy + (dragging.origHh - newHh);
      }
      if (mode.includes('b')) {
        newHh = Math.max(MIN_HH, dragging.origHh + dy / 2);
        newCy = dragging.origCy + (newHh - dragging.origHh);
      }

      // Clamp to canvas bounds
      newCx = Math.max(newHw, Math.min(1 - newHw, newCx));
      newCy = Math.max(newHh, Math.min(1 - newHh, newCy));
      newHw = Math.min(newHw, 0.25);
      newHh = Math.min(newHh, 0.25);

      zone.cx = newCx;
      zone.cy = newCy;
      zone.hw = newHw;
      zone.hh = newHh;

      // Update zone editor sliders if open
      callbacks.onZoneResized(dragging.zoneIndex);
    }
  }

  function onPointerUp() {
    if (dragging) {
      canvas.style.cursor = 'crosshair';
    }
    dragging = null;
  }

  function onContextMenu(e) {
    e.preventDefault();
    const { x, y } = canvasCoords(e);
    const hitIdx = hitTestZone(x, y);
    if (hitIdx >= 0) {
      callbacks.onDeleteZone(hitIdx);
    }
  }

  function onWheel(e) {
    const { x, y } = canvasCoords(e);
    const hitIdx = hitTestZone(x, y);
    if (hitIdx >= 0) {
      e.preventDefault();
      const zone = persistent.zones[hitIdx];
      const factor = e.deltaY < 0 ? 1.08 : 0.92;
      zone.hw = Math.max(0.01, Math.min(0.25, zone.hw * factor));
      zone.hh = Math.max(0.01, Math.min(0.25, zone.hh * factor));
      if (transient.selectedZone === hitIdx) {
        callbacks.onZoneResized(hitIdx);
      }
    }
  }

  // Mouse events
  canvas.addEventListener('mousedown', onPointerDown);
  window.addEventListener('mousemove', onPointerMove);
  window.addEventListener('mouseup', onPointerUp);
  canvas.addEventListener('contextmenu', onContextMenu);
  canvas.addEventListener('wheel', onWheel, { passive: false });

  // Touch events
  canvas.addEventListener('touchstart', (e) => {
    e.preventDefault();
    onPointerDown(e);
  }, { passive: false });
  canvas.addEventListener('touchmove', (e) => {
    onPointerMove(e);
  }, { passive: false });
  canvas.addEventListener('touchend', onPointerUp);
}
