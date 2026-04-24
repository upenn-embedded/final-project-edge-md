import type { ArcStep, Decision, Milestone, MosaicDataset, Pivot, Session } from './schema';
import { validateMosaicDataset } from './validate';

const decisions: Decision[] = [
  {
    id: 'd-harvard-split',
    date: '2026-04-01',
    sessionTime: '~11:30 PM',
    timeBucket: 'Late Night',
    title: 'Program and data memory split',
    fromState: 'Single EEPROM intended for instructions and mutable data',
    toState: 'Harvard split with AT28C256 for program and 62256 SRAM for data',
    why: 'EEPROM write latency made runtime data updates impractical for the target clock modes.',
    alternativesKilled: ['Single EEPROM for everything', 'Dual EEPROM architecture'],
    downstreamUnlocked: ['Reliable read/write data path', 'Microcode-friendly fetch/decode cycle'],
    reversal: false,
    module: 'Memory',
  },
  {
    id: 'd-alu-lut-mux',
    date: '2026-04-02',
    sessionTime: '~2:10 PM',
    timeBucket: 'Afternoon',
    title: 'Universal LUT via MUX-select adopted',
    fromState: 'Discrete gate trees and NAND-only synthesis explored',
    toState: '4-to-1 MUX-select logic per bit with opcode-driven truth table select',
    why: 'MUX-select gave compact logic composition with predictable transistor budget and clear scaling.',
    alternativesKilled: ['NAND-only synthesis', 'Pure discrete logic tree'],
    downstreamUnlocked: ['Consistent ALU slices', 'Straight path to board replication'],
    reversal: false,
    module: 'ALU',
  },
  {
    id: 'd-alu-zero-mask',
    date: '2026-04-02',
    sessionTime: '~9:00 PM',
    timeBucket: 'Night',
    title: 'Zero-mask arithmetic unit locked',
    fromState: 'Operation-specific arithmetic wiring with hardcoded constants',
    toState: 'B_ENABLE mask + XOR inverter M for ADD/SUB/INC/DEC/pass behavior',
    why: 'Single arithmetic primitive reduced special-case wiring and kept all 8-bit slices identical.',
    alternativesKilled: ['Dedicated adder variants for each op'],
    downstreamUnlocked: ['8-bit to 32-bit replication via identical boards'],
    reversal: false,
    module: 'ALU',
  },
  {
    id: 'd-carry-hybrid-cla',
    date: '2026-04-03',
    sessionTime: '~4:40 PM',
    timeBucket: 'Afternoon',
    title: '2x4-bit rippled CLA selected',
    fromState: 'Pure ripple carry baseline and full 8-bit CLA considered',
    toState: 'Two 4-bit CLA blocks chained per 8-bit slice',
    why: 'Hybrid CLA gave major delay improvements with lower gate cost than full 8-bit CLA.',
    alternativesKilled: ['Full 8-bit CLA', 'Pure ripple carry'],
    downstreamUnlocked: ['Faster critical path with manageable routing complexity'],
    reversal: false,
    module: 'ALU',
  },
  {
    id: 'd-clock-trimode',
    date: '2026-04-03',
    sessionTime: '~10:30 PM',
    timeBucket: 'Night',
    title: 'Tri-mode clock architecture defined',
    fromState: 'Single 555-only free-running clock',
    toState: 'Visual debug (1-100 Hz), medium, and high-speed modes with knob control',
    why: 'A single mode could not satisfy both human-debug visibility and throughput testing.',
    alternativesKilled: ['Fixed-frequency clock only'],
    downstreamUnlocked: ['Debug-first bring-up plus performance validation mode'],
    reversal: false,
    module: 'Control',
  },
  {
    id: 'd-isa-custom',
    date: '2026-04-04',
    sessionTime: '~1:15 AM',
    timeBucket: 'Late Night',
    title: 'Custom ISA retained over RISC-V compatibility',
    fromState: 'RISC-V mapping was evaluated for familiarity',
    toState: 'Custom 32-bit ISA and micro-step sequencing',
    why: 'Control complexity and board-level constraints favored an ISA tuned to the hardware datapath.',
    alternativesKilled: ['RISC-V compatibility layer'],
    downstreamUnlocked: ['Cleaner microcode ROM plan', 'Deterministic multi-cycle shift operations'],
    reversal: false,
    module: 'Control',
  },
  {
    id: 'd-shift-board',
    date: '2026-04-05',
    sessionTime: '~3:00 PM',
    timeBucket: 'Afternoon',
    title: 'Dedicated logical shift board',
    fromState: 'Barrel-shifter style all-at-once hardware considered',
    toState: '1-bit universal shifter over multiple micro-steps',
    why: 'Serial shift micro-steps reduced transistor and routing footprint while keeping control explicit.',
    alternativesKilled: ['Barrel shifter'],
    downstreamUnlocked: ['Lower complexity PCB for shift functions', 'Predictable multi-cycle timing'],
    reversal: false,
    module: 'Shift',
  },
  {
    id: 'd-bt-hijack',
    date: '2026-04-06',
    sessionTime: '~11:55 PM',
    timeBucket: 'Late Night',
    title: 'Bluetooth hardware hijack path',
    fromState: 'Software-heavy command ingestion path',
    toState: 'UART decoder to shadow register with MUX flip instruction injection',
    why: 'Instruction injection needed zero software overhead to preserve deterministic control behavior.',
    alternativesKilled: ['Firmware-mediated command path'],
    downstreamUnlocked: ['Live instruction injection and remote control for demos'],
    reversal: false,
    module: 'Input',
  },
  {
    id: 'd-display-split',
    date: '2026-04-06',
    sessionTime: '~8:20 PM',
    timeBucket: 'Night',
    title: 'Display responsibilities split',
    fromState: 'LCD as full output device was considered',
    toState: '7-segment for numeric output, LCD for static program text',
    why: 'Numeric values needed immediate readability while LCD bandwidth was better used for static labels.',
    alternativesKilled: ['LCD-only output'],
    downstreamUnlocked: ['Clear execution visibility and low-overhead text output'],
    reversal: false,
    module: 'Display',
  },
  {
    id: 'd-mosfet-pair',
    date: '2026-04-07',
    sessionTime: '~2:30 PM',
    timeBucket: 'Afternoon',
    title: 'DMN2041L/DMP2035U pair selected',
    fromState: 'Earlier BSS84/BSS138 selection',
    toState: 'Matched N/P MOSFET pair with substantially lower RDS(on)',
    why: 'Lower on-resistance improved switching margin and thermal headroom on dense slices.',
    alternativesKilled: ['BSS84/BSS138 pair'],
    downstreamUnlocked: ['More reliable board-level electrical performance'],
    reversal: true,
    module: 'ALU',
  },
  {
    id: 'd-board-partition',
    date: '2026-04-07',
    sessionTime: '~5:10 PM',
    timeBucket: 'Afternoon',
    title: 'Eleven-board partition locked',
    fromState: 'Single large board concept',
    toState: '11-board modular partition with backplane integration',
    why: 'Partitioning reduced routing risk and made fabrication/debug iterative instead of all-or-nothing.',
    alternativesKilled: ['Monolithic board'],
    downstreamUnlocked: ['Parallel bring-up and staged fabrication'],
    reversal: false,
    module: 'PCB',
  },
  {
    id: 'd-arch-lock-32',
    date: '2026-04-08',
    sessionTime: '~11:00 PM',
    timeBucket: 'Late Night',
    title: '32-bit architecture locked',
    fromState: 'Open-ended scaling discussion',
    toState: 'Fixed 32-bit architecture via four chained 8-bit ALU slices',
    why: '32-bit met capability goals while preserving realistic fabrication and verification scope.',
    alternativesKilled: ['Further scope expansion'],
    downstreamUnlocked: ['Stable routing targets and verification closure'],
    reversal: false,
    module: 'Architecture',
  },
  {
    id: 'd-verif-pipeline',
    date: '2026-04-09',
    sessionTime: '~6:45 PM',
    timeBucket: 'Night',
    title: 'Verification pipeline frozen',
    fromState: 'Ad-hoc simulation tools',
    toState: 'Falstad, Wokwi, Logisim, SystemVerilog delays, iverilog, GTKWave, static timing sheet',
    why: 'Cross-domain validation was required before committing dense board routing.',
    alternativesKilled: ['Single-simulator verification'],
    downstreamUnlocked: ['Reproducible sign-off path for ALU and control timing'],
    reversal: false,
    module: 'Verification',
  },
];

