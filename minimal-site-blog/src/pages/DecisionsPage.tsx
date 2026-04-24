import { useEffect, useMemo, useState } from 'react';
import { useSearchParams } from 'react-router-dom';
import { DecisionCard } from '../components/docs/DecisionCard';
import { type DecisionFilters, FiltersPanel } from '../components/docs/FiltersPanel';
import { DecisionsBarChart } from '../components/docs/SimpleCharts';
import { allTimelineDates, decisionsByDate, filtersMetadata, mosaicData } from '../data/mosaic32/data';
import type { MosaicModule } from '../data/mosaic32/schema';
import { useDocPageMeta } from '../hooks/useDocPageMeta';

function isIsoDate(s: string): boolean {
  return /^\d{4}-\d{2}-\d{2}$/.test(s);
}

export function DecisionsPage() {
  const [searchParams] = useSearchParams();
  const timelineDates = useMemo(() => allTimelineDates(mosaicData), []);
  const filterMeta = useMemo(() => filtersMetadata(mosaicData), []);
  const pivotIdSet = useMemo(() => new Set(mosaicData.pivots.map((p) => p.id)), []);

  const dateParam = searchParams.get('date') ?? '';
  const initialFrom =
    isIsoDate(dateParam) && timelineDates.includes(dateParam)
      ? dateParam
      : timelineDates[0] ?? mosaicData.meta.timelineStart;
  const initialTo =
    isIsoDate(dateParam) && timelineDates.includes(dateParam)
      ? dateParam
      : timelineDates[timelineDates.length - 1] ?? mosaicData.meta.timelineEnd;

  const [filters, setFilters] = useState<DecisionFilters>(() => ({
    modules: filterMeta.modules as MosaicModule[],
    showType: 'both',
    reversalsOnly: false,
    timeBuckets: [...filterMeta.timeBuckets],
    dateFrom: initialFrom,
    dateTo: initialTo,
  }));

  useDocPageMeta(
    'Decisions',
    'MOSAIC-32 engineering decisions and pivots, filterable by module and date.',
    '/decisions',
  );

  useEffect(() => {
    const d = searchParams.get('date');
    if (!d || !isIsoDate(d) || !timelineDates.includes(d)) return;
    setFilters((f) => ({ ...f, dateFrom: d, dateTo: d }));
  }, [searchParams, timelineDates]);

  const inDateRange = (date: string) => date >= filters.dateFrom && date <= filters.dateTo;

  const filteredDecisions = useMemo(() => {
    return mosaicData.decisions.filter((decision) => {
      if (!filters.modules.includes(decision.module)) return false;
      if (!filters.timeBuckets.includes(decision.timeBucket)) return false;
      if (filters.reversalsOnly && !decision.reversal) return false;
      if (!inDateRange(decision.date)) return false;
      if (filters.showType === 'both' && pivotIdSet.has(decision.id)) return false;
      return true;
    });
  }, [filters, pivotIdSet]);

  const filteredPivots = useMemo(() => {
    return mosaicData.pivots.filter((pivot) => {
      if (!filters.modules.includes(pivot.module)) return false;
      if (!filters.timeBuckets.includes(pivot.timeBucket)) return false;
      if (filters.reversalsOnly && !pivot.reversal) return false;
      if (!inDateRange(pivot.date)) return false;
      return true;
    });
  }, [filters]);

  return (
    <div className="doc-page">
      <header className="mb-6">
        <h1>Decisions and Pivots</h1>
        <p className="mono">MOSAIC-32 decision stream · Mar 31–Apr 9, 2026</p>
      </header>

      <section className="panel mb-8">
        <h2>Decisions per day</h2>
        <DecisionsBarChart data={decisionsByDate(mosaicData)} />
      </section>

      <section className="decisions-layout">
        <FiltersPanel
          allModules={filterMeta.modules as MosaicModule[]}
          filters={filters}
          onChange={setFilters}
          timelineDates={timelineDates}
        />

        <div className="decision-stream">
          {(filters.showType === 'both' || filters.showType === 'pivots') &&
            filteredPivots.map((pivot) => <DecisionCard key={`p-${pivot.id}`} item={pivot} isPivot />)}
          {(filters.showType === 'both' || filters.showType === 'decisions') &&
            filteredDecisions.map((decision) => <DecisionCard key={`d-${decision.id}`} item={decision} />)}
        </div>
      </section>
    </div>
  );
}
