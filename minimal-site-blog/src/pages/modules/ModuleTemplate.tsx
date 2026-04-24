import { ArcTimeline } from '../../components/docs/ArcTimeline';
import { mosaicData } from '../../data/mosaic32/data';
import type { ArcStep } from '../../data/mosaic32/schema';

interface ModuleTemplateProps {
  title: string;
  status: string;
  summary: string;
  stats: string[];
  arcs?: Array<{ title: string; steps: ArcStep[] }>;
  sections: Array<{ heading: string; body: string }>;
}

export function ModuleTemplate({ title, status, summary, stats, arcs = [], sections }: ModuleTemplateProps) {
  return (
    <div className="doc-page">
      <header className="mb-6">
        <h1>{title}</h1>
        <p className="mono">Status: {status}</p>
        <p>{summary}</p>
      </header>
      <section className="stats-grid mb-8">
        {stats.map((stat) => (
          <article key={stat} className="panel stat">
            <p className="mono stat-value">{stat}</p>
          </article>
        ))}
      </section>
      {arcs.map((arc) => (
        <ArcTimeline key={arc.title} title={arc.title} steps={arc.steps} />
      ))}
      {sections.map((section) => (
        <section key={section.heading} className="panel mt-6">
          <h2>{section.heading}</h2>
          <p>{section.body}</p>
        </section>
      ))}
      <footer className="panel mt-8 mono small">
        Manufactured by{' '}
        <a href={mosaicData.meta.nextPcbTrackingUrl} rel="noopener noreferrer" target="_blank">
          NextPCB
        </a>
      </footer>
    </div>
  );
}
