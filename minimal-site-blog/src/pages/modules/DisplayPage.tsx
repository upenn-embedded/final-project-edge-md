import { mosaicData } from '../../data/mosaic32/data';
import { useDocPageMeta } from '../../hooks/useDocPageMeta';
import { ModuleTemplate } from './ModuleTemplate';

export function DisplayPage() {
  useDocPageMeta('Display', 'MOSAIC-32 7-segment and LCD output split.', '/modules/display');

  return (
    <ModuleTemplate
      title="Display"
      status="Design defined"
      summary="Output subsystem split between numeric visibility and static textual context."
      stats={['7-segment numeric output', 'LCD static text role', 'Passive LCD addressing']}
      arcs={[{ title: 'Display arc', steps: mosaicData.arcs.displayArc }]}
      sections={[
        {
          heading: 'Numeric first',
          body: '7-segment display handles execution values so debugging state is visible at a glance.',
        },
        {
          heading: 'LCD scope',
          body: 'LCD is reserved for static program names and text presentation to avoid competing with numeric instrumentation.',
        },
      ]}
    />
  );
}
