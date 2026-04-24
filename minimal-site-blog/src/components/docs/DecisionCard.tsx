import type { Decision, Pivot } from '../../data/mosaic32/schema';

interface DecisionCardProps {
  item: Decision | Pivot;
  isPivot?: boolean;
}

function isPivotRecord(item: Decision | Pivot): item is Pivot {
  return 'directionChangedReason' in item && typeof (item as Pivot).directionChangedReason === 'string';
}

export function DecisionCard({ item, isPivot = false }: DecisionCardProps) {
  const pivotReason = isPivotRecord(item) ? item.directionChangedReason : undefined;
  return (
    <article className={`decision-card ${isPivot ? 'decision-card-pivot' : ''}`}>
      <header className="decision-card-head">
        <span className="chip">{item.module}</span>
        <span className="mono">{item.date}</span>
        <span className="mono">{item.sessionTime}</span>
      </header>
      {isPivot && <p className="pivot-label">Direction changed</p>}
      {isPivot && pivotReason ? <p className="pivot-reason mono small">{pivotReason}</p> : null}
      <h3>{item.title}</h3>
      <p className="mono from-to">
        {item.fromState} {'->'} {item.toState}
      </p>
      <p>{item.why}</p>
      <p className="meta-line">
        <strong>Killed:</strong> {item.alternativesKilled.join(' | ')}
      </p>
      <p className="meta-line">
        <strong>Unlocked:</strong> {item.downstreamUnlocked.join(' | ')}
      </p>
    </article>
  );
}
