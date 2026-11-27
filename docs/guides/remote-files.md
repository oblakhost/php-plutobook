# Images and fonts from the web

Nothing is downloaded unless you list the hosts a document may contact:

```php
$document->loadHtml(
    $html,
    baseUrl: 'https://shop.example/invoices/42/',
    allowedHosts: ['shop.example', 'fonts.gstatic.com', '*.cdn.example'],
);
```

## Host names

An entry is a host name, a `*.example.com` pattern for its subdomains (not
`example.com` itself), a bracketed IPv6 address such as `[2001:db8::1]`, or `*`
for any host. Write international domain names in their ASCII (punycode) form.

Google Fonts needs two hosts: `fonts.googleapis.com` for the stylesheet and
`fonts.gstatic.com` for the font files.

## A web page as the base URL

If `baseUrl` is a web address, relative paths are downloaded from that site, the
way a browser resolves them, and local files can't be used, so you can't also pass
`documentRoot`. You can still use `allowedHosts` with a local `baseUrl` or
`documentRoot` for absolute `https://` links.

## Redirected stylesheets

A stylesheet or SVG that redirects keeps the address you linked. PlutoBook looks
up its relative links, such as `url(fonts/face.ttf)` or `@import`, next to that
address, not next to the one it redirected to. Link to the final address, or use
absolute URLs inside files that get redirected.

## What else to know

- The extension downloads with libcurl directly and doesn't need PHP's `curl`
  extension.
- Downloads have time and size limits; see
  [Configuration](../reference/configuration.md).
- [Security](../security-model.md) lists what the extension refuses to fetch, such as
  internal network addresses.
