import { useState } from 'react';
import { Link, useLocation } from 'react-router-dom';
import { Menu, Moon, Sun, X } from 'lucide-react';
import { site } from '../data/site';

const navTop = [
  { to: '/', label: 'Home' },
  { to: '/decisions', label: 'Decisions' },
  { to: '/timeline', label: 'Timeline' },
  { to: '/build', label: 'Build' },
  { to: '/architecture', label: 'Architecture' },
];

const moduleNav = [
  { to: '/modules/alu', label: 'ALU' },
  { to: '/modules/control', label: 'Control' },
  { to: '/modules/memory', label: 'Memory' },
  { to: '/modules/display', label: 'Display' },
  { to: '/modules/input', label: 'Input' },
  { to: '/modules/shift', label: 'Shift' },
];

interface SidebarProps {
  theme: 'dark' | 'light';
  onToggleTheme: () => void;
}

export function Sidebar({ theme, onToggleTheme }: SidebarProps) {
  const { pathname } = useLocation();
  const [open, setOpen] = useState(false);

  const linkCls = (active: boolean) =>
    `block rounded px-3 py-2 text-sm font-medium no-underline transition-colors mono ${
      active
        ? 'bg-[var(--accent-soft)] text-[var(--text-main)]'
        : 'text-[var(--text-dim)] hover:bg-[var(--panel)] hover:text-[var(--text-main)]'
    }`;

  const inner = (
    <>
      <div className="mb-8 text-left">
        <div className="font-display text-lg font-bold text-[var(--text-main)]">{site.title}</div>
        <div className="mt-1 text-xs leading-snug text-[var(--text-dim)]">{site.tagline}</div>
      </div>

      <nav className="flex flex-col gap-1" aria-label="Primary">
        {navTop.map(({ to, label }) => (
          <Link
            key={to}
            to={to}
            className={linkCls(to === '/' ? pathname === '/' : pathname === to)}
            onClick={() => setOpen(false)}
          >
            {label}
          </Link>
        ))}
      </nav>

      <div className="mt-8 border-t border-[var(--line)] pt-6">
        <div className="mb-3 mono text-xs uppercase tracking-widest text-[var(--text-dim)]">Modules</div>
        <div className="flex flex-col gap-1">
          {moduleNav.map(({ to, label }) => (
            <Link key={to} to={to} className={linkCls(pathname === to)} onClick={() => setOpen(false)}>
              {label}
            </Link>
          ))}
        </div>
      </div>

      <div className="mt-auto flex flex-col gap-3 border-t border-[var(--line)] pt-6">
        <div className="flex flex-wrap justify-start gap-2">
          {site.social.map((s) => {
            return (
              <a
                key={s.label}
                href={s.href}
                target="_blank"
                rel="noopener noreferrer"
                title={s.label}
                aria-label={s.label}
                className="rounded border px-2 py-1 mono text-xs"
                style={{ borderColor: 'var(--line)', color: 'var(--accent)' }}
                onClick={() => setOpen(false)}
              >
                {s.label}
              </a>
            );
          })}
        </div>
        <div className="flex justify-start">
          <button
            type="button"
            onClick={onToggleTheme}
            className="inline-flex size-9 items-center justify-center rounded border"
            style={{ borderColor: 'var(--line)', color: 'var(--text-dim)' }}
            title={theme === 'dark' ? 'Light mode' : 'Dark mode'}
            aria-label={theme === 'dark' ? 'Switch to light mode' : 'Switch to dark mode'}
          >
            {theme === 'dark' ? <Sun size={18} strokeWidth={1.75} /> : <Moon size={18} strokeWidth={1.75} />}
          </button>
        </div>
      </div>
    </>
  );

  return (
    <>
      <header
        className="fixed left-0 right-0 top-0 z-50 flex h-14 items-center justify-between border-b px-4 md:hidden"
        style={{ background: 'var(--panel)', borderColor: 'var(--line)' }}
      >
        <span className="font-display font-bold text-[var(--text-main)]">{site.title}</span>
        <button
          type="button"
          className="rounded p-2 text-[var(--text-main)]"
          aria-label={open ? 'Close menu' : 'Open menu'}
          onClick={() => setOpen(!open)}
        >
          {open ? <X size={22} /> : <Menu size={22} />}
        </button>
      </header>

      {open && (
        <div
          className="fixed inset-0 z-40 bg-black/50 md:hidden"
          aria-hidden
          onClick={() => setOpen(false)}
        />
      )}

      <aside
        className={`fixed bottom-0 left-0 top-0 z-50 flex w-[min(100%,280px)] flex-col overflow-y-auto border-r p-6 transition-transform ${
          open ? 'translate-x-0' : '-translate-x-full'
        } md:static md:z-0 md:h-screen md:w-[260px] md:translate-x-0 md:shrink-0 md:border-r md:px-5 md:py-8 lg:w-[300px]`}
        style={{ background: 'var(--panel)', borderColor: 'var(--line)' }}
      >
        {inner}
      </aside>
    </>
  );
}
