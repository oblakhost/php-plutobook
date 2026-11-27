# PlutoBook for PHP

**Make PDFs from HTML and CSS, right inside PHP.**

Write your invoice, report or ticket as an HTML page, and this extension turns it
into a PDF. It runs inside PHP, so it works in your web app, CLI scripts and queue
workers without a headless browser, a separate PDF service or shell commands.

It is built on [PlutoBook](https://github.com/plutoprint/plutobook), a lightweight
engine made for laying out pages for print. It is not a browser engine like
Chromium or WebKit.

```php
$document = new PlutoBook\Document(pageSize: 'A4', margins: '15mm');
$document->loadHtml(
    '<h1>Invoice 42</h1><p>Total: 120.00 EUR</p>',
    css: 'h1 { color: #194572 }',
);
$document->writeToFile('invoice-42.pdf');
```

## Features

- **Runs inside PHP.** Install the extension and render. Nothing else to run or call.
- **CSS for paper.** Page sizes and margins, running headers and footers, page
  numbers, page breaks, flexbox, tables, multiple columns and SVG. See
  [Page setup](guides/page-setup.md).
- **Any language.** Right-to-left and complex scripts such as Arabic, Hebrew and
  Hindi, plus emoji.
- **Your images and fonts.** PNG, JPEG, WebP and SVG images, stylesheets and web
  fonts, from [folders you choose](guides/local-files.md) or
  [websites you allow](guides/remote-files.md).
- **Safe by default.** No network access, and no files outside the folders you
  pass in, unless you allow it. See [Security](security-model.md).
- **Tells you what's missing.** `getResourceErrors()` lists every image, font or
  stylesheet that couldn't be loaded, and why. See
  [Missing images and fonts](guides/missing-resources.md).
- **Ready-made builds.** For PHP 8.2 to 8.6 on Linux (x86_64 and arm64) and Windows
  (x64), plus deb and rpm packages. See [Installation](install/index.md).

## Get started

1. [Install the extension](install/index.md). On most systems that is
   `pie install oblakhost/ext-plutobook`.
2. [Add your HTML and CSS](guides/html-and-css.md), from any template engine, and
   [set up your pages](guides/page-setup.md).
3. [Save the PDF](guides/saving.md) as a string, a file or a stream, or
   [send it to the browser](guides/saving.md#sending-the-pdf-to-the-browser).

The whole API is one class and one exception: see the
[API reference](reference/api.md).
