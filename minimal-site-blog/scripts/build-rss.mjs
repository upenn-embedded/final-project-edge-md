#!/usr/bin/env node
import fs from 'fs';
import path from 'path';
import { fileURLToPath } from 'url';
import matter from 'gray-matter';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.join(__dirname, '..');
const postsDir = path.join(root, 'content', 'posts');
const outPath = path.join(root, 'public', 'rss.xml');

const SITE = process.env.VITE_SITE_URL ?? process.env.SITE_URL ?? 'https://tmarhguy.com';
const AUTHOR = 'Tyrone Iras Marhguy';
const EMAIL = 'tmarhguy@seas.upenn.edu';

function esc(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function fileSlug(name) {
  return name.replace(/^\d{4}-\d{2}-\d{2}-/, '').replace(/\.md$/i, '');
}

function loadPosts() {
  if (!fs.existsSync(postsDir)) return [];
  const out = [];
  for (const f of fs.readdirSync(postsDir)) {
    if (!f.endsWith('.md')) continue;
    const raw = fs.readFileSync(path.join(postsDir, f), 'utf8');
    const { data, content } = matter(raw);
    if (data.draft) continue;
    if (!data.title || !data.date) continue;
    const slug = (data.slug && String(data.slug).trim()) || fileSlug(f);
    const date = new Date(data.date);
    if (Number.isNaN(date.getTime())) continue;
    const desc = data.description ? String(data.description) : content.replace(/\s+/g, ' ').trim().slice(0, 280);
    out.push({ title: String(data.title), slug, date, description: desc });
  }
  out.sort((a, b) => b.date - a.date);
  return out;
}

function build(posts) {
  const base = SITE.replace(/\/$/, '');
  const items = posts
    .map(
      (p) => `
    <item>
      <title>${esc(p.title)}</title>
      <link>${esc(`${base}/posts/${p.slug}`)}</link>
      <guid isPermaLink="true">${esc(`${base}/posts/${p.slug}`)}</guid>
      <pubDate>${p.date.toUTCString()}</pubDate>
      <description>${esc(p.description)}</description>
    </item>`,
    )
    .join('');
  const last = posts[0]?.date ?? new Date();
  return `<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0" xmlns:atom="http://www.w3.org/2005/Atom">
  <channel>
    <title>${esc(AUTHOR)} — Blog</title>
    <link>${esc(base + '/')}</link>
    <description>Writing on hardware, systems, and engineering.</description>
    <language>en-us</language>
    <lastBuildDate>${last.toUTCString()}</lastBuildDate>
    <atom:link href="${esc(base + '/rss.xml')}" rel="self" type="application/rss+xml"/>
    <managingEditor>${esc(EMAIL)} (${esc(AUTHOR)})</managingEditor>
    ${items}
  </channel>
</rss>
`;
}

const posts = loadPosts();
fs.mkdirSync(path.dirname(outPath), { recursive: true });
fs.writeFileSync(outPath, build(posts).trim() + '\n', 'utf8');
console.log(`Wrote ${path.relative(root, outPath)} (${posts.length} items)`);
