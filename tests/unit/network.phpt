--TEST--
Remote stylesheets, fonts and images load from allowed hosts through policy-checked, bounded requests
--SKIPIF--
<?php
require __DIR__ . '/network.inc';
if (!is_readable(plutobook_test_font())) {
    die('skip FONTCONFIG_FILE must point at the pinned DejaVu fonts');
}
?>
--EXTENSIONS--
zlib
--INI--
plutobook.allow_private_addresses=1
plutobook.http_timeout=2
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require dirname(__DIR__) . '/fixtures/file-url.php';
require __DIR__ . '/network.inc';
use PlutoBook\Document;
$port = plutobook_test_server('tcp');
$origin = "http://127.0.0.1:$port";
echo 'ext/curl loaded: ', var_export(extension_loaded('curl'), true), "\n";

echo "-- remote page\n";
// Relative, root-relative and absolute references resolve against the remote baseUrl. The
// stylesheet's font URL is relative to the stylesheet. Its face is DejaVu Sans Bold, while the
// page keeps the heading at normal weight, so "DejaVuSans-Bold" appears only via the web font.
$doc = new Document();
$doc->loadHtml(
    '<style>h1 { font-weight: normal }</style><link rel="stylesheet" href="../../style.css">'
    . '<h1>Remote font</h1><p>Blue text</p>'
    . "<img src=\"/img/dot.png\"><img src=\"$origin/img/logo.svg\">",
    "$origin/invoices/42/",
    allowedHosts: ['127.0.0.1'],
);
$pdf = $doc->toPdf();
echo 'errors: ', json_encode($doc->getResourceErrors()), "\n";
echo 'PNG embedded: ', var_export(str_contains($pdf, '/Subtype /Image'), true), "\n";
echo 'stylesheet font embedded: ', var_export(str_contains($pdf, 'DejaVuSans-Bold'), true), "\n";
echo 'fills: ', implode(', ', plutobook_test_fills($pdf)), "\n";

echo "-- policy\n";
$local = ['127.0.0.1'];
$cases = [
    'redirect to an allowed path' => ["$origin/redirect/dot.png", $local],
    'HTTP error' => ["$origin/missing.png", $local],
    'redirect to another host' => ["$origin/redirect/away", $local],
    'redirect loop' => ["$origin/redirect/loop", $local],
    'redirect to ftp:' => ["$origin/redirect/ftp", $local],
    'empty body' => ["$origin/empty.png", $local],
    'host not listed' => ["http://localhost:$port/img/dot.png", $local],
    'wildcard is subdomains only' => ["http://localhost:$port/img/dot.png", ['*.localhost']],
    'any host' => ["http://localhost:$port/img/dot.png", ['*']],
    'network off' => ["$origin/img/dot.png", []],
];
foreach ($cases as $label => [$src, $hosts]) {
    echo str_pad($label, 28), plutobook_test_image($doc, $src, $hosts), "\n";
}
echo str_pad('remote base, no hosts', 28), plutobook_test_image($doc, 'img/dot.png', [], "$origin/"), "\n";

echo "-- limits\n";
ini_set('plutobook.max_asset_size', '1K');
echo str_pad('Content-Length too large', 28), plutobook_test_image($doc, "$origin/big.png", $local, detail: false), "\n";
echo str_pad('chunked body too large', 28), plutobook_test_image($doc, "$origin/chunked.png", $local, detail: false), "\n";
ini_restore('plutobook.max_asset_size');
ini_set('plutobook.http_timeout', '1');
echo str_pad('slow response', 28), plutobook_test_image($doc, "$origin/slow.png", $local, detail: false), "\n";

echo "-- validation\n";
$invalid = [
    [['allowedHosts' => [42]]],
    [['allowedHosts' => ['bad host']]],
    [['allowedHosts' => ['*.']]],
    [['baseUrl' => 'https://']],
    [['baseUrl' => "$origin/", 'documentRoot' => plutobookTestFileUrl(__DIR__ . '/')]],
];
foreach ($invalid as [$arguments]) {
    try {
        $doc->loadHtml('text', ...$arguments);
        echo "accepted\n";
    } catch (TypeError | ValueError $e) {
        echo get_class($e), ': ', $e->getMessage(), "\n";
    }
}
?>
--EXPECTF--
ext/curl loaded: false
-- remote page
errors: []
PNG embedded: true
stylesheet font embedded: true
fills: 0 0 0, 0 0 1, 0 1 0
-- policy
redirect to an allowed path loaded
HTTP error                  http-error (HTTP 404)
redirect to another host    host-not-allowed (redirect target localhost is not in allowedHosts)
redirect loop               redirect-error (more than 5 redirects)
redirect to ftp:            redirect-error (redirect to a ftp: URL)
empty body                  empty (empty file)
host not listed             host-not-allowed (localhost is not in allowedHosts)
wildcard is subdomains only host-not-allowed (localhost is not in allowedHosts)
any host                    loaded
network off                 network-disabled (network access needs allowedHosts)
remote base, no hosts       network-disabled (network access needs allowedHosts)
-- limits
Content-Length too large    too-large
chunked body too large      too-large
slow response               timeout
-- validation
TypeError: PlutoBook\Document::loadHtml(): Argument #5 ($allowedHosts) must contain only strings, int given
ValueError: PlutoBook\Document::loadHtml(): Argument #5 ($allowedHosts) must contain host names such as example.com, *.example.com, [::1] or *
ValueError: PlutoBook\Document::loadHtml(): Argument #5 ($allowedHosts) must contain host names such as example.com, *.example.com, [::1] or *
ValueError: PlutoBook\Document::loadHtml(): Argument #2 ($baseUrl) must be a valid http:// or https:// URL with a host
ValueError: PlutoBook\Document::loadHtml(): Argument #2 ($baseUrl) must be a file:/// URL when documentRoot is given
