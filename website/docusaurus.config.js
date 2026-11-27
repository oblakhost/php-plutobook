// @ts-check
// Documentation site for PlutoBook for PHP. The pages live in ../docs as plain
// Markdown, so they also read well on GitHub.
import {themes as prismThemes} from 'prism-react-renderer';

const repo = 'https://github.com/oblakhost/php-plutobook';
const packagist = 'https://packagist.org/packages/oblakhost/ext-plutobook';

/** @type {import('@docusaurus/types').Config} */
const config = {
  title: 'PlutoBook for PHP',
  tagline: 'Make PDFs from HTML and CSS, right inside PHP.',
  url: 'https://php.plutobook.dev',
  baseUrl: '/',
  trailingSlash: false,
  onBrokenLinks: 'throw',
  onBrokenAnchors: 'throw',
  markdown: {
    format: 'detect',
    hooks: {onBrokenMarkdownLinks: 'throw'},
  },
  i18n: {defaultLocale: 'en', locales: ['en']},

  presets: [
    [
      'classic',
      /** @type {import('@docusaurus/preset-classic').Options} */
      ({
        docs: {
          path: '../docs',
          routeBasePath: '/',
          sidebarPath: './sidebars.js',
          editUrl: ({docPath}) => `${repo}/edit/master/docs/${docPath}`,
        },
        blog: false,
        theme: {customCss: './src/css/custom.css'},
      }),
    ],
  ],

  themes: [
    [
      '@easyops-cn/docusaurus-search-local',
      {
        hashed: true,
        docsDir: '../docs',
        docsRouteBasePath: '/',
        indexBlog: false,
        indexPages: false,
        highlightSearchTermsOnTargetPage: true,
      },
    ],
  ],

  themeConfig:
    /** @type {import('@docusaurus/preset-classic').ThemeConfig} */
    ({
      colorMode: {respectPrefersColorScheme: true},
      navbar: {
        title: 'PlutoBook for PHP',
        // Plain links that are never marked active: all pages share one sidebar, so
        // "doc" items would all light up on every page, and the search plugin would
        // put the first of them into every result's breadcrumb.
        items: [
          {to: '/install', label: 'Installation', position: 'left', activeBaseRegex: '(?!)'},
          {to: '/guides/html-and-css', label: 'Guides', position: 'left', activeBaseRegex: '(?!)'},
          {to: '/reference/api', label: 'Reference', position: 'left', activeBaseRegex: '(?!)'},
          {href: packagist, label: 'Packagist', position: 'right'},
          {href: repo, label: 'GitHub', position: 'right'},
        ],
      },
      footer: {
        style: 'dark',
        links: [
          {
            title: 'Docs',
            items: [
              {label: 'Installation', to: '/install'},
              {label: 'Guides', to: '/guides/html-and-css'},
              {label: 'API reference', to: '/reference/api'},
            ],
          },
          {
            title: 'Project',
            items: [
              {label: 'GitHub', href: repo},
              {label: 'Releases', href: `${repo}/releases`},
              {label: 'Packagist', href: packagist},
              {label: 'PlutoBook', href: 'https://github.com/plutoprint/plutobook'},
            ],
          },
        ],
        copyright:
          'PlutoBook for PHP is licensed under Apache-2.0. PlutoBook is licensed under MPL-2.0.',
      },
      prism: {
        theme: prismThemes.github,
        darkTheme: prismThemes.dracula,
        additionalLanguages: ['php', 'bash', 'ini'],
      },
    }),
};

export default config;
