const timelineNotes: Record<string, string> = {
  '2026-03-31': '8-bit ALU baseline and first datapath sketches.',
  '2026-04-01': 'Harvard memory split selected.',
  '2026-04-02': 'ALU logic moved to universal LUT MUX-select.',
  '2026-04-03': 'Carry and clock architecture stabilized.',
  '2026-04-04': 'Custom ISA and micro-step sequencing defined.',
  '2026-04-05': 'Logical shift board separated as dedicated module.',
  '2026-04-06': 'Display split and Bluetooth hardware hijack path.',
  '2026-04-07': 'MOSFET pair changed and 11-board partition locked.',
  '2026-04-08': '32-bit architecture lock.',
  '2026-04-09': 'Verification pipeline frozen and routing prep.',
};

export interface TimelineScrubberProps {
  dates: string[];
  selectedDate: string;
  onSelectDate: (isoDate: string) => void;
}

export function TimelineScrubber({ dates, selectedDate, onSelectDate }: TimelineScrubberProps) {
  const index = Math.max(0, dates.indexOf(selectedDate));
  const date = dates[index] ?? dates[0] ?? '';

  return (
    <section className="scrubber panel">
      <div className="scrubber-head">
        <h3>Project replay</h3>
        <p className="mono">{date}</p>
      </div>

      <label className="scrubber-range-label mono small scrubber-desktop-only" htmlFor="mosaic-timeline-range">
        Drag to move through Mar 31–Apr 9, 2026
      </label>
      <input
        id="mosaic-timeline-range"
        className="scrubber-desktop-only"
        type="range"
        min={0}
        max={Math.max(0, dates.length - 1)}
        value={index}
        onChange={(event) => {
          const i = Number(event.currentTarget.value);
          const next = dates[i];
          if (next) onSelectDate(next);
        }}
      />

      <div className="scrubber-mobile-only">
        <label className="mono small" htmlFor="mosaic-timeline-select">
          Jump to date
        </label>
        <select
          id="mosaic-timeline-select"
          className="scrubber-select mono"
          value={date}
          onChange={(e) => onSelectDate(e.currentTarget.value)}
        >
          {dates.map((d) => (
            <option key={d} value={d}>
              {d}
            </option>
          ))}
        </select>
      </div>

      <p>{timelineNotes[date] ?? 'Session milestone logged.'}</p>
    </section>
  );
}
