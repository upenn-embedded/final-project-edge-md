import { mosaicData } from '../../data/mosaic32/data';
import { useDocPageMeta } from '../../hooks/useDocPageMeta';
import { ModuleTemplate } from './ModuleTemplate';

export function InputPage() {
  useDocPageMeta('Input', 'MOSAIC-32 input: keypad, DIP, Bluetooth hardware hijack.', '/modules/input');

  return (
    <ModuleTemplate
      title="Input"
      status="Design defined"
      summary="Input path evolved from simple hardware toggles to keyboard and Bluetooth instruction injection."
      stats={['DIP switch baseline', '4x4 keypad matrix', 'Bluetooth hardware hijack path']}
      arcs={[{ title: 'Input arc', steps: mosaicData.arcs.ioArc }]}
      sections={[
        {
          heading: 'Hardware hijack',
          body: 'UART decoding and shadow register logic can push instructions into the execution path without software scheduling overhead.',
        },
        {
          heading: 'Why it matters',
          body: 'This keeps debugging deterministic while still allowing remote interaction and demonstrations.',
        },
      ]}
    />
  );
}
