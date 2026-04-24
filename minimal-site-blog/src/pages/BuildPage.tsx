import { formatDuration, mosaicData } from '../data/mosaic32/data';
import { useDocPageMeta } from '../hooks/useDocPageMeta';

export function BuildPage() {
  useDocPageMeta(
    'Build',
    'MOSAIC-32 construction record: sessions, artifacts, pivots, and breakthroughs.',
    '/build',
  );

  return (
    <div className="doc-page">
      <header className="mb-6">
        <h1>Build Record</h1>
        <p className="mono">Vertical construction log pulled from session records.</p>
      </header>

      <section className="build-strip">
        {mosaicData.sessions.map((session) => (
          <article key={session.id} className="build-row">
            <aside className="mono">{session.date}</aside>
            <div className="panel">
              <p className="mono">Duration: {formatDuration(session.durationMinutes, true)}</p>
              <p>{session.primaryFocus}</p>
              <p className="mono small">Artifacts: {session.artifact.join(' | ')}</p>
              {session.pivots.length > 0 ? <p className="tag-amber">Pivot session</p> : null}
              {session.breakthrough.isBreakthrough ? <p className="tag-teal">Breakthrough session</p> : null}
            </div>
          </article>
        ))}
      </section>
    </div>
  );
}
