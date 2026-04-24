import fs from 'node:fs';
import path from 'node:path';

const root = process.cwd();
const sourcePath = path.join(root, 'data', 'timeline-raw.md');
const outDir = path.join(root, 'src', 'data', 'mosaic32');
const outPath = path.join(outDir, 'generated-from-raw.json');

function ensureOutDir() {
  if (!fs.existsSync(outDir)) fs.mkdirSync(outDir, { recursive: true });
}

function writeOutput(payload) {
  ensureOutDir();
  fs.writeFileSync(outPath, JSON.stringify(payload, null, 2));
}

/** **Apr 7** style → 2026-04-07 */
function isoDateFromAprHeader(line) {
  const m = /^\*\*Apr\s+(\d{1,2})\*\*/.exec(line.trim());
  if (!m) return null;
  const day = String(Number(m[1])).padStart(2, '0');
  return `2026-04-${day}`;
}

/** **Mar 31** style → 2026-03-31 */
function isoDateFromMarHeader(line) {
  const m = /^\*\*Mar\s+(\d{1,2})\*\*/.exec(line.trim());
  if (!m) return null;
  const day = String(Number(m[1])).padStart(2, '0');
  return `2026-03-${day}`;
}

/** Lines that begin with an ISO date (metadata / session titles). */
function isoDateLineStart(line) {
  const t = line.trim();
  const m = /^(20\d{2}-\d{2}-\d{2})\b/.exec(t);
  return m ? m[1] : null;
}

function parseRaw(md) {
  const lines = md.split(/\r?\n/);
  const sessions = [];
  const hints = [];

  for (let i = 0; i < lines.length; i += 1) {
    const line = lines[i] ?? '';
    const date = isoDateFromAprHeader(line) || isoDateFromMarHeader(line) || isoDateLineStart(line);
    if (date) {
      sessions.push({
        id: `raw-${date}-${sessions.length + 1}`,
        date,
        line: i + 1,
        marker: line.trim().slice(0, 200),
      });
    }

    if (/Harvard|62256|AT28C|RISC-V|MUX|CLA|Bluetooth|NextPCB|MOSFET/i.test(line)) {
      hints.push({
        line: i + 1,
        text: line.trim().slice(0, 220),
      });
    }
  }

  return { sessions, hints };
}

if (!fs.existsSync(sourcePath)) {
  const payload = {
    status: 'missing-source',
    sourcePath: './data/timeline-raw.md',
    message:
      'Source timeline file is missing. Export your raw session dump to ./data/timeline-raw.md and rerun this script.',
    generatedAt: new Date().toISOString(),
    sessions: [],
    hints: [],
  };
  writeOutput(payload);
  console.log(`[mosaic32] Missing source file at ${sourcePath}`);
  console.log(`[mosaic32] Wrote placeholder output to ${outPath}`);
  process.exit(0);
}

const raw = fs.readFileSync(sourcePath, 'utf8');
const extracted = parseRaw(raw);
const payload = {
  status: 'ok',
  sourcePath: './data/timeline-raw.md',
  generatedAt: new Date().toISOString(),
  ...extracted,
};
writeOutput(payload);
console.log(`[mosaic32] Parsed ${payload.sessions.length} session markers`);
console.log(`[mosaic32] Collected ${payload.hints.length} decision hints`);
console.log(`[mosaic32] Wrote ${outPath}`);
