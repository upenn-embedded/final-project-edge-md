# minimal-site-blog

Blog-focused static site for **Tyrone Marhguy** — minimal Chirpy-style shell (sidebar, typography, dark/light), Markdown posts, About page, RSS.

## Stack

Vite 7, React 19, React Router 7, Tailwind CSS 4, `react-markdown` + `gray-matter`.

## Content

- Posts: `content/posts/*.md` (YAML frontmatter: `title`, `date`, `description`, `tags`, optional `slug`, `draft`).
- Site copy & links: `src/data/site.ts`.

## Scripts

```bash
npm install
npm run dev      # http://localhost:3002
npm run build    # generates public/rss.xml, then TypeScript + Vite
npm run preview
```

Set `VITE_SITE_URL` (or `SITE_URL` for the RSS script only) if your canonical origin is not `https://tmarhguy.com`.

## Deploy

Output is `dist/`. Configure the host for SPA fallback so client routes (`/posts/...`, `/about`) resolve. Copy or serve `rss.xml` from `public/` at `/rss.xml`.

## Relation to the main portfolio

The interactive portfolio remains at [tmarhguy.com](https://tmarhguy.com). This package can be deployed as the same origin (path or subdomain) or as a standalone blog-only deployment.
