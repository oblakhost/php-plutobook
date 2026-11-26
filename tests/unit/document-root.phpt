--TEST--
documentRoot makes one directory the URL root for local assets, like a web server's document root
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require dirname(__DIR__) . '/fixtures/file-url.php';
use PlutoBook\Document;
$dir = __DIR__ . '/document-root-' . uniqid();
mkdir("$dir/release/public/assets", 0777, true);
mkdir("$dir/release/public/invoices/2026", 0777, true);
mkdir("$dir/outside");
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
file_put_contents("$dir/release/public/assets/dot.png", $png);
file_put_contents("$dir/release/public/invoices/2026/local.png", $png);
file_put_contents("$dir/outside/secret.png", $png);
$windows = PHP_OS_FAMILY === 'Windows';
$served = "$dir/release";
if (!$windows) {
    symlink($served, "$dir/current");
    $served = "$dir/current";
}
$root = plutobookTestFileUrl("$served/public/");
$base = $root . 'invoices/2026/';
$file = plutobookTestFileUrl("$dir/release/public/assets/dot.png");
$fixtureUrl = plutobookTestFileUrl($dir);
$doc = new Document();
$show = function (string $src, ?string $base, ?string $root, ?array $expectedError = null) use ($doc, $fixtureUrl): void {
    $doc->loadHtml("<img src=\"$src\">", $base, documentRoot: $root);
    $loaded = str_contains($doc->toPdf(), '/Subtype /Image');
    $errors = [];
    foreach ($doc->getResourceErrors() as ['reason' => $reason, 'url' => $url]) {
        $errors[] = [$reason, $url] === $expectedError ? 'rejected at expected URL' : "$reason $url";
    }
    $summary = $loaded ? 'loaded' : implode(', ', $errors);
    printf("%-42s %s\n", str_replace($fixtureUrl, 'file://DIR', $src), str_replace($fixtureUrl, 'file://DIR', $summary));
};
echo "-- documentRoot with a baseUrl below it\n";
foreach (['/assets/dot.png', '../../assets/dot.png', 'local.png', '/invoices/2026/local.png',
          '../../../../outside/secret.png', '/assets/..%2F..%2F..%2Foutside/secret.png'] as $src) {
    $show($src, $base, $root);
}
// A literal drive colon is invalid in the virtual path on Windows.
$show($file, $base, $root, [$windows ? 'invalid-url' : 'missing', $file]);
echo "-- documentRoot alone\n";
$show('/assets/dot.png', null, $root);
$show('assets/dot.png', null, $root);
echo "-- baseUrl alone keeps filesystem paths\n";
// A root-relative URL inherits the base drive on Windows. The asset is outside the pinned directory.
$expectedError = $windows
    ? ['outside-base', plutobookTestFileUrl(substr($dir, 0, 2) . '/assets/dot.png')]
    : ['missing', 'file:///assets/dot.png'];
$show('/assets/dot.png', $root, null, $expectedError);
$show($file, $root, null);
echo "-- validation\n";
foreach ([[$fixtureUrl . '/outside/', $root], [$base, $fixtureUrl . '/missing/'], [$base, rtrim($root, '/')]] as [$b, $r]) {
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
    if (is_link("$dir/current")) unlink("$dir/current");
    foreach (['outside/secret.png', 'release/public/assets/dot.png', 'release/public/invoices/2026/local.png'] as $file) {
        if (is_file("$dir/$file")) unlink("$dir/$file");
    }
    foreach (['outside', 'release/public/invoices/2026', 'release/public/invoices', 'release/public/assets',
              'release/public', 'release'] as $folder) {
        if (is_dir("$dir/$folder")) rmdir("$dir/$folder");
    }
    rmdir($dir);
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
file://DIR/release/public/assets/dot.png   rejected at expected URL
-- documentRoot alone
/assets/dot.png                            loaded
assets/dot.png                             loaded
-- baseUrl alone keeps filesystem paths
/assets/dot.png                            rejected at expected URL
file://DIR/release/public/assets/dot.png   loaded
-- validation
PlutoBook\Document::loadHtml(): Argument #2 ($baseUrl) must be a directory inside documentRoot
PlutoBook\Document::loadHtml(): Argument #4 ($documentRoot) must be a file:/// URL ending in / for an accessible local directory
PlutoBook\Document::loadHtml(): Argument #4 ($documentRoot) must be a file:/// URL ending in / for an accessible local directory
