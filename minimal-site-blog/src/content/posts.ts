import GithubSlugger from 'github-slugger';

export interface PostFrontmatter {
  title: string;
  date: string;
  description?: string;
  tags?: string[];
  draft?: boolean;
  slug?: string;
  series?: string;
}

export interface BlogPost extends PostFrontmatter {
  slug: string;
  excerpt: string;
  body: string;
}

const rawModules = import.meta.glob('../../content/posts/*.md', {
  query: '?raw',
  import: 'default',
  eager: true,
}) as Record<string, string>;

function excerptFromBody(content: string, max = 140): string {
  const t = content
    .replace(/```[\s\S]*?```/g, ' ')
    .replace(/!\[.*?\]\(.*?\)/g, '')
    .replace(/\[(.*?)\]\(.*?\)/g, '$1')
    .replace(/[#*_`>]/g, '')
    .replace(/\s+/g, ' ')
    .trim();
  return t.length <= max ? t : `${t.slice(0, max).trim()}…`;
}

function fileSlug(path: string): string {
  const base = path.split('/').pop() ?? path;
  return base.replace(/^\d{4}-\d{2}-\d{2}-/, '').replace(/\.md$/i, '');
}

function parseFrontmatter(raw: string): { fm: string; body: string } {
  const m = raw.match(/^---\s*\r?\n([\s\S]*?)\r?\n---\s*\r?\n?([\s\S]*)$/);
  if (!m) return { fm: '', body: raw };
  return { fm: m[1], body: m[2] ?? '' };
}

function stripQuotes(s: string): string {
  const t = s.trim();
  if (
    (t.startsWith('"') && t.endsWith('"')) ||
    (t.startsWith("'") && t.endsWith("'"))
  ) {
    return t.slice(1, -1);
  }
  return t;
}

function parseYamlLike(frontmatter: string): Partial<PostFrontmatter> {
  const out: Partial<PostFrontmatter> = {};
  let inTags = false;

  const lines = frontmatter.split(/\r?\n/);
  for (const line of lines) {
    const l = line.trim();
    if (!l) continue;

    if (l.startsWith('tags:')) {
      inTags = true;
      out.tags = [];
      // Support inline arrays like: tags: [a, b]
      const inline = l.slice('tags:'.length).trim();
      if (inline.startsWith('[') && inline.endsWith(']')) {
        const inner = inline.slice(1, -1).trim();
        out.tags = inner
          ? inner.split(',').map((x) => stripQuotes(x.trim())).filter(Boolean)
          : [];
        inTags = false;
      }
      continue;
    }

    if (inTags) {
      const mArr = /^-\s+(.*)$/.exec(l);
      if (mArr) {
        out.tags = out.tags ?? [];
        out.tags.push(stripQuotes(mArr[1] ?? ''));
        continue;
      }
      // tags section ended
      inTags = false;
    }

    const mKV = /^([A-Za-z0-9_]+)\s*:\s*(.*)$/.exec(l);
    if (!mKV) continue;
    const key = mKV[1] as keyof PostFrontmatter;
    const rawVal = (mKV[2] ?? '').trim();
    if (!rawVal) continue;

    if (key === 'draft') {
      if (rawVal === 'true') out.draft = true;
      if (rawVal === 'false') out.draft = false;
      continue;
    }

    if (key === 'title' || key === 'date' || key === 'description' || key === 'slug' || key === 'series') {
      // Frontmatter values are strings in our content set.
      (out as Partial<PostFrontmatter>)[key] = stripQuotes(rawVal) as any;
      continue;
    }

    // Unknown keys are ignored.
  }

  return out;
}

function parseOne(path: string, raw: string): BlogPost | null {
  const { fm: fmRaw, body } = parseFrontmatter(raw);
  const fm = parseYamlLike(fmRaw);
  const title = fm.title?.toString();
  const date = fm.date?.toString();
  const draft = fm.draft === true;
  if (!title || !date) return null;
  if (draft) return null;

  const slug = fm.slug?.toString().trim() || fileSlug(path);
  const tags = fm.tags?.map((t) => String(t));
  const description = fm.description?.toString();
  const series = fm.series?.toString();

  return {
    title,
    date,
    description,
    tags,
    draft: draft ? true : undefined,
    slug,
    series,
    excerpt: description?.trim() ? description.trim() : excerptFromBody(body),
    body,
  };
}

let cache: BlogPost[] | null = null;

export function getAllPosts(): BlogPost[] {
  if (!cache) {
    const list: BlogPost[] = [];
    for (const [path, raw] of Object.entries(rawModules)) {
      const p = parseOne(path, raw);
      if (p) list.push(p);
    }
    list.sort((a, b) => new Date(b.date).getTime() - new Date(a.date).getTime());
    cache = list;
  }
  return cache;
}

export function getPost(slug: string): BlogPost | null {
  return getAllPosts().find((p) => p.slug === slug) ?? null;
}

export function getAllTags(): string[] {
  const s = new Set<string>();
  for (const p of getAllPosts()) {
    for (const t of p.tags ?? []) s.add(t);
  }
  return [...s].sort((a, b) => a.localeCompare(b));
}

export function estimateReadingMinutes(body: string): number {
  const n = body.trim().split(/\s+/).filter(Boolean).length;
  return Math.max(1, Math.round(n / 220));
}

export interface TocItem {
  id: string;
  text: string;
  level: 2 | 3;
}

export function buildToc(markdown: string): TocItem[] {
  const slugger = new GithubSlugger();
  const out: TocItem[] = [];
  for (const line of markdown.split('\n')) {
    const m3 = /^###\s+(.+)$/.exec(line);
    const m2 = /^##\s+(.+)$/.exec(line);
    const text = m3?.[1]?.trim() ?? m2?.[1]?.trim();
    if (!text) continue;
    out.push({ id: slugger.slug(text), text, level: m3 ? 3 : 2 });
  }
  return out;
}
