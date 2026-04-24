import { Link } from 'react-router-dom';

export function NotFound() {
  return (
    <div className="mx-auto max-w-md py-24 text-center">
      <p className="mono text-xs uppercase tracking-widest text-[var(--text-dim)]">404</p>
      <h1 className="font-display mt-2 text-2xl font-bold text-[var(--text-main)]">Page not found</h1>
      <Link to="/" className="mt-6 inline-block text-[var(--accent)] no-underline">
        ← Home
      </Link>
    </div>
  );
}
