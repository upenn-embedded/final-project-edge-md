import { useEffect, useMemo, useState } from 'react';
import { ProjectMap } from '../components/docs/ProjectMap';
import { TimelineScrubber } from '../components/docs/TimelineScrubber';
import { allTimelineDates, formatDuration, mosaicData, totalSessionMinutes } from '../data/mosaic32/data';
import { site } from '../data/site';
import { setCanonical, setDocumentTitle, setMeta, setMetaProperty } from '../utils/seo';

const SITE_URL = import.meta.env.VITE_SITE_URL ?? 'https://tmarhguy.com';
const NEXTPCB = mosaicData.meta.nextPcbTrackingUrl;

export function Home() {
  const [counter, setCounter] = useState(0);
  const timelineDates = useMemo(() => allTimelineDates(mosaicData), []);
  const [replayDate, setReplayDate] = useState(
    () => timelineDates[timelineDates.length - 1] ?? mosaicData.meta.timelineEnd,
  );

  const totals = useMemo(
    () => ({
      decisions: mosaicData.decisions.length,
      pivots: mosaicData.pivots.length,
      totalHours: formatDuration(totalSessionMinutes(mosaicData)),
    }),
    [],
  );

  const m = mosaicData.meta.metrics;

  useEffect(() => {
    setDocumentTitle(`${site.title} — ${site.tagline}`);
    setMeta('description', site.description);
    setCanonical(SITE_URL.replace(/\/$/, '') + '/');
    setMetaProperty('og:type', 'website');
    setMetaProperty('og:title', site.title);
    setMetaProperty('og:description', site.description);
    setMetaProperty('og:image', site.avatarUrl);

    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
      setCounter(totals.decisions);
      return;
    }
    let frame = 0;
    const step = () => {
      frame += 1;
      const next = Math.min(totals.decisions, frame);
      setCounter(next);
      if (next < totals.decisions) window.requestAnimationFrame(step);
    };
    window.requestAnimationFrame(step);
  }, [totals.decisions]);

  return (
    <div className="doc-page">
      <header className="mb-10 max-w-3xl">
        <h1 className="font-display text-4xl font-bold text-[var(--text-main)] md:text-5xl">MOSAIC-32</h1>
        <p className="mt-4 leading-relaxed">
          Somewhere between a notebook and a timing report, the MOSAIC-32 processor became a locked 32-bit machine:
          discrete MOSFET ALU datapath (hybrid architecture with integrated control and memory), built solo by Tyrone
          Marhguy at Penn Engineering.
        </p>
      </header>

      <section className="stats-grid">
        <Stat label="MOSFETs per ALU board" value={m.mosfetsPerAluBoard} />
        <Stat label="Total boards" value={`${m.totalBoards}`} />
        <Stat label="Total MOSFETs (approx.)" value={m.totalMosfetsApprox} />
        <Stat label="ALU transistors (8-bit slice)" value={`${m.transistorCountAlu.toLocaleString()}`} />
        <Stat label="Test vectors" value={`${m.testVectorsGenerated} generated to date`} />
        <Stat label="ISA width" value="32-bit custom" />
        <Stat label="ALU slice topology" value="4 × 8-bit slices" />
        <Stat label="Timeline" value="Mar 31–Apr 9, 2026" />
        <Stat label="Total logged effort" value={totals.totalHours} />
        <Stat label="Total decisions" value={`${totals.decisions}`} />
        <Stat label="Total pivots" value={`${totals.pivots}`} />
      </section>

      <section className="panel mt-8">
        <h2>Live decision counter</h2>
        <p className="mono counter" aria-live="polite">
          {counter}
        </p>
      </section>

      <section className="panel mt-8">
        <h2>Project map</h2>
        <ProjectMap replayDate={replayDate} />
      </section>

      <section className="mt-8">
        <TimelineScrubber dates={timelineDates} selectedDate={replayDate} onSelectDate={setReplayDate} />
      </section>

      <section className="panel mt-8 sponsor-panel">
        <h2>Manufacturing</h2>
        <p>
          PCBs manufactured by{' '}
          <a href={NEXTPCB} rel="noopener noreferrer" target="_blank">
            NextPCB
          </a>
          . Use the partner link for tracking:{' '}
          <span className="mono small">{NEXTPCB}</span>
        </p>
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
