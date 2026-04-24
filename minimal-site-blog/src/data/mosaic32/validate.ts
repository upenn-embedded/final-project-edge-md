import type { Decision, Milestone, MosaicDataset, Pivot, Session } from './schema';

const MODULES = new Set([
  'ALU',
  'Control',
  'Memory',
  'Display',
  'Input',
  'Shift',
  'Architecture',
  'PCB',
  'Verification',
]);

const BUCKETS = new Set(['Morning', 'Afternoon', 'Night', 'Late Night']);

function isNonEmptyString(v: unknown): v is string {
  return typeof v === 'string' && v.trim().length > 0;
}

function validateDecision(d: Decision, label: string): string[] {
  const errs: string[] = [];
  if (!isNonEmptyString(d.id)) errs.push(`${label}: missing id`);
  if (!/^\d{4}-\d{2}-\d{2}$/.test(d.date)) errs.push(`${label} ${d.id}: bad date`);
  if (!isNonEmptyString(d.title)) errs.push(`${label} ${d.id}: missing title`);
  if (!MODULES.has(d.module)) errs.push(`${label} ${d.id}: invalid module ${d.module}`);
  if (!BUCKETS.has(d.timeBucket)) errs.push(`${label} ${d.id}: invalid timeBucket`);
  if (!Array.isArray(d.alternativesKilled)) errs.push(`${label} ${d.id}: alternativesKilled`);
  if (!Array.isArray(d.downstreamUnlocked)) errs.push(`${label} ${d.id}: downstreamUnlocked`);
  return errs;
}

function validatePivot(p: Pivot): string[] {
  const errs = validateDecision(p, 'Pivot');
  if (!isNonEmptyString(p.directionChangedReason)) errs.push(`Pivot ${p.id}: directionChangedReason`);
  return errs;
}

function validateSession(s: Session): string[] {
  const errs: string[] = [];
  if (!isNonEmptyString(s.id)) errs.push(`Session: missing id`);
  if (!/^\d{4}-\d{2}-\d{2}$/.test(s.date)) errs.push(`Session ${s.id}: bad date`);
  if (typeof s.durationMinutes !== 'number' || s.durationMinutes < 0) errs.push(`Session ${s.id}: durationMinutes`);
  if (!Array.isArray(s.toolsUsed)) errs.push(`Session ${s.id}: toolsUsed`);
  if (!Array.isArray(s.decisionsMade)) errs.push(`Session ${s.id}: decisionsMade`);
  if (!Array.isArray(s.pivots)) errs.push(`Session ${s.id}: pivots`);
  if (!s.breakthrough || typeof s.breakthrough.isBreakthrough !== 'boolean') {
    errs.push(`Session ${s.id}: breakthrough`);
  }
  return errs;
}

function validateMilestone(m: Milestone): string[] {
  const errs: string[] = [];
  if (!isNonEmptyString(m.id)) errs.push(`Milestone: missing id`);
  if (!/^\d{4}-\d{2}-\d{2}$/.test(m.date)) errs.push(`Milestone ${m.id}: bad date`);
  if (!isNonEmptyString(m.title)) errs.push(`Milestone ${m.id}: title`);
  return errs;
}

/** Returns validation errors; empty array means OK. */
export function validateMosaicDataset(data: MosaicDataset): string[] {
  const errs: string[] = [];
  if (data.meta.projectName !== 'MOSAIC-32') errs.push('meta.projectName');
  if (!/^\d{4}-\d{2}-\d{2}$/.test(data.meta.timelineStart)) errs.push('meta.timelineStart');
  if (!/^\d{4}-\d{2}-\d{2}$/.test(data.meta.timelineEnd)) errs.push('meta.timelineEnd');
  if (data.meta.timelineStart > data.meta.timelineEnd) errs.push('meta timeline order');

  const decisionIds = new Set<string>();
  for (const d of data.decisions) {
    for (const e of validateDecision(d, 'Decision')) errs.push(e);
    if (decisionIds.has(d.id)) errs.push(`duplicate decision id ${d.id}`);
    decisionIds.add(d.id);
  }

  const pivotIds = new Set<string>();
  for (const p of data.pivots) {
    for (const e of validatePivot(p)) errs.push(e);
    if (pivotIds.has(p.id)) errs.push(`duplicate pivot id ${p.id}`);
    pivotIds.add(p.id);
    if (!decisionIds.has(p.id)) errs.push(`pivot ${p.id} has no matching decision id`);
  }

  for (const s of data.sessions) {
    for (const e of validateSession(s)) errs.push(e);
    for (const id of s.decisionsMade) {
      if (!decisionIds.has(id)) errs.push(`session ${s.id} references unknown decision ${id}`);
    }
    for (const id of s.pivots) {
      if (!pivotIds.has(id)) errs.push(`session ${s.id} references unknown pivot ${id}`);
    }
  }

  for (const m of data.milestones) {
    for (const e of validateMilestone(m)) errs.push(e);
  }

  return errs;
}
