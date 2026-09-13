const WINDOW_MS = 120000;
const WARMUP_MS = 15000;
const STALE_MS = 15000;
const IDLE_AMPS = 0.05;

// Integrate current over elapsed time so duplicate or irregular polls do not
// skew the average. History belongs to this browser connection only.
export class RuntimeEstimator {
  constructor() { this.reset(); }

  reset() {
    this.segments = [];
    this.last = null;
    this.direction = 0;
    this.idleSince = null;
  }

  update(battery, now) {
    const fields = ['current', 'remainingAh', 'capacityAh', 'updatedAt'];
    if (!battery?.connected || !battery.valid || !fields.every(key => Number.isFinite(battery[key])) ||
        battery.capacityAh <= 0 || battery.remainingAh < 0 || battery.remainingAh > battery.capacityAh) {
      this.reset();
      return this.snapshot(now);
    }
    if (this.last && battery.updatedAt === this.last.battery.updatedAt) return this.snapshot(now);
    if (this.last && (now - this.last.at >= STALE_MS || battery.updatedAt < this.last.battery.updatedAt)) this.reset();

    const direction = Math.abs(battery.current) > IDLE_AMPS ? Math.sign(battery.current) : 0;
    if (direction && this.idleSince !== null && now - this.idleSince >= WARMUP_MS) this.reset();
    if (direction && this.direction && direction !== this.direction) this.reset();
    if (direction) {
      this.direction = direction;
      this.idleSince = null;
    } else if (this.idleSince === null) {
      this.idleSince = now;
    }
    if (this.last) this.segments.push({ start: this.last.at, end: now, current: this.last.battery.current });
    this.segments = this.segments.filter(segment => segment.end > now - WINDOW_MS);
    this.last = { at: now, battery: { ...battery } };
    return this.snapshot(now);
  }

  snapshot(now) {
    if (!this.last || now - this.last.at >= STALE_MS) return { state: 'unavailable' };
    if (!this.direction || (this.idleSince !== null && now - this.idleSince >= WARMUP_MS)) {
      return { state: 'idle' };
    }
    let duration = 0;
    let ampMs = 0;
    for (const segment of this.segments) {
      const ms = Math.max(0, segment.end - Math.max(segment.start, this.last.at - WINDOW_MS));
      duration += ms;
      ampMs += segment.current * ms;
    }
    const direction = this.direction > 0 ? 'charging' : 'discharging';
    if (duration < WARMUP_MS) return { state: 'warming', direction };
    const averageAmps = ampMs / duration;
    if (Math.abs(averageAmps) <= IDLE_AMPS) return { state: 'idle' };
    const { remainingAh, capacityAh } = this.last.battery;
    const hours = (direction === 'charging' ? capacityAh - remainingAh : remainingAh) / Math.abs(averageAmps);
    return { state: 'ready', direction, hours, averageAmps, windowSeconds: duration / 1000 };
  }
}

export function formatRuntime(estimate) {
  const label = estimate.direction === 'charging' ? 'Time until full' : estimate.direction === 'discharging' ? 'Time until empty' : 'Battery time estimate';
  if (estimate.state === 'unavailable') return { label, value: '—', detail: 'Waiting for fresh battery data.' };
  if (estimate.state === 'idle') return { label, value: 'Idle', detail: 'No sustained charge or discharge to estimate.' };
  if (estimate.state === 'warming') return { label, value: 'Learning…', detail: 'Gathering 15 seconds of readings to smooth brief spikes.' };
  const minutes = Math.round(estimate.hours * 60 / (estimate.hours >= 1 ? 5 : 1)) * (estimate.hours >= 1 ? 5 : 1);
  const hours = Math.floor(minutes / 60);
  const time = minutes === 0 ? '< 1 min' : hours ? `${hours}h ${minutes % 60}m` : `${minutes} min`;
  const window = estimate.windowSeconds >= 120 ? '2 min average' : `${Math.round(estimate.windowSeconds)}s average · building to 2 min`;
  const days = estimate.hours >= 24 ? ` · ${(estimate.hours / 24).toFixed(1)} days` : '';
  const caveat = estimate.direction === 'charging' ? 'Charging may slow near full.' : 'If your average use stays the same.';
  return { label, value: `≈ ${time}`, detail: `${Math.abs(estimate.averageAmps).toFixed(2)} A · ${window}${days}. ${caveat}` };
}
