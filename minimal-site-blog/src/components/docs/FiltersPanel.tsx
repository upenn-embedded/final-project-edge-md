import type { MosaicModule, SessionBucket } from '../../data/mosaic32/schema';

export interface DecisionFilters {
  modules: MosaicModule[];
  showType: 'both' | 'decisions' | 'pivots';
  reversalsOnly: boolean;
  timeBuckets: SessionBucket[];
  dateFrom: string;
  dateTo: string;
}

interface FiltersPanelProps {
  allModules: MosaicModule[];
  filters: DecisionFilters;
  onChange: (next: DecisionFilters) => void;
  timelineDates: string[];
}

function toggleInArray<T extends string>(arr: T[], value: T): T[] {
  return arr.includes(value) ? arr.filter((x) => x !== value) : [...arr, value];
}

export function FiltersPanel({ allModules, filters, onChange, timelineDates }: FiltersPanelProps) {
  const last = Math.max(0, timelineDates.length - 1);
  const rawFrom = timelineDates.indexOf(filters.dateFrom);
  const rawTo = timelineDates.indexOf(filters.dateTo);
  const fromIdx = rawFrom >= 0 ? rawFrom : 0;
  const toIdx = Math.max(fromIdx, rawTo >= 0 ? rawTo : last);

  return (
    <aside className="filter-panel">
      <h2>Filters</h2>

      <div className="filter-group">
        <h3>Date range</h3>
        <p className="mono small">
          {filters.dateFrom} → {filters.dateTo}
        </p>
        <label className="mono small filter-range-label" htmlFor="filter-date-from">
          From
        </label>
        <input
          id="filter-date-from"
          type="range"
          min={0}
          max={last}
          value={fromIdx}
          onChange={(e) => {
            const i = Number(e.currentTarget.value);
            const nextFrom = timelineDates[i] ?? filters.dateFrom;
            const nextTo = filters.dateTo < nextFrom ? nextFrom : filters.dateTo;
            onChange({ ...filters, dateFrom: nextFrom, dateTo: nextTo });
          }}
        />
        <label className="mono small filter-range-label" htmlFor="filter-date-to">
          To
        </label>
        <input
          id="filter-date-to"
          type="range"
          min={fromIdx}
          max={last}
          value={toIdx}
          onChange={(e) => {
            const i = Number(e.currentTarget.value);
            const nextTo = timelineDates[i] ?? filters.dateTo;
            onChange({ ...filters, dateTo: nextTo });
          }}
        />
      </div>

      <div className="filter-group">
        <h3>Module</h3>
        <div className="chip-wrap">
          {allModules.map((module) => (
            <button
              key={module}
              type="button"
              className={`chip-btn ${filters.modules.includes(module) ? 'active' : ''}`}
              onClick={() => onChange({ ...filters, modules: toggleInArray(filters.modules, module) })}
            >
              {module}
            </button>
          ))}
        </div>
      </div>

      <div className="filter-group">
        <h3>Type</h3>
        <div className="chip-wrap">
          {(['both', 'decisions', 'pivots'] as const).map((type) => (
            <button
              key={type}
              type="button"
              className={`chip-btn ${filters.showType === type ? 'active' : ''}`}
              onClick={() => onChange({ ...filters, showType: type })}
            >
              {type}
            </button>
          ))}
        </div>
      </div>

      <div className="filter-group">
        <h3>Time of day</h3>
        <div className="chip-wrap">
          {(['Morning', 'Afternoon', 'Night', 'Late Night'] as SessionBucket[]).map((bucket) => (
            <button
              key={bucket}
              type="button"
              className={`chip-btn ${filters.timeBuckets.includes(bucket) ? 'active' : ''}`}
              onClick={() => onChange({ ...filters, timeBuckets: toggleInArray(filters.timeBuckets, bucket) })}
            >
              {bucket}
            </button>
          ))}
        </div>
      </div>

      <label className="toggle-row">
        <input
          type="checkbox"
          checked={filters.reversalsOnly}
          onChange={(event) => onChange({ ...filters, reversalsOnly: event.currentTarget.checked })}
        />
        Reversals only
      </label>
    </aside>
  );
}
