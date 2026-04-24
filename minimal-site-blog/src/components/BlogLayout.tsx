import { useEffect, useState } from 'react';
import { Outlet } from 'react-router-dom';
import { site } from '../data/site';
import { mosaicData } from '../data/mosaic32/data';
import { Sidebar } from './Sidebar';
import '../styles/blog-theme.css';

const KEY = 'mosaic32-theme';

export function BlogLayout() {
  const [theme, setTheme] = useState<'dark' | 'light'>(() => {
    if (typeof window === 'undefined') return 'dark';
    const s = localStorage.getItem(KEY);
    if (s === 'light' || s === 'dark') return s;
    return window.matchMedia('(prefers-color-scheme: light)').matches ? 'light' : 'dark';
  });

  useEffect(() => {
    localStorage.setItem(KEY, theme);
  }, [theme]);

  useEffect(() => {
    const bg = theme === 'dark' ? 'rgb(27, 27, 30)' : '#ffffff';
    document.documentElement.style.backgroundColor = bg;
    document.body.style.backgroundColor = bg;
    document.documentElement.style.colorScheme = theme;
  }, [theme]);

  return (
    <div className="blog-root" data-theme={theme}>
      <div className="flex min-h-screen md:pt-0 pt-14">
        <Sidebar theme={theme} onToggleTheme={() => setTheme((t) => (t === 'dark' ? 'light' : 'dark'))} />
        <div className="min-w-0 flex-1 md:pl-0">
          <main className="w-full max-w-[1250px] px-4 py-8 md:px-10 md:py-12">
            <Outlet />
          </main>
          <footer
            className="mx-auto max-w-[1250px] border-t px-4 py-8 text-center text-xs md:px-10"
            style={{ borderColor: 'var(--line)', color: 'var(--text-dim)' }}
          >
            <p>
              {site.author} ·{' '}
              <a href={`mailto:${site.email}`} className="text-[var(--accent)] no-underline">
                {site.email}
              </a>
            </p>
            <p className="mt-1">Manufactured by NextPCB · <a href={mosaicData.meta.nextPcbTrackingUrl}>nextpcb.com</a></p>
          </footer>
        </div>
      </div>
    </div>
  );
}
