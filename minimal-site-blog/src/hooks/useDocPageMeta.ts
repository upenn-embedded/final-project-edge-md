import { useEffect } from 'react';
import { site } from '../data/site';
import { setCanonical, setDocumentTitle, setMeta } from '../utils/seo';

const SITE_URL = import.meta.env.VITE_SITE_URL ?? 'https://tmarhguy.com';

/** Sets document title, meta description, and canonical for MOSAIC-32 doc routes. */
export function useDocPageMeta(title: string, description: string, path: string) {
  useEffect(() => {
    setDocumentTitle(`${title} — ${site.title}`);
    setMeta('description', description);
    const base = SITE_URL.replace(/\/$/, '');
    setCanonical(`${base}${path.startsWith('/') ? path : `/${path}`}`);
  }, [title, description, path]);
}
