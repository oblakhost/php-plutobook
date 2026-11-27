--TEST--
Relative links in a redirected stylesheet resolve against the URL PlutoBook asked for
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
require __DIR__ . '/network.inc';
$port = plutobook_test_server('tcp');
$origin = "http://127.0.0.1:$port";
// PlutoBook 0.20.0 resolves a stylesheet's links against the URL it requested: its
// resource API has no field for the address a redirect led to. /redirect/style.css
// redirects to /style.css, whose font is fonts/face.ttf, so PlutoBook asks for
// /redirect/fonts/face.ttf. README: "Downloads". If this starts loading the font,
// PlutoBook has learned the final URL and the README note can go.
$doc = new PlutoBook\Document();
$doc->loadHtml(
    "<link rel=\"stylesheet\" href=\"$origin/redirect/style.css\"><h1>Remote font</h1><p>Blue text</p>",
    allowedHosts: ['127.0.0.1'],
);
$pdf = $doc->toPdf();
echo 'stylesheet applied: ', var_export(in_array('0 0 1', plutobook_test_fills($pdf), true), true), "\n";
echo 'stylesheet font embedded: ', var_export(str_contains($pdf, 'DejaVuSans-Bold'), true), "\n";
foreach ($doc->getResourceErrors() as $error) {
    echo str_replace($origin, 'ORIGIN', $error['url']), ': ', $error['reason'], ' (', $error['detail'], ")\n";
}
?>
--EXPECT--
stylesheet applied: true
stylesheet font embedded: false
ORIGIN/redirect/fonts/face.ttf: http-error (HTTP 404)
