import test from 'node:test';
import assert from 'node:assert/strict';
import { RuntimeEstimator, formatRuntime } from '../public/runtime-estimate.mjs';

const battery = (current, updatedAt, extra = {}) => ({ connected: true, valid: true, current, updatedAt, remainingAh: 40, capacityAh: 100, ...extra });
function feed(estimator, current, from = 0, to = 120000) {
  let result;
  for (let time = from; time <= to; time += 3000) result = estimator.update(battery(current, time), time);
  return result;
}

test('steady discharge uses remaining Ah, charging uses missing Ah', () => {
  assert.equal(feed(new RuntimeEstimator(), -10).hours, 4);
  const charge = feed(new RuntimeEstimator(), 10);
  assert.equal(charge.hours, 6);
  assert.equal(formatRuntime(charge).label, 'Time until full');
});

test('alternating loads average by time, including irregular polling', () => {
  const estimator = new RuntimeEstimator();
  estimator.update(battery(-2, 0), 0);
  estimator.update(battery(-8, 3000), 3000);
  estimator.update(battery(-2, 15000), 15000);
  const result = estimator.update(battery(-8, 18000), 18000);
  assert.equal(result.averageAmps, -6);
  assert.equal(result.hours, 40 / 6);
});

test('a brief spike is damped and a sustained new load replaces the old window', () => {
  const estimator = new RuntimeEstimator();
  feed(estimator, -2);
  estimator.update(battery(-10, 123000), 123000);
  const spike = estimator.update(battery(-2, 126000), 126000);
  assert.ok(Math.abs(spike.averageAmps) < 2.3);
  const changed = feed(estimator, -10, 129000, 252000);
  assert.equal(changed.averageAmps, -10);
  assert.equal(changed.windowSeconds, 120);
});

test('duplicates do not count as new readings or keep a stale estimate alive', () => {
  const estimator = new RuntimeEstimator();
  feed(estimator, -10, 0, 15000);
  assert.equal(estimator.update(battery(-10, 15000), 18000).windowSeconds, 15);
  assert.equal(estimator.update(battery(-10, 15000), 30000).state, 'unavailable');
  assert.equal(estimator.update(battery(-10, 15000), 33000).state, 'unavailable');
  assert.equal(estimator.update(battery(-10, 36000), 36000).state, 'warming');
});

test('mode changes and device restarts start a new averaging window', () => {
  const estimator = new RuntimeEstimator();
  feed(estimator, -10);
  const switched = estimator.update(battery(5, 123000), 123000);
  assert.equal(switched.state, 'warming');
  assert.equal(switched.direction, 'charging');
  assert.equal(feed(estimator, 5, 126000, 141000).hours, 12);
  assert.equal(estimator.update(battery(5, 1000), 144000).state, 'warming');
});

test('idle and missing/invalid data never produce an infinite or misleading estimate', () => {
  assert.equal(feed(new RuntimeEstimator(), 0).state, 'idle');
  for (const extra of [{ current: null }, { capacityAh: 0 }, { remainingAh: 101 }, { current: Infinity }, { connected: false }, { valid: false }]) {
    const estimator = new RuntimeEstimator();
    feed(estimator, -10);
    assert.equal(estimator.update(battery(-10, 123000, extra), 123000).state, 'unavailable');
  }
});

test('brief zero-load samples contribute to the average; sustained idle then resume restarts it', () => {
  const estimator = new RuntimeEstimator();
  feed(estimator, -10);
  estimator.update(battery(0, 123000), 123000);
  assert.equal(estimator.update(battery(0, 126000), 126000).state, 'ready');
  assert.equal(feed(estimator, 0, 129000, 141000).state, 'idle');
  assert.equal(estimator.update(battery(-10, 144000), 144000).state, 'warming');
});

test('estimate expires without new messages and reset clears a previous session', () => {
  const estimator = new RuntimeEstimator();
  feed(estimator, -10);
  assert.equal(estimator.snapshot(135000).state, 'unavailable');
  estimator.reset();
  assert.equal(estimator.snapshot(120001).state, 'unavailable');
});

test('display rounds long estimates and explains smoothing and charging taper', () => {
  const result = formatRuntime({ state: 'ready', direction: 'charging', hours: 26.04, averageAmps: 2, windowSeconds: 120 });
  assert.equal(result.value, '≈ 26h 0m');
  assert.match(result.detail, /2 min average.*1.1 days.*Charging may slow/);
});
