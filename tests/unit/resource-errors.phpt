--TEST--
Skipped resources are reported per document with stable reasons
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
use PlutoBook\Document;
$dir = __DIR__ . '/resource-errors-' . uniqid();
mkdir("$dir/base/folder", 0777, true);
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
file_put_contents("$dir/base/dot.png", $png);
file_put_contents("$dir/base/empty.css", '');
file_put_contents("$dir/outside.png", $png);
$base = "file://$dir/base/";
$doc = new Document();
var_dump($doc->getResourceErrors());
// PlutoBook fetches stylesheets while parsing and images while building boxes; order is not API.
function report(Document $doc, string $html, ?string $base, string $dir): void {
    $doc->loadHtml($html, $base);
    $doc->toPdf();
    $lines = [];
    foreach ($doc->getResourceErrors() as ['url' => $url, 'reason' => $reason]) {
        $lines[] = str_pad($reason, 19) . str_replace("file://$dir", '', $url);
    }
    sort($lines);
    echo implode("\n", [...$lines, '--']), "\n";
}
$data = 'data:image/png;base64,' . base64_encode($png);
report($doc, '<img src="dot.png"><img src="' . $data . '">', $base, $dir);
report($doc, '<img src="missing.png"><img src="missing.png"><img src="../outside.png">'
    . '<img src="folder"><link rel="stylesheet" href="empty.css"><img src="dot%00.png">'
    . '<img src="https://example.invalid/a.png"><img src="ftp://example.invalid/b.png">', $base, $dir);
report($doc, '<img src="dot.png">', null, $dir);
ini_set('plutobook.max_asset_size', '10');
report($doc, '<img src="dot.png">', $base, $dir);
ini_restore('plutobook.max_asset_size');
report($doc, '<img src="dot.png">', $base, $dir);
ini_set('open_basedir', "$dir/base");
report($doc, '<img src="../outside.png">', $base, $dir);
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/resource-errors-*') as $dir) {
    foreach (["$dir/base/dot.png", "$dir/base/empty.css", "$dir/outside.png"] as $file) {
        if (is_file($file)) unlink($file);
    }
    if (is_dir("$dir/base/folder")) rmdir("$dir/base/folder");
    if (is_dir("$dir/base")) rmdir("$dir/base");
    rmdir($dir);
}
?>
--EXPECT--
array(0) {
}
--
empty              /base/empty.css
invalid-url        /base/dot%00.png
missing            /base/missing.png
network-disabled   https://example.invalid/a.png
not-regular-file   /base/folder
outside-base       /outside.png
unsupported-scheme ftp://example.invalid/b.png
--
no-base-url        file:///dot.png
--
too-large          /base/dot.png
--
--
open-basedir       /outside.png
--
