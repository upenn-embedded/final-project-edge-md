import { mosaicData } from '../../data/mosaic32/data';
import { useDocPageMeta } from '../../hooks/useDocPageMeta';
import { ModuleTemplate } from './ModuleTemplate';

export function MemoryPage() {
  useDocPageMeta('Memory', 'MOSAIC-32 Harvard split: program EEPROM and data SRAM.', '/modules/memory');

  return (
    <ModuleTemplate
      title="Memory"
      status="Architecture stable"
      summary="Integrated memory layer paired with the discrete ALU datapath in the hybrid MOSAIC-32 architecture."
      stats={['AT28C256 program memory', '62256 SRAM data memory', 'Harvard split']}
      arcs={[{ title: 'Memory decision arc', steps: mosaicData.arcs.memoryArc }]}
      sections={[
        {
          heading: 'Harvard split',
          body: 'Program and data responsibilities are separated so immutable instruction fetch and mutable runtime data can coexist at practical speeds.',
        },
        {
          heading: 'Alternatives killed',
          body: 'FRAM, PSRAM, and dual-port SRAM options were evaluated and rejected due to complexity and integration cost.',
        },
      ]}
    />
  );
}
