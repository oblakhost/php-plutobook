# Local images, stylesheets and fonts

By default a document can only use resources embedded as `data:` URLs. To use
files from disk, tell `loadHtml()` which folder they are in, as an absolute
`file:///` URL that ends with `/`. Files must be inside that folder: a path such as
`../css/print.css` that leads out of it is refused as `outside-base`.

## Relative paths

For relative paths such as `logo.svg` or `css/print.css`, pass the folder as
`baseUrl`. Percent-encode each part of the path, so spaces and `#` in folder names
work:

```php
$assets = realpath(__DIR__ . '/assets');
if ($assets === false) {
    throw new RuntimeException('Missing invoice assets directory');
}
$baseUrl = 'file://' . implode('/', array_map('rawurlencode', explode('/', $assets))) . '/';
$document->loadHtml('<img src="logo.svg"><p>Statement</p>', baseUrl: $baseUrl);
```

On Windows, use a drive URL such as `file:///C:/invoices/assets/`.

## Paths that start with `/`

Templates written for a website often use paths that start with `/`, such as
`/assets/logo.svg`. For those, pass the site's public folder as `documentRoot`. It
then acts as `/`, the way it does on a web server.

You can still pass `baseUrl` to say where relative paths such as `stamp.png`
start. It must be a folder inside the document root; without it, relative paths
start at the document root.

```php
$document->loadHtml(
    '<img src="/assets/logo.svg"><img src="stamp.png">',
    baseUrl: 'file:///srv/shop/public/invoices/',  // stamp.png -> public/invoices/stamp.png
    documentRoot: 'file:///srv/shop/public/',      // /assets/… -> public/assets/…
);
```

With a document root, even full `file:` URLs are read inside it, so
`file:///srv/shop/…` stays within the public folder, and `..` can't climb out. The
same template renders the same way from your local files as from the live site.

## In a framework app

The document root is the app's public folder: `public_path()` in Laravel,
`public/` in Symfony. Turn it into a `file:///` URL as shown above.

Asset helpers such as Laravel's `asset()` print full URLs to your own site, which
turns every image into a download. Print root-relative paths such as
`/images/logo.png` instead, or add your host to `allowedHosts`. Hosts on loopback
or private addresses, such as `localhost`, are refused; see
[Images and fonts from the web](remote-files.md).

## Fonts

Put font files in your `baseUrl` or `documentRoot` folder and load them with
`@font-face`, like any other file:

```css
@font-face { font-family: "Inter"; src: url("fonts/Inter-Regular.ttf"); }
body { font-family: "Inter", sans-serif; }
```

On Linux, fonts installed on the system work too, through Fontconfig. On Windows,
only the DejaVu fonts that come with the extension do; see
[Windows](../install/windows.md#fonts).

[Security](../security-model.md) describes exactly which files the extension will
and won't read.
