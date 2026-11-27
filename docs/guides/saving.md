# Saving the PDF

```php
$pdf = $document->toPdf();               // as a string
$document->writeToFile('/tmp/out.pdf');  // to a file, replacing it
$document->writeToStream($stream);       // to an open PHP stream
```

## Sending the PDF to the browser

Render to a string first, so nothing has been sent yet if rendering fails:

```php
$pdf = $document->toPdf();
header('Content-Type: application/pdf');
header('Content-Disposition: inline; filename="invoice-42.pdf"'); // or attachment
echo $pdf;
```

In Laravel, return `response($pdf, 200, ['Content-Type' => 'application/pdf'])`.
In Symfony, return `new Response($pdf, 200, ['Content-Type' => 'application/pdf'])`.

## Files and streams

`writeToFile()` takes a local path. For anything with `://` in it, such as a
stream wrapper, open the stream yourself and use `writeToStream()`.

`writeToStream()` starts at the stream's current position and leaves the stream
open. Flush or close it yourself, and don't use it elsewhere until the call
returns.

Open files in binary mode. A file opened with `t` in its mode, such as `wt`, is
refused, because Windows would change the line endings inside the PDF.

## Streams with write filters

Write filters, added with `stream_filter_append()` or a `php://filter` URL, hide
failed writes. PHP only reports what the first filter took in, and when the stream
behind the filter refuses data, PHP drops it without an error. A filter can also
hold back its last bytes until you close the stream. When you need to know the
whole PDF arrived, write to a stream without filters, which throws when data is
refused, or change the PDF yourself and check what `fwrite()` returns:

```php
$data = base64_encode($document->toPdf());
if (fwrite($stream, $data) !== strlen($data)) {
    throw new RuntimeException('The PDF was not written completely');
}
```

## Partial output

If rendering fails halfway, the file or stream can hold a partial PDF. To never
serve a broken file, write to a temporary file in the same folder and `rename()`
it when the write succeeds.

Fonts and background images can load while the PDF is written, so check
[`getResourceErrors()`](missing-resources.md) after saving. Rendering also counts
toward PHP's time limit; see [Errors and time limits](errors-and-time-limits.md).
