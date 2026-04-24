export const site = {
  title: 'MOSAIC-32',
  tagline: 'Discrete MOSFET ALU datapath, hybrid system architecture',
  description:
    'Documentation of the MOSAIC-32 processor: a 32-bit hybrid discrete-logic computer built by Tyrone Marhguy at the University of Pennsylvania.',
  url: 'https://tmarhguy.com',
  portfolioUrl: 'https://tmarhguy.com',
  author: 'Tyrone Iras Marhguy',
  email: 'tmarhguy@seas.upenn.edu',
  location: 'Philadelphia, PA',
  avatarUrl: 'https://tmarhguy.com/images/tyrone.jpg',
  social: [
    { label: 'GitHub', href: 'https://github.com/tmarhguy' },
    { label: 'LinkedIn', href: 'https://linkedin.com/in/tmarhguy' },
    { label: 'Portfolio', href: 'https://tmarhguy.com', hint: 'Full project showcase' },
    { label: 'Substack', href: 'https://tmarhguy.substack.com' },
    { label: 'Project', href: 'https://alu.tmarhguy.com' },
  ],
} as const;

export const highlights = [
  {
    title: '32-bit locked architecture',
    detail: 'Four 8-bit ALU slices chained into MOSAIC-32 datapath',
    href: 'https://alu.tmarhguy.com',
  },
  {
    title: '11-board modular system',
    detail: 'ALU, control, memory, display, input, and shift modules',
    href: 'https://www.nextpcb.com?code=tmarhguy',
  },
  {
    title: '1.24M+ vectors generated',
    detail: 'Verification campaign through Falstad, SV, iverilog, GTKWave',
    href: 'https://github.com/tmarhguy',
  },
] as const;
