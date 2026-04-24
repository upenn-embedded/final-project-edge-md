import { mosaicData } from '../../data/mosaic32/data';
import { useDocPageMeta } from '../../hooks/useDocPageMeta';
import { ModuleTemplate } from './ModuleTemplate';

export function AluPage() {
  useDocPageMeta(
    'ALU',
    'MOSAIC-32 discrete MOSFET ALU: logic arc, carry, zero-mask arithmetic, verification.',
    '/modules/alu',
  );

  return (
    <ModuleTemplate
      title="Arithmetic Logic Unit"
      status="Fabricated - Rev 1 complete"
      summary="Discrete MOSFET ALU datapath with hybrid integration into MOSAIC-32."
      stats={['3,488 transistors', '600+ MOSFETs per ALU board', '1.24M+ vectors generated', '270 x 270 mm board']}
      arcs={[
        { title: 'Logic implementation arc', steps: mosaicData.arcs.aluLogicArc },
        { title: 'Carry architecture arc', steps: mosaicData.arcs.carryArc },
      ]}
      sections={[
        {
          heading: 'Carry lookahead tradeoff',
          body: 'Full 8-bit carry lookahead was analyzed and rejected for this slice in favor of two 4-bit CLA blocks chained: roughly three times fewer lookahead gates than a monolithic 8-bit CLA at the cost of one extra carry boundary, acceptable for an 8-bit board before scaling concerns move to the 32-bit path.',
        },
        {
          heading: 'Zero-mask arithmetic',
          body: 'B_ENABLE masking plus XOR inverter control keeps arithmetic logic uniform so each 8-bit slice is physically identical.',
        },
        {
          heading: 'MOSFET selection pivot',
          body: 'The design pivoted from BSS84/BSS138 to DMN2041L/DMP2035U for lower RDS(on) and better electrical margin.',
        },
        {
          heading: 'Verification path',
          body: 'Falstad, Wokwi, Logisim, SystemVerilog with delay injection, iverilog/GTKWave, static timing spreadsheets, then KiCad routing constraints.',
        },
      ]}
    />
  );
}
