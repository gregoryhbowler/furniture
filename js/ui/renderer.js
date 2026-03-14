// Canvas rendering — web-only
import { ZONE_COLORS } from '../engine/state.js';
import { midiToName } from '../engine/scales.js';

function hexToRgb(hex) {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return { r, g, b };
}

function brighten(rgb, amount) {
  return {
    r: Math.min(255, rgb.r + amount),
    g: Math.min(255, rgb.g + amount),
    b: Math.min(255, rgb.b + amount),
  };
}

export function setupCanvas(canvas) {
  const dpr = window.devicePixelRatio || 1;
  const container = canvas.parentElement;
  const w = container.clientWidth;
  const h = container.clientHeight;
  if (w === 0 || h === 0) return canvas.getContext('2d');
  const needsResize = canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr);
  if (needsResize) {
    canvas.width = Math.round(w * dpr);
    canvas.height = Math.round(h * dpr);
  }
  const ctx = canvas.getContext('2d');
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  return ctx;
}

export function render(canvas, ctx, persistent, transient) {
  const container = canvas.parentElement;
  const w = container.clientWidth;
  const h = container.clientHeight;
  const { zones, ballSize } = persistent;
  const { balls, zoneFlash, selectedZone, audioStarted } = transient;
  const ballRadius = 0.005 + (ballSize / 18) * 0.015;

  // Background
  ctx.fillStyle = '#2d5a3a';
  ctx.fillRect(0, 0, w, h);

  // Grid
  ctx.strokeStyle = 'rgba(255,255,255,0.08)';
  ctx.lineWidth = 0.5;
  const gridSize = 40;
  for (let x = gridSize; x < w; x += gridSize) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, h);
    ctx.stroke();
  }
  for (let y = gridSize; y < h; y += gridSize) {
    ctx.beginPath();
    ctx.moveTo(0, y);
    ctx.lineTo(w, y);
    ctx.stroke();
  }

  // Zones
  for (let i = 0; i < zones.length; i++) {
    const zone = zones[i];
    const flash = zoneFlash[i] || 0;
    const color = ZONE_COLORS[zone.colorIndex % ZONE_COLORS.length];
    const rgb = hexToRgb(color);
    const bright = brighten(rgb, 30);

    const zx = (zone.cx - zone.hw) * w;
    const zy = (zone.cy - zone.hh) * h;
    const zw = zone.hw * 2 * w;
    const zh = zone.hh * 2 * h;

    // Fill
    const fillAlpha = 0.32 + flash * 0.5;
    ctx.fillStyle = `rgba(${rgb.r},${rgb.g},${rgb.b},${fillAlpha})`;
    ctx.fillRect(zx, zy, zw, zh);

    // Stroke
    if (selectedZone === i) {
      ctx.strokeStyle = 'rgba(255,255,255,0.8)';
      ctx.lineWidth = 2;
      ctx.setLineDash([6, 4]);
      ctx.strokeRect(zx, zy, zw, zh);
      ctx.setLineDash([]);

      // Resize handles — small squares at corners and edge midpoints
      const hs = 4;
      ctx.fillStyle = 'rgba(255,255,255,0.9)';
      const handles = [
        [zx, zy],                     // top-left
        [zx + zw, zy],                // top-right
        [zx, zy + zh],                // bottom-left
        [zx + zw, zy + zh],           // bottom-right
        [zx + zw / 2, zy],            // top-mid
        [zx + zw / 2, zy + zh],       // bottom-mid
        [zx, zy + zh / 2],            // left-mid
        [zx + zw, zy + zh / 2],       // right-mid
      ];
      for (let hi = 0; hi < handles.length; hi++) {
        ctx.fillRect(handles[hi][0] - hs, handles[hi][1] - hs, hs * 2, hs * 2);
      }
    } else {
      const strokeAlpha = 0.55 + flash * 0.45;
      ctx.strokeStyle = `rgba(${bright.r},${bright.g},${bright.b},${strokeAlpha})`;
      ctx.lineWidth = flash > 0.01 ? 2.5 : 1;
      ctx.strokeRect(zx, zy, zw, zh);
    }

    // Label
    const labelAlpha = 0.5 + flash * 0.5;
    const fontSize = (zw < 35 || zh < 25) ? 9 : 11;
    ctx.fillStyle = `rgba(255,255,255,${labelAlpha})`;
    ctx.font = `${fontSize}px monospace`;
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText(midiToName(zone.midi), zx + zw / 2, zy + zh / 2);
  }

  // Ball trails and balls
  for (let b = 0; b < balls.length; b++) {
    const ball = balls[b];

    // Trail
    if (ball.trail.length > 1) {
      for (let t = 1; t < ball.trail.length; t++) {
        const alpha = (t / ball.trail.length) * 0.3;
        const lw = 1 + (t / ball.trail.length) * 2;
        ctx.strokeStyle = `rgba(200,230,100,${alpha})`;
        ctx.lineWidth = lw;
        ctx.beginPath();
        ctx.moveTo(ball.trail[t - 1].x * w, ball.trail[t - 1].y * h);
        ctx.lineTo(ball.trail[t].x * w, ball.trail[t].y * h);
        ctx.stroke();
      }
    }

    // Ball
    const bx = ball.x * w;
    const by = ball.y * h;
    const br = ballRadius * w;
    const grad = ctx.createRadialGradient(bx, by, 0, bx, by, br);
    grad.addColorStop(0, '#eeff88');
    grad.addColorStop(1, '#88aa00');
    ctx.fillStyle = grad;
    ctx.beginPath();
    ctx.arc(bx, by, br, 0, Math.PI * 2);
    ctx.fill();
  }

  // Audio overlay
  if (!audioStarted) {
    ctx.fillStyle = 'rgba(0,0,0,0.45)';
    ctx.fillRect(0, 0, w, h);
    ctx.fillStyle = 'rgba(255,255,255,0.85)';
    ctx.font = '18px sans-serif';
    ctx.textAlign = 'center';
    ctx.textBaseline = 'middle';
    ctx.fillText('Click to start', w / 2, h / 2);
  }
}
