// Ball and zone physics simulation
// PURE — no DOM, no browser APIs, no Math.random()

import { circleRectOverlap, minOverlapFace, circleCircleOverlap } from './collision.js';
import { getScaleNotesInOctave, quantizeToScale } from './scales.js';

const TRAIL_MAX = 60;
const LOCKOUT_TICKS = 12;
const FLASH_DECAY = 0.04;
const EPSILON = 0.001;
const MAX_SPEED = 0.025; // hard velocity cap to prevent runaway

// Main physics tick. Mutates balls/flash/lockout in place, returns events array.
// dt is normalized — 1.0 = one standard tick
export function tick(persistent, transient, dt) {
  const events = [];
  const {
    gravity, bounce, friction, ballSize, minEnergy,
    solidZones, ballCollide, zones,
    momentum, jitter,
    fanAmount, fanSpeed, fanDirection,
    randomPitch, randomOctaveChance, randomOctaveAmount, randomVelocity,
    scaleRoot, scaleName,
  } = persistent;
  const { balls, zoneFlash, zoneLockout, prng } = transient;

  transient.tickCount += dt;

  const gravityVal = (gravity / 100) * 0.0003;
  const frictionVal = (friction / 100) * 0.03;
  // Bounce is elasticity only — capped at 1.0 (perfectly elastic). Never adds energy on its own.
  const bounceVal = Math.min(bounce / 100, 1.0);
  const wallBounceVal = bounceVal;
  const ballRadius = 0.005 + (ballSize / 18) * 0.015;
  const minEnergyVal = (minEnergy / 100) * 0.004;
  // Momentum: extra energy on zone bounce, with diminishing returns at high speeds
  const momentumRaw = momentum / 100; // 0-1
  // Cubic curve so low values (1-10) are very subtle, high values still effective
  const jitterNorm = jitter / 100;
  const jitterVal = jitterNorm * jitterNorm * jitterNorm * 0.003;

  // Fan force calculation
  const fanForceVal = (fanAmount / 100) * 0.0004;
  const fanFreq = (fanSpeed / 100) * 0.15;
  const fanPhase = transient.tickCount * fanFreq;
  const fanOsc = Math.sin(fanPhase);

  for (let b = 0; b < balls.length; b++) {
    const ball = balls[b];

    // 1. Gravity
    ball.vy += gravityVal * dt;

    // 2. Fan force
    if (fanForceVal > 0) {
      const force = fanOsc * fanForceVal * dt;
      switch (fanDirection) {
        case 'north':
          ball.vy -= Math.abs(force);
          break;
        case 'south':
          ball.vy += Math.abs(force);
          break;
        case 'east':
          ball.vx += Math.abs(force);
          break;
        case 'west':
          ball.vx -= Math.abs(force);
          break;
        case 'random': {
          // Use a slowly rotating angle derived from tick count + ball index
          const angle = transient.tickCount * fanFreq * 0.3 + b * 2.39996;
          ball.vx += Math.cos(angle) * force;
          ball.vy += Math.sin(angle) * force;
          break;
        }
      }
    }

    // 3. Jitter — random velocity perturbation
    if (jitterVal > 0) {
      ball.vx += (prng() - 0.5) * 2 * jitterVal * dt;
      ball.vy += (prng() - 0.5) * 2 * jitterVal * dt;
    }

    // 4. Friction
    const fricMul = Math.pow(1 - frictionVal, dt);
    ball.vx *= fricMul;
    ball.vy *= fricMul;

    // 5. Minimum energy enforcement
    const spdAfterFric = Math.sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
    if (spdAfterFric < minEnergyVal && minEnergyVal > 0) {
      if (spdAfterFric > 0.00001) {
        const scale = minEnergyVal / spdAfterFric;
        ball.vx *= scale;
        ball.vy *= scale;
      } else {
        // Use PRNG for direction when fully stopped, instead of fixed 45°
        const angle = prng() * Math.PI * 2;
        ball.vx = Math.cos(angle) * minEnergyVal;
        ball.vy = Math.sin(angle) * minEnergyVal;
      }
    }

    // 6. Update position
    ball.x += ball.vx * dt;
    ball.y += ball.vy * dt;

    // 7. Wall bouncing (walls never add energy, capped at 1.0)
    if (ball.x - ballRadius < 0) {
      ball.x = ballRadius + EPSILON;
      ball.vx = Math.abs(ball.vx) * wallBounceVal;
    } else if (ball.x + ballRadius > 1) {
      ball.x = 1 - ballRadius - EPSILON;
      ball.vx = -Math.abs(ball.vx) * wallBounceVal;
    }
    if (ball.y - ballRadius < 0) {
      ball.y = ballRadius + EPSILON;
      ball.vy = Math.abs(ball.vy) * wallBounceVal;
    } else if (ball.y + ballRadius > 1) {
      ball.y = 1 - ballRadius - EPSILON;
      ball.vy = -Math.abs(ball.vy) * wallBounceVal;
    }

    // 7b. Hard velocity cap
    const spdNow = Math.sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
    if (spdNow > MAX_SPEED) {
      const capScale = MAX_SPEED / spdNow;
      ball.vx *= capScale;
      ball.vy *= capScale;
    }

    // 8. Zone collision
    for (let z = 0; z < zones.length; z++) {
      const zone = zones[z];
      if (zoneLockout[z] > 0) continue;

      if (circleRectOverlap(ball.x, ball.y, ballRadius, zone.cx, zone.cy, zone.hw, zone.hh)) {
        const impactSpeed = Math.sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
        const velocity = Math.min(1.0, Math.max(0.3, impactSpeed / 0.012));

        // Determine output MIDI note with randomization
        let outMidi = zone.midi;

        // Random pitch — replace with random scale note in same octave
        if (randomPitch > 0 && prng() * 100 < randomPitch) {
          const scaleNotes = getScaleNotesInOctave(outMidi, scaleRoot, scaleName);
          if (scaleNotes.length > 0) {
            outMidi = scaleNotes[Math.floor(prng() * scaleNotes.length)];
          }
        }

        // Random octave — shift up or down by random amount
        if (randomOctaveChance > 0 && prng() * 100 < randomOctaveChance) {
          const maxShift = randomOctaveAmount;
          // Random integer from -maxShift to +maxShift (excluding 0)
          let shift = Math.floor(prng() * maxShift * 2 + 1) - maxShift;
          if (shift === 0) shift = prng() < 0.5 ? -1 : 1;
          const shifted = outMidi + shift * 12;
          if (shifted >= 12 && shifted <= 120) {
            outMidi = shifted;
          }
        }

        // Random velocity variation
        let outVelocity = velocity;
        if (randomVelocity > 0) {
          const velRange = (randomVelocity / 100);
          outVelocity = velocity * (1 - velRange) + prng() * velocity * velRange * 2;
          outVelocity = Math.min(1.0, Math.max(0.1, outVelocity));
        }

        events.push({
          type: 'noteOn',
          midi: outMidi,
          velocity: outVelocity,
          zoneIndex: z,
        });

        zoneFlash[z] = 1.0;
        zoneLockout[z] = LOCKOUT_TICKS;

        // Solid zone bounce with speed-aware momentum
        if (solidZones) {
          const face = minOverlapFace(ball.x, ball.y, ballRadius, zone.cx, zone.cy, zone.hw, zone.hh);
          // Momentum adds less energy when already moving fast (diminishing returns)
          const speedRatio = Math.min(1.0, impactSpeed / MAX_SPEED);
          const momentumBoost = momentumRaw * (1.0 - speedRatio * 0.8); // fades to 20% effect at max speed
          const bounceMultiplier = bounceVal * (1.0 + momentumBoost);
          switch (face) {
            case 'left':
              ball.vx = -Math.abs(ball.vx) * bounceMultiplier;
              ball.x = zone.cx - zone.hw - ballRadius - EPSILON;
              break;
            case 'right':
              ball.vx = Math.abs(ball.vx) * bounceMultiplier;
              ball.x = zone.cx + zone.hw + ballRadius + EPSILON;
              break;
            case 'top':
              ball.vy = -Math.abs(ball.vy) * bounceMultiplier;
              ball.y = zone.cy - zone.hh - ballRadius - EPSILON;
              break;
            case 'bottom':
              ball.vy = Math.abs(ball.vy) * bounceMultiplier;
              ball.y = zone.cy + zone.hh + ballRadius + EPSILON;
              break;
          }
          // Cap velocity after momentum-boosted bounce
          const postBounceSpd = Math.sqrt(ball.vx * ball.vx + ball.vy * ball.vy);
          if (postBounceSpd > MAX_SPEED) {
            const cs = MAX_SPEED / postBounceSpd;
            ball.vx *= cs;
            ball.vy *= cs;
          }
        }
      }
    }

    // 9. Trail
    ball.trail.push({ x: ball.x, y: ball.y });
    if (ball.trail.length > TRAIL_MAX) {
      ball.trail.shift();
    }
  }

  // 10. Ball-ball collision
  if (ballCollide) {
    for (let i = 0; i < balls.length; i++) {
      for (let j = i + 1; j < balls.length; j++) {
        const a = balls[i];
        const b2 = balls[j];
        const result = circleCircleOverlap(a.x, a.y, ballRadius, b2.x, b2.y, ballRadius);
        if (result.overlapping && result.dist > 0.0001) {
          const nx = result.dx / result.dist;
          const ny = result.dy / result.dist;

          const dvx = a.vx - b2.vx;
          const dvy = a.vy - b2.vy;
          const dvn = dvx * nx + dvy * ny;

          if (dvn > 0) {
            a.vx -= dvn * nx;
            a.vy -= dvn * ny;
            b2.vx += dvn * nx;
            b2.vy += dvn * ny;
          }

          const overlap = result.minDist - result.dist;
          const pushX = (overlap / 2 + EPSILON) * nx;
          const pushY = (overlap / 2 + EPSILON) * ny;
          a.x -= pushX;
          a.y -= pushY;
          b2.x += pushX;
          b2.y += pushY;
        }
      }
    }
  }

  // Decay flash and lockout
  for (let z = 0; z < zones.length; z++) {
    if (zoneFlash[z] > 0) {
      zoneFlash[z] = Math.max(0, zoneFlash[z] - FLASH_DECAY * dt);
    }
    if (zoneLockout[z] > 0) {
      zoneLockout[z] = Math.max(0, zoneLockout[z] - dt);
    }
  }

  return events;
}
