--TEST--
documentRoot makes one directory the URL root for local assets, like a web server's document root
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
use PlutoBook\Document;
$dir = __DIR__ . '/document-root-' . uniqid();
mkdir("$dir/release/public/assets", 0777, true);
mkdir("$dir/release/public/invoices/2026", 0777, true);
mkdir("$dir/outside");
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
file_put_contents("$dir/release/public/assets/dot.png", $png);
file_put_contents("$dir/release/public/invoices/2026/local.png", $png);
file_put_contents("$dir/outside/secret.png", $png);
symlink("$dir/release", "$dir/current");
$root = "file://$dir/current/public/";
$base = "file://$dir/current/public/invoices/2026/";
$doc = new Document();
$show = function (string $src, ?string $base, ?string $root) use ($doc, $dir): void {
    $doc->loadHtml("<img src=\"$src\">", $base, documentRoot: $root);
    $loaded = str_contains($doc->toPdf(), '/Subtype /Image');
    $errors = array_map(fn ($e) => "$e[reason] $e[url]", $doc->getResourceErrors());
    $line = sprintf("%-42s %s\n", str_replace($dir, 'DIR', $src), $loaded ? 'loaded' : implode(', ', $errors));
    echo str_replace($dir, 'DIR', $line);
};
echo "-- documentRoot with a baseUrl below it\n";
foreach (['/assets/dot.png', '../../assets/dot.png', 'local.png', '/invoices/2026/local.png',
          '../../../../outside/secret.png', '/assets/..%2F..%2F..%2Foutside/secret.png',
          "file://$dir/release/public/assets/dot.png"] as $src) {
    $show($src, $base, $root);
}
echo "-- documentRoot alone\n";
$show('/assets/dot.png', null, $root);
$show('assets/dot.png', null, $root);
echo "-- baseUrl alone keeps filesystem paths\n";
$show('/assets/dot.png', "file://$dir/current/public/", null);
$show("file://$dir/release/public/assets/dot.png", "file://$dir/current/public/", null);
echo "-- validation\n";
foreach ([["file://$dir/outside/", $root], [$base, "file://$dir/missing/"], [$base, "file://$dir/current/public"]] as [$b, $r]) {
    try {
        $doc->loadHtml('text', $b, documentRoot: $r);
        echo "accepted\n";
    } catch (ValueError $e) {
        echo $e->getMessage(), "\n";
    }
}
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/document-root-*') as $dir) {
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
-- documentRoot with a baseUrl below it
/assets/dot.png                            loaded
../../assets/dot.png                       loaded
local.png                                  loaded
/invoices/2026/local.png                   loaded
../../../../outside/secret.png             missing file:///outside/secret.png
/assets/..%2F..%2F..%2Foutside/secret.png  outside-base file:///assets/..%2F..%2F..%2Foutside/secret.png
file://DIR/release/public/assets/dot.png   missing file://DIR/release/public/assets/dot.png
-- documentRoot alone
/assets/dot.png                            loaded
assets/dot.png                             loaded
-- baseUrl alone keeps filesystem paths
/assets/dot.png                            missing file:///assets/dot.png
file://DIR/release/public/assets/dot.png   loaded
-- validation
PlutoBook\Document::loadHtml(): Argument #2 ($baseUrl) must be a directory inside documentRoot
PlutoBook\Document::loadHtml(): Argument #4 ($documentRoot) must be a file:/// URL ending in / for an accessible local directory
PlutoBook\Document::loadHtml(): Argument #4 ($documentRoot) must be a file:/// URL ending in / for an accessible local directory
