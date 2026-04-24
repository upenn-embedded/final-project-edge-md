import { mosaicData } from '../../data/mosaic32/data';
import { useDocPageMeta } from '../../hooks/useDocPageMeta';
import { ModuleTemplate } from './ModuleTemplate';

export function ShiftPage() {
  useDocPageMeta('Shift', 'MOSAIC-32 logical shift board and micro-step control.', '/modules/shift');

  return (
    <ModuleTemplate
      title="Logical Shift Board"
      status="Design locked"
      summary="Dedicated shift module integrated into the MOSAIC-32 multi-cycle control model."
      stats={['1-bit universal shifter', 'Micro-step controlled', 'Lower routing cost than barrel design']}
      sections={[
        {
          heading: 'Design choice',
          body: 'A universal 1-bit shifter repeated over micro-steps replaced high-density barrel-shifter options to control board complexity.',
        },
        {
          heading: 'Integration',
          body: 'Control unit micro-step sequencing coordinates shift operations while preserving deterministic timing behavior.',
        },
      ]}
      arcs={[{ title: 'Related control arc', steps: mosaicData.arcs.isaArc }]}
    />
  );
}