const pivots: Pivot[] = [
  {
    ...decisions.find((d) => d.id === 'd-mosfet-pair')!,
    directionChangedReason: 'Device research changed the transistor pair after electrical analysis.',
  },
  {
    ...decisions.find((d) => d.id === 'd-board-partition')!,
    directionChangedReason: 'Physical constraints forced a shift from monolithic to modular PCB strategy.',
  },
  {
    ...decisions.find((d) => d.id === 'd-alu-lut-mux')!,
    directionChangedReason: 'ALU logic philosophy moved from gate-by-gate synthesis to LUT-like mux-select composition.',
  },
];

const sessions: Session[] = [
  {
    id: 's-2026-03-31',
    date: '2026-03-31',
    dayOfWeek: 'Tuesday',
    startTimeEstimate: '~10:30 AM',
    endTimeEstimate: '~7:30 PM',
    durationMinutes: 540,
    primaryFocus: 'Baseline ALU slice behavior and first memory architecture assumptions',
    toolsUsed: ['Wokwi', 'Falstad'],
    decisionsMade: [],
    pivots: [],
    breakthrough: { isBreakthrough: false },
    artifact: ['Initial ALU logic sketch'],
    timeEstimateConfidence: 'low',
  },
  {
    id: 's-2026-04-01',
    date: '2026-04-01',
    dayOfWeek: 'Wednesday',
    startTimeEstimate: '~11:00 AM',
    endTimeEstimate: '~11:40 PM',
    durationMinutes: 760,
    primaryFocus: 'Harvard memory split and ISA shape',
    toolsUsed: ['Logisim Evolution', 'Notebook modeling'],
    decisionsMade: ['d-harvard-split'],
    pivots: [],
    breakthrough: {
      isBreakthrough: true,
      detail: 'Memory split removed EEPROM write bottleneck and unlocked realistic execution.',
    },
    artifact: ['Memory bus draft'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-02',
    date: '2026-04-02',
    dayOfWeek: 'Thursday',
    startTimeEstimate: '~9:00 AM',
    endTimeEstimate: '~10:40 PM',
    durationMinutes: 820,
    primaryFocus: 'ALU logic implementation arc and arithmetic unification',
    toolsUsed: ['Falstad', 'Wokwi', 'Digital'],
    decisionsMade: ['d-alu-lut-mux', 'd-alu-zero-mask'],
    pivots: ['d-alu-lut-mux'],
    breakthrough: {
      isBreakthrough: true,
      detail: 'Universal LUT mux-select concept clicked and simplified slice replication.',
    },
    artifact: ['ALU mux-select topology notes'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-03',
    date: '2026-04-03',
    dayOfWeek: 'Friday',
    startTimeEstimate: '~12:30 PM',
    endTimeEstimate: '~11:50 PM',
    durationMinutes: 680,
    primaryFocus: 'Carry architecture and clock strategy',
    toolsUsed: ['Falstad', 'KiCad ngspice'],
    decisionsMade: ['d-carry-hybrid-cla', 'd-clock-trimode'],
    pivots: [],
    breakthrough: { isBreakthrough: false },
    artifact: ['Clock schematic v1'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-04',
    date: '2026-04-04',
    dayOfWeek: 'Saturday',
    startTimeEstimate: '~2:00 PM',
    endTimeEstimate: '~10:00 PM',
    durationMinutes: 480,
    primaryFocus: 'Instruction format and control sequencing',
    toolsUsed: ['SystemVerilog', 'iverilog', 'GTKWave'],
    decisionsMade: ['d-isa-custom'],
    pivots: [],
    breakthrough: { isBreakthrough: false },
    artifact: ['Instruction format draft'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-05',
    date: '2026-04-05',
    dayOfWeek: 'Sunday',
    startTimeEstimate: '~10:00 AM',
    endTimeEstimate: '~6:00 PM',
    durationMinutes: 480,
    primaryFocus: 'Logical shift board architecture',
    toolsUsed: ['SystemVerilog', 'Logisim Evolution'],
    decisionsMade: ['d-shift-board'],
    pivots: [],
    breakthrough: { isBreakthrough: false },
    artifact: ['Shift board micro-step map'],
    timeEstimateConfidence: 'high',
  },
  {
    id: 's-2026-04-06',
    date: '2026-04-06',
    dayOfWeek: 'Monday',
    startTimeEstimate: '~1:30 PM',
    endTimeEstimate: '~11:55 PM',
    durationMinutes: 625,
    primaryFocus: 'Input and output interfaces',
    toolsUsed: ['KiCad', 'UART prototyping'],
    decisionsMade: ['d-bt-hijack', 'd-display-split'],
    pivots: [],
    breakthrough: {
      isBreakthrough: true,
      detail: 'Hardware instruction injection path enabled zero-overhead Bluetooth control.',
    },
    artifact: ['Input interface pin map', 'Display mux timing table'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-07',
    date: '2026-04-07',
    dayOfWeek: 'Tuesday',
    startTimeEstimate: '~11:00 AM',
    endTimeEstimate: '~11:10 PM',
    durationMinutes: 730,
    primaryFocus: 'MOSFET selection and board partitioning',
    toolsUsed: ['Datasheet analysis', 'KiCad'],
    decisionsMade: ['d-mosfet-pair', 'd-board-partition'],
    pivots: ['d-mosfet-pair', 'd-board-partition'],
    breakthrough: {
      isBreakthrough: true,
      detail: 'Electrical component pivot and 11-board partition stabilized fabrication strategy.',
    },
    artifact: ['Board partition map'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-08',
    date: '2026-04-08',
    dayOfWeek: 'Wednesday',
    startTimeEstimate: '~12:20 PM',
    endTimeEstimate: '~11:40 PM',
    durationMinutes: 680,
    primaryFocus: 'Architecture lock and full-system integration constraints',
    toolsUsed: ['System diagrams', 'KiCad'],
    decisionsMade: ['d-arch-lock-32'],
    pivots: [],
    breakthrough: { isBreakthrough: true, detail: '32-bit scope locked and integration boundaries frozen.' },
    artifact: ['32-bit topology diagram'],
    timeEstimateConfidence: 'medium',
  },
  {
    id: 's-2026-04-09',
    date: '2026-04-09',
    dayOfWeek: 'Thursday',
    startTimeEstimate: '~9:30 AM',
    endTimeEstimate: '~9:00 PM',
    durationMinutes: 690,
    primaryFocus: 'Verification sign-off and routing prep',
    toolsUsed: ['SystemVerilog', 'iverilog', 'GTKWave', 'Static timing sheet', 'KiCad'],
    decisionsMade: ['d-verif-pipeline'],
    pivots: [],
    breakthrough: { isBreakthrough: false },
    artifact: ['Critical path spreadsheet', 'Routing constraints notes'],
    timeEstimateConfidence: 'high',
  },
];

const milestones: Milestone[] = [
  {
    id: 'm-alu-vectors',
    date: '2026-04-02',
    title: 'ALU verification campaign scaled',
    whatItProved: 'ALU operation space could be tested exhaustively with automated vectors.',
    whatItUnlocked: 'Confidence to proceed to board-level synthesis and routing.',
  },
  {
    id: 'm-arch-lock',
    date: '2026-04-08',
    title: 'MOSAIC-32 architecture locked',
    whatItProved: 'Four 8-bit slices could realize the 32-bit datapath within board budget.',
    whatItUnlocked: 'Stable final routing and fabrication planning.',
  },
  {
    id: 'm-pcb-start',
    date: '2026-04-09',
    title: 'PCB routing sequence started',
    whatItProved: 'Signal-integrity constraints and module partitioning were mature enough for implementation.',
    whatItUnlocked: 'Manufacturing package preparation.',
  },
];

function step(
  id: string,
  date: string,
  title: string,
  chosen: boolean,
  module: ArcStep['module'],
  reason: string,
  alternativesKilled?: string[],
): ArcStep {
  return { id, date, title, chosen, module, reason, alternativesKilled };
}

const arcs: MosaicDataset['arcs'] = {
  aluLogicArc: [
    step('a1', '2026-04-01', 'Pure discrete gate arrays', false, 'ALU', 'Too large and difficult to route.', [
      'Large custom gate arrays',
    ]),
    step('a2', '2026-04-02', 'NAND-only synthesis', false, 'ALU', 'Gate depth and wiring overhead were excessive.'),
    step('a3', '2026-04-02', 'Standard 4:1 MUX logic', true, 'ALU', 'Balanced flexibility and transistor count.'),
    step('a4', '2026-04-02', 'ROM LUT implementation', false, 'ALU', 'Rejected to preserve transparent hardware identity.'),
    step('a5', '2026-04-02', 'Universal LUT via MUX-select', true, 'ALU', 'Locked approach for scalable ALU slices.'),
    step('a6', '2026-04-02', 'Zero-mask arithmetic unit', true, 'ALU', 'Unified add/sub/inc/dec/pass behavior per slice.'),
  ],
  carryArc: [
    step('c1', '2026-04-02', 'Ripple carry baseline', false, 'ALU', 'Functional but slower for long chains.'),
    step('c2', '2026-04-03', 'Full 8-bit CLA analysis', false, 'ALU', 'Higher gate/routing cost than needed per slice.'),
    step('c3', '2026-04-03', '2x4-bit rippled CLA', true, 'ALU', 'Best speed/complexity tradeoff on 8-bit board.'),
  ],
  memoryArc: [
    step('m1', '2026-03-31', 'Single EEPROM for code + data', false, 'Memory', 'Write latency too high for data path.'),
    step('m2', '2026-04-01', 'Harvard split (EEPROM + SRAM)', true, 'Memory', 'Separated immutable program and mutable data.'),
    step('m3', '2026-04-01', 'FRAM/PSRAM/dual-port SRAM alternatives', false, 'Memory', 'Availability and interface complexity concerns.'),
  ],
  clockArc: [
    step('k1', '2026-04-01', 'Single 555 timer', false, 'Control', 'Insufficient for both debug and fast modes.'),
    step('k2', '2026-04-03', 'Tri-mode clock with knob control', true, 'Control', 'Supports human-debug and high-speed operation.'),
  ],
  isaArc: [
    step('i1', '2026-04-01', '5-bit opcode baseline', true, 'Control', 'Simple opcode map for early decode logic.'),
    step('i2', '2026-04-04', 'RISC-V compatibility pass', false, 'Control', 'Complex mapping relative to hardware goals.'),
    step('i3', '2026-04-04', 'Custom 32-bit ISA lock', true, 'Control', 'Aligned ISA semantics with board-level microcode.'),
  ],
  toolchainArc: [
    step('t1', '2026-03-31', 'Wokwi', true, 'Verification', 'Fast chip-level behavior checks.'),
    step('t2', '2026-04-01', 'Falstad', true, 'Verification', 'Analog timing intuition and signal behavior.'),
    step('t3', '2026-04-02', 'Logisim Evolution', true, 'Verification', 'System-level logic organization.'),
    step('t4', '2026-04-04', 'SystemVerilog + injected delays', true, 'Verification', 'Digital timing realism with datasheet delay models.'),
    step('t5', '2026-04-04', 'iverilog + GTKWave', true, 'Verification', 'Waveform-based regression checks.'),
    step('t6', '2026-04-09', 'Static critical path sheet', true, 'Verification', 'Cross-check timing closure assumptions.'),
    step('t7', '2026-04-09', 'KiCad schematic/routing', true, 'PCB', 'Design-for-manufacture implementation.'),
  ],
  ioArc: [
    step('io1', '2026-04-01', 'DIP switches only', false, 'Input', 'Useful for bring-up but limited UX.'),
    step('io2', '2026-04-05', '4x4 keypad matrix', true, 'Input', 'Human input without external host dependency.'),
    step('io3', '2026-04-06', 'Bluetooth hardware hijack', true, 'Input', 'Remote instruction injection with zero software overhead.'),
  ],
  displayArc: [
    step('dsp1', '2026-04-02', 'LCD-only output concept', false, 'Display', 'Too slow and indirect for numeric debugging.'),
    step('dsp2', '2026-04-06', '7-segment for numeric output', true, 'Display', 'Immediate visibility for register/ALU values.'),
    step('dsp3', '2026-04-06', 'LCD for static text only', true, 'Display', 'Program labels and anthem text display role.'),
  ],
  architectureArc32: [
    step('ar1', '2026-03-31', '8-bit slice baseline', true, 'Architecture', 'Proven base unit for scaling.'),
    step('ar2', '2026-04-08', '32-bit lock via four slices', true, 'Architecture', 'Balanced capability and buildability.'),
  ],
  pcbArc: [
    step('p1', '2026-04-05', 'Single board concept', false, 'PCB', 'Board area and risk too high.'),
    step('p2', '2026-04-07', '11-board modular architecture', true, 'PCB', 'Partitioning improved manufacturability and debug flow.'),
  ],
};

export const mosaicData: MosaicDataset = {
  meta: {
    projectName: 'MOSAIC-32',
    processorLabel: 'the MOSAIC-32 processor',
    timelineStart: '2026-03-31',
    timelineEnd: '2026-04-09',
    dataSourcePath: './data/timeline-raw.md',
    architectureType: 'Hybrid',
    architectureSummary:
      'Discrete MOSFET ALU datapath (600+ transistors per slice), integrated IC control and memory subsystems.',
    nextPcbTrackingUrl: 'https://www.nextpcb.com?code=tmarhguy',
    metrics: {
      mosfetsPerAluBoard: '600+',
      totalBoards: 11,
      totalMosfetsApprox: '~6,600+',
      testVectorsGenerated: '1.24M+',
      testVectorsStatus: 'in-progress',
      transistorCountAlu: 3488,
    },
  },
  decisions,
  pivots,
  sessions,
  milestones,
  arcs,
};

const validationErrors = validateMosaicDataset(mosaicData);
if (validationErrors.length > 0) {
  const msg = `[MOSAIC-32] Dataset validation failed:\n${validationErrors.join('\n')}`;
  if (import.meta.env.DEV) {
    console.error(msg);
    throw new Error(msg);
  }
  console.warn(msg);
}

export function allTimelineDates(data: MosaicDataset = mosaicData): string[] {
  const { timelineStart, timelineEnd } = data.meta;
  const out: string[] = [];
  const [ys, ms, ds] = timelineStart.split('-').map(Number);
  const [ye, me, de] = timelineEnd.split('-').map(Number);
  const start = Date.UTC(ys, ms - 1, ds);
  const end = Date.UTC(ye, me - 1, de);
  for (let t = start; t <= end; t += 86400000) {
    out.push(new Date(t).toISOString().slice(0, 10));
  }
  return out;
}

export function totalSessionMinutes(data: MosaicDataset = mosaicData): number {
  return data.sessions.reduce((acc, session) => acc + session.durationMinutes, 0);
}

export function formatDuration(minutes: number, withEstimate = false): string {
  const h = Math.floor(minutes / 60);
  const m = minutes % 60;
  const core = m === 0 ? `${h}h` : `${h}h ${m}m`;
  return withEstimate ? `~${core}` : core;
}

export function decisionsByDate(data: MosaicDataset = mosaicData): Array<{ date: string; count: number }> {
  const counts = new Map<string, number>();
  for (const d of data.decisions) counts.set(d.date, (counts.get(d.date) ?? 0) + 1);
  return [...counts.entries()]
    .map(([date, count]) => ({ date, count }))
    .sort((a, b) => a.date.localeCompare(b.date));
}

export function cumulativeHoursByDate(data: MosaicDataset = mosaicData): Array<{ date: string; hours: number }> {
  const ordered = [...data.sessions].sort((a, b) => a.date.localeCompare(b.date));
  let acc = 0;
  return ordered.map((session) => {
    acc += session.durationMinutes;
    return { date: session.date, hours: Number((acc / 60).toFixed(2)) };
  });
}

export function filtersMetadata(data: MosaicDataset = mosaicData) {
  return {
    modules: [...new Set(data.decisions.map((d) => d.module))].sort(),
    dates: [...new Set(data.decisions.map((d) => d.date))].sort(),
    timeBuckets: ['Morning', 'Afternoon', 'Night', 'Late Night'] as const,
  };
}

/** Aggregate stats for dashboards (canonical counts from the dataset). */
export function getMosaicAggregateStats(data: MosaicDataset = mosaicData) {
  const decisionsByDay = decisionsByDate(data);
  const mostProductiveDay =
    decisionsByDay.length > 0
      ? decisionsByDay.reduce((a, b) => (b.count > a.count ? b : a), decisionsByDay[0]!)
      : { date: '', count: 0 };

  return {
    decisionCount: data.decisions.length,
    pivotCount: data.pivots.length,
    sessionCount: data.sessions.length,
    milestoneCount: data.milestones.length,
    totalSessionMinutes: totalSessionMinutes(data),
    mostProductiveDay: mostProductiveDay.date,
    mostProductiveDayDecisions: mostProductiveDay.count,
  };
}

