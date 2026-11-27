# Missing images and fonts

If an image, stylesheet or font can't be loaded, it is left out and the rest of
the page still renders. `getResourceErrors()` tells you what was skipped:

```php
$document->writeToFile('invoice.pdf');

foreach ($document->getResourceErrors() as $error) {
    // ['url' => 'https://cdn.example/logo.png', 'reason' => 'http-error', 'detail' => 'HTTP 404']
    error_log("{$error['url']}: {$error['reason']} ({$error['detail']})");
}
```

Check the list after rendering, because fonts and background images load while
the PDF is written. Each `loadHtml()` clears the list, and rendering adds to it.
Each resource appears once. To treat missing assets as an error, throw when the
list isn't empty. With a document root, URLs are reported as the document sees
them, for example `file:///assets/missing.png` rather than the path on the server.

`reason` is a stable code you can match on, and `detail` is a human-readable
explanation. [Resource errors](../reference/resource-errors.md) lists every
reason.

## Warnings on stderr

PlutoBook also prints a `WARNING:` line to stderr for each skipped resource, such as
`WARNING: Asset skipped, outside the asset directory: file:///…`. The extension
can't turn this off. Under PHP-FPM, those lines only reach the log when the pool
has `catch_workers_output = yes`.
