import type { ArcStep } from '../../data/mosaic32/schema';

export function ArcTimeline({ title, steps }: { title: string; steps: ArcStep[] }) {
  return (
    <section className="arc-timeline">
      <h3>{title}</h3>
      <ol>
        {steps.map((step) => (
          <li key={step.id} className={step.chosen ? 'chosen' : 'rejected'}>
            <p className="mono small">
              {step.date} · {step.module}
            </p>
            <p className="step-title">{step.title}</p>
            <p>{step.reason}</p>
            {!step.chosen && step.alternativesKilled?.length ? (
              <p className="mono small">Killed: {step.alternativesKilled.join(' | ')}</p>
            ) : null}
          </li>
        ))}
      </ol>
    </section>
  );
}
