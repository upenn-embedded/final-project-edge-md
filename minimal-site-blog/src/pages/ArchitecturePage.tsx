import { ArcTimeline } from '../components/docs/ArcTimeline';
import { mosaicData } from '../data/mosaic32/data';
import { useDocPageMeta } from '../hooks/useDocPageMeta';

export function ArchitecturePage() {
  useDocPageMeta(
    'Architecture',
    'MOSAIC-32 32-bit hybrid architecture: four 8-bit ALU slices, memory split, PCB topology.',
    '/architecture',
  );

  return (
    <div className="doc-page">
      <header className="mb-6">
        <h1>Architecture</h1>
        <p>
          The MOSAIC-32 processor is locked at 32-bit: four identical 8-bit ALU slices chained in a hybrid architecture
          with integrated control and memory subsystems.
        </p>
      </header>

      <section className="panel mb-8">
        <h2>System topology</h2>
        <p className="mono">Input {'->'} Control {'->'} ALU slices {'->'} Register/Memory {'->'} Display</p>
        <p>Signal integrity constraints include conservative edge-rate handling, buffered buses, and series termination strategy.</p>
      </section>

      <ArcTimeline title="Architecture arc (locked 32-bit)" steps={mosaicData.arcs.architectureArc32} />
      <ArcTimeline title="PCB partition arc" steps={mosaicData.arcs.pcbArc} />

      <footer className="panel mt-8 mono small">
        Manufactured by{' '}
        <a href={mosaicData.meta.nextPcbTrackingUrl} rel="noopener noreferrer" target="_blank">
          NextPCB
        </a>
      </footer>
    </div>
  );
}
