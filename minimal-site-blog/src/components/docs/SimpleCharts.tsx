interface Datum {
  date: string;
  count: number;
}

interface HoursDatum {
  date: string;
  hours: number;
}

export function DecisionsBarChart({ data }: { data: Datum[] }) {
  const max = Math.max(1, ...data.map((d) => d.count));
  return (
    <div className="chart-wrap" role="img" aria-label="Decisions per day">
      {data.map((d) => (
        <div key={d.date} className="chart-bar-col">
          <div className="chart-bar" style={{ height: `${(d.count / max) * 100}%` }} />
          <span className="mono small">{d.date.slice(5)}</span>
          <span className="mono small">{d.count}</span>
        </div>
      ))}
    </div>
  );
}

export function CumulativeHoursChart({ data }: { data: HoursDatum[] }) {
  const max = Math.max(1, ...data.map((d) => d.hours));
  return (
    <div className="line-chart" role="img" aria-label="Cumulative hours">
      {data.map((d) => (
        <div key={d.date} className="line-point-col">
          <div className="line-point" style={{ bottom: `${(d.hours / max) * 100}%` }} />
          <span className="mono small">{d.date.slice(5)}</span>
        </div>
      ))}
    </div>
  );
}
