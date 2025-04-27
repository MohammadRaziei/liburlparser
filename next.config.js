module.exports = {
  // output: 'export',
  publicRuntimeConfig: {
    site: {
      name: 'Next.js + Tailwind CSS template',
      url:
        process.env.NODE_ENV === 'development'
          ? 'http://localhost:3000'
          : 'https://earvinpiamonte-nextjs-tailwindcss-template.vercel.app',
      title: 'Next.js + Tailwind CSS template',
      description: 'Next.js + Tailwind CSS template',
      socialPreview: '/images/preview.png',
    },
  },
  swcMinify: true,
};
