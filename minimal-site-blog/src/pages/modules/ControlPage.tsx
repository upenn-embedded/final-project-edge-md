import { mosaicData } from '../../data/mosaic32/data';
import { useDocPageMeta } from '../../hooks/useDocPageMeta';
import { ModuleTemplate } from './ModuleTemplate';

export function ControlPage() {
  useDocPageMeta(
    'Control',
    'MOSAIC-32 control unit: microcode, clock modes, ISA, hardware hijack path.',
    '/modules/control',
  );

  return (
    <ModuleTemplate
      title="Control Unit"
      status="Design locked"
      summary="Integrated control and memory sequencing layer for the MOSAIC-32 processor."
      stats={['Microcode EEPROM control', 'Tri-mode clock', 'Custom 32-bit ISA decode']}
      arcs={[
        { title: 'Clock design arc', steps: mosaicData.arcs.clockArc },
        { title: 'ISA and opcode arc', steps: mosaicData.arcs.isaArc },
      ]}
      sections={[
        {
          heading: 'Hardware hijack path',
          body: 'Bluetooth UART data enters a shadow register and MUX flip path so instructions can be injected without software overhead.',
        },
        {
          heading: 'Multi-cycle operations',
          body: 'Micro-step counters manage sequential shifts and control sequencing instead of transistor-expensive one-cycle structures.',
        },
      ]}
    />
  );
}
