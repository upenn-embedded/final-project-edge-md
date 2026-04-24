export type MosaicModule =
  | 'ALU'
  | 'Control'
  | 'Memory'
  | 'Display'
  | 'Input'
  | 'Shift'
  | 'Architecture'
  | 'PCB'
  | 'Verification';

export type SessionBucket = 'Morning' | 'Afternoon' | 'Night' | 'Late Night';
export type ClaimStatus = 'confirmed' | 'estimated' | 'in-progress';

export interface ProjectMetrics {
  mosfetsPerAluBoard: string;
  totalBoards: number;
  totalMosfetsApprox: string;
  testVectorsGenerated: string;
  testVectorsStatus: ClaimStatus;
  transistorCountAlu: number;
}

export interface ProjectMeta {
  projectName: 'MOSAIC-32';
  processorLabel: string;
  timelineStart: string;
  timelineEnd: string;
  dataSourcePath: string;
  architectureType: 'Hybrid';
  architectureSummary: string;
  nextPcbTrackingUrl: string;
  metrics: ProjectMetrics;
}

export interface Decision {
  id: string;
  date: string;
  sessionTime: string;
  timeBucket: SessionBucket;
  title: string;
  fromState: string;
  toState: string;
  why: string;
  alternativesKilled: string[];
  downstreamUnlocked: string[];
  reversal: boolean;
  module: MosaicModule;
  sourceHint?: string;
}

export interface Pivot extends Decision {
  directionChangedReason: string;
}

export interface Session {
  id: string;
  date: string;
  dayOfWeek: string;
  startTimeEstimate: string;
  endTimeEstimate: string;
  durationMinutes: number;
  primaryFocus: string;
  toolsUsed: string[];
  decisionsMade: string[];
  pivots: string[];
  breakthrough: {
    isBreakthrough: boolean;
    detail?: string;
  };
  artifact: string[];
  timeEstimateConfidence: 'high' | 'medium' | 'low';
}

export interface Milestone {
  id: string;
  date: string;
  title: string;
  whatItProved: string;
  whatItUnlocked: string;
  evidenceRefs?: string[];
}

export interface ArcStep {
  id: string;
  date: string;
  title: string;
  chosen: boolean;
  module: MosaicModule;
  reason: string;
  alternativesKilled?: string[];
}

export interface MosaicDataset {
  meta: ProjectMeta;
  decisions: Decision[];
  pivots: Pivot[];
  sessions: Session[];
  milestones: Milestone[];
  arcs: {
    aluLogicArc: ArcStep[];
    carryArc: ArcStep[];
    memoryArc: ArcStep[];
    clockArc: ArcStep[];
    isaArc: ArcStep[];
    toolchainArc: ArcStep[];
    ioArc: ArcStep[];
    displayArc: ArcStep[];
    architectureArc32: ArcStep[];
    pcbArc: ArcStep[];
  };
}
