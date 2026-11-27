# Page setup

## Page size, orientation and margins

```php
$document = new PlutoBook\Document(
    pageSize: 'Letter',        // A3, A4 (default), A5, B4, B5, Letter, Legal or Ledger
    orientation: 'landscape',  // portrait (default) or landscape
    margins: '20mm 15mm',      // like CSS margin: 1 to 4 lengths, default 15mm
);
```

Margins take lengths in `pt`, `mm`, `cm`, `in`, `px` or `pc`, and `0` works
without a unit. An `@page` rule in your CSS can change all of these. PlutoBook uses
the size and margins of the first page for the whole document.

## Headers, footers and page numbers

Put text in the page margins with `@page` margin boxes, and number pages with
`counter(page)`:

```php
$document->loadHtml($html, css: <<<'CSS'
    @page {
        @bottom-center { content: "Page " counter(page); }
    }
    h1 { break-before: page; }
    CSS);
```

- `@bottom-center` puts its content in the middle of the bottom margin. The other
  margin boxes, such as `@top-left` or `@bottom-right`, work the same way.
- `counter(page)` is the current page number.
- `break-before: page` starts each `h1` on a new page.

`@page :first` is the only page selector that works reliably. See
[Limitations](../limitations.md).
