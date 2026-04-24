import ReactMarkdown from 'react-markdown';
import remarkGfm from 'remark-gfm';
import rehypeSlug from 'rehype-slug';
import rehypeAutolinkHeadings from 'rehype-autolink-headings';
import rehypeHighlight from 'rehype-highlight';

interface MarkdownPostProps {
  children: string;
}

export function MarkdownPost({ children }: MarkdownPostProps) {
  return (
    <div className="blog-prose">
      <ReactMarkdown
        remarkPlugins={[remarkGfm]}
        rehypePlugins={[
          rehypeSlug,
          [rehypeAutolinkHeadings, { behavior: 'wrap', properties: { className: 'anchor' } }],
          rehypeHighlight,
        ]}
      >
        {children}
      </ReactMarkdown>
    </div>
  );
}
