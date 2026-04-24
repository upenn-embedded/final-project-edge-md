import { useNavigate } from 'react-router-dom';

type MapNode = {
  id: string;
  label: string;
  href: string;
  /** First calendar day this block is considered “present” in the replay. */
  firstSeen: string;
  x: number;
  y: number;
  w: number;
  h: number;
};

const NODES: MapNode[] = [
  { id: 'input', label: 'Input', href: '/modules/input', firstSeen: '2026-04-01', x: 16, y: 72, w: 76, h: 34 },
  { id: 'control', label: 'Control', href: '/modules/control', firstSeen: '2026-04-01', x: 108, y: 72, w: 82, h: 34 },
  { id: 'alu', label: 'ALU 4×8b', href: '/modules/alu', firstSeen: '2026-03-31', x: 206, y: 56, w: 100, h: 50 },
  { id: 'reg', label: 'Reg file', href: '/architecture', firstSeen: '2026-04-04', x: 320, y: 72, w: 76, h: 34 },
  { id: 'mem', label: 'Memory', href: '/modules/memory', firstSeen: '2026-04-01', x: 412, y: 72, w: 82, h: 34 },
  { id: 'disp', label: 'Display', href: '/modules/display', firstSeen: '2026-04-02', x: 508, y: 72, w: 82, h: 34 },
  { id: 'shift', label: 'Shift', href: '/modules/shift', firstSeen: '2026-04-05', x: 320, y: 16, w: 76, h: 32 },
];

function midRight(n: MapNode): { x: number; y: number } {
  return { x: n.x + n.w, y: n.y + n.h / 2 };
}

function midLeft(n: MapNode): { x: number; y: number } {
  return { x: n.x, y: n.y + n.h / 2 };
}

function LinkLine({ x1, y1, x2, y2, dashed }: { x1: number; y1: number; x2: number; y2: number; dashed?: boolean }) {
  return (
    <line
      x1={x1}
      y1={y1}
      x2={x2}
      y2={y2}
      stroke="var(--line)"
      strokeWidth={1.5}
      strokeDasharray={dashed ? '4 4' : undefined}
      markerEnd="url(#mosaic-arrowhead)"
    />
  );
}

export interface ProjectMapProps {
  /** ISO date from timeline scrubber; modules with firstSeen > date are dimmed. */
  replayDate: string;
}

export function ProjectMap({ replayDate }: ProjectMapProps) {
  const navigate = useNavigate();

  const byId = Object.fromEntries(NODES.map((n) => [n.id, n])) as Record<string, MapNode>;
  const chain: [string, string][] = [
    ['input', 'control'],
    ['control', 'alu'],
    ['alu', 'reg'],
    ['reg', 'mem'],
    ['mem', 'disp'],
  ];

  const alu = byId.alu;
  const shift = byId.shift;
  const aluShiftFrom = { x: alu.x + alu.w / 2, y: alu.y };
  const aluShiftTo = { x: shift.x + shift.w / 2, y: shift.y + shift.h };

  return (
    <div className="project-map-svg-wrap">
      <svg
        className="project-map-svg"
        viewBox="0 0 610 130"
        role="img"
        aria-label="MOSAIC-32 system map: click a module"
      >
        <title>MOSAIC-32 datapath</title>
        <defs>
          <marker id="mosaic-arrowhead" markerWidth="7" markerHeight="7" refX="6" refY="3.5" orient="auto">
            <polygon points="0 0, 7 3.5, 0 7" fill="var(--text-dim)" />
          </marker>
        </defs>

        {chain.map(([a, b]) => {
          const na = byId[a];
          const nb = byId[b];
          if (!na || !nb) return null;
          const p0 = midRight(na);
          const p1 = midLeft(nb);
          return <LinkLine key={`${a}-${b}`} x1={p0.x} y1={p0.y} x2={p1.x} y2={p1.y} />;
        })}
        <LinkLine x1={aluShiftFrom.x} y1={aluShiftFrom.y} x2={aluShiftTo.x} y2={aluShiftTo.y} dashed />

        {NODES.map((n) => {
          const active = replayDate >= n.firstSeen;
          return (
            <g key={n.id}>
              <rect
                x={n.x}
                y={n.y}
                width={n.w}
                height={n.h}
                rx={6}
                ry={6}
                fill={active ? 'var(--accent-soft)' : 'var(--panel)'}
                stroke={active ? 'var(--accent)' : 'var(--line)'}
                strokeWidth={active ? 2 : 1}
                opacity={active ? 1 : 0.38}
                className="project-map-hit"
                tabIndex={0}
                role="link"
                aria-label={`${n.label}, ${active ? 'in scope for selected date' : 'not yet in scope'}`}
                onClick={() => navigate(n.href)}
                onKeyDown={(e) => {
                  if (e.key === 'Enter' || e.key === ' ') {
                    e.preventDefault();
                    navigate(n.href);
                  }
                }}
              />
              <text
                x={n.x + n.w / 2}
                y={n.y + n.h / 2 + 4}
                textAnchor="middle"
                fill="var(--text-main)"
                opacity={active ? 1 : 0.45}
                pointerEvents="none"
                style={{ fontSize: '11px', fontFamily: 'var(--font-mono)' }}
              >
                {n.label}
              </text>
            </g>
          );
        })}
      </svg>
      <p className="mono small project-map-caption">
        Replay date <strong>{replayDate}</strong>: dimmed blocks were not yet locked. Click any block to open its page.
      </p>
    </div>
  );
}
