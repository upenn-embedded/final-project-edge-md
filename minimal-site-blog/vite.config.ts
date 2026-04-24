import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import tailwindcss from '@tailwindcss/vite';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
/** Parent repo dump — do not watch (very large, slows file watcher). */
const parentTextDump = path.join(__dirname, '..', 'text.md');

export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    port: 3002,
    strictPort: false,
    watch: {
      ignored: ['**/node_modules/**', parentTextDump],
    },
  },
  optimizeDeps: {
    include: [
      'react',
      'react-dom',
      'react/jsx-runtime',
      'react-router',
      'react-router-dom',
      'react-markdown',
      'remark-gfm',
      'rehype-highlight',
      'rehype-slug',
      'rehype-autolink-headings',
    ],
  },
});
