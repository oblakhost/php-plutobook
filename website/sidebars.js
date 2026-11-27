// @ts-check
// Sidebar order lives here, so the Markdown in ../docs needs no front matter.

/** @type {import('@docusaurus/plugin-content-docs').SidebarsConfig} */
const sidebars = {
  docs: [
    {type: 'doc', id: 'index', label: 'Introduction'},
    {
      type: 'category',
      label: 'Installation',
      link: {type: 'doc', id: 'install/index'},
      items: [
        'install/debian-ubuntu',
        'install/rhel',
        'install/other-linux',
        'install/windows',
        'install/from-source',
        'install/troubleshooting',
      ],
    },
    {
      type: 'category',
      label: 'Guides',
      collapsed: false,
      items: [
        'guides/html-and-css',
        'guides/page-setup',
        'guides/saving',
        'guides/local-files',
        'guides/remote-files',
        'guides/missing-resources',
        'guides/errors-and-time-limits',
      ],
    },
    {
      type: 'category',
      label: 'Reference',
      collapsed: false,
      items: ['reference/api', 'reference/configuration', 'reference/resource-errors'],
    },
    'security-model',
    'limitations',
    {
      type: 'link',
      label: 'Contributing',
      href: 'https://github.com/oblakhost/php-plutobook/blob/master/CONTRIBUTING.md',
    },
  ],
};

export default sidebars;
