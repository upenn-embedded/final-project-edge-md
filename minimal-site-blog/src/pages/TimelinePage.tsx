import { Link } from 'react-router-dom';
import { CumulativeHoursChart } from '../components/docs/SimpleCharts';
import {
  cumulativeHoursByDate,
  formatDuration,
  getMosaicAggregateStats,
  mosaicData,
  totalSessionMinutes,
} from '../data/mosaic32/data';
import { useDocPageMeta } from '../hooks/useDocPageMeta';

export function TimelinePage() {
  useDocPageMeta(
    'Timeline',
    'MOSAIC-32 design sessions Mar 31–Apr 9, 2026: duration, tools, decisions, breakthroughs.',
    '/timeline',
  );

  const agg = getMosaicAggregateStats(mosaicData);
  const totalMinutes = totalSessionMinutes(mosaicData);
  const average = Math.round(totalMinutes / mosaicData.sessions.length);
  const longest = [...mosaicData.sessions].sort((a, b) => b.durationMinutes - a.durationMinutes)[0];

  return (
    <div className="doc-page">
      <header className="mb-6">
        <h1>Session Timeline</h1>
        <p className="mono">Contiguous work sessions across Mar 31–Apr 9, 2026</p>
      </header>

      <section className="panel mb-8">
        <h2>Cumulative hours</h2>
        <CumulativeHoursChart data={cumulativeHoursByDate(mosaicData)} />
      </section>

      <section className="timeline-list">
        {mosaicData.sessions.map((session) => (
          <article key={session.id} className={`panel ${session.breakthrough.isBreakthrough ? 'panel-breakthrough' : ''}`}>
            <header className="session-head">
              <h3>
                {session.date} · {session.dayOfWeek}
              </h3>
              <span className="mono">{formatDuration(session.durationMinutes, true)}</span>
            </header>
            <div className="duration-rail">
              <div style={{ width: `${Math.min(100, (session.durationMinutes / 1200) * 100)}%` }} />
            </div>
            <p>{session.primaryFocus}</p>
            <p className="mono small">Tools: {session.toolsUsed.join(' | ')}</p>
            <p className="mono small">
              Decisions:{' '}
              {session.decisionsMade.length > 0 ? (
                <Link to={`/decisions?date=${session.date}`} className="timeline-decision-link">
                  {session.decisionsMade.length}
                </Link>
              ) : (
                '0'
              )}{' '}
              · Pivots: {session.pivots.length}
            </p>
            {session.breakthrough.isBreakthrough && session.breakthrough.detail ? (
              <p className="breakthrough-note">{session.breakthrough.detail}</p>
            ) : null}
          </article>
        ))}
      </section>

      <section className="stats-grid mt-8">
        <Stat label="Total hours" value={formatDuration(totalMinutes)} />
        <Stat label="Total sessions" value={`${mosaicData.sessions.length}`} />
        <Stat label="Average session" value={formatDuration(average)} />
        <Stat label="Longest session" value={longest ? formatDuration(longest.durationMinutes, true) : '~0h'} />
        <Stat
          label="Most productive day (decisions)"
          value={agg.mostProductiveDay ? `${agg.mostProductiveDay} (${agg.mostProductiveDayDecisions})` : 'n/a'}
        />
      </section>
    </div>
  );
}

function Stat({ label, value }: { label: string; value: string }) {
  return (
    <article className="panel stat">
      <p className="mono stat-value">{value}</p>
      <p className="mono stat-label">{label}</p>
    </article>
  );
}
