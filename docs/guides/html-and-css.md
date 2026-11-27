# HTML and CSS

`loadHtml()` takes UTF-8 HTML and, optionally, extra CSS. Calling it again
replaces the content. PlutoBook's HTML parser copes with ordinary imperfect markup.

```php
$document->loadHtml(
    '<h1>Invoice 42</h1><p>Total: 120.00 EUR</p>',
    css: 'h1 { color: #194572 }',
);
```

`loadHtml()` takes a string, so any template engine works. Render the template
first, for example with `view('invoice', $data)->render()` in Laravel or
`$twig->render('invoice.html.twig', $data)` with Twig, and pass the result.

The document is laid out for print: `@media print` rules apply and
`@media screen` rules don't.

## Which styles win

The CSS you pass this way beats the document's own `<style>` tags, linked
stylesheets and `style` attributes, however specific their selectors are. The one
exception is the document's `!important` rules, which still beat yours unless you
mark yours `!important` too. (Browsers handle this differently.)

## What CSS works

PlutoBook supports most of CSS 3 and parts of CSS 4, including flexbox, tables,
multiple columns, transforms, custom properties and media queries. JavaScript and
CSS Grid don't work yet. PlutoBook's
[feature list](https://github.com/plutoprint/plutobook/blob/v0.20.0/FEATURES.md)
has the details, and [Limitations](../limitations.md) lists what to avoid.

Images, stylesheets and fonts referenced by the HTML need a folder or a list of
allowed websites; see [Local images, stylesheets and fonts](local-files.md) and
[Images and fonts from the web](remote-files.md).
