--TEST--
Documents release their descriptors: asset directories, output files and libcurl handles
--SKIPIF--
<?php
if (!is_dir('/proc/self/fd')) {
    die('skip /proc/self/fd is needed to count open descriptors');
}
?>
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require dirname(__DIR__) . '/fixtures/file-url.php';
$dir = __DIR__ . '/lifecycle-' . uniqid();
mkdir($dir);
file_put_contents("$dir/dot.png", base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg=='));
$open = static fn (): int => count(scandir('/proc/self/fd')) - 2;
$cycle = static function (int $times) use ($dir): void {
    for ($i = 0; $i < $times; $i++) {
        $doc = new PlutoBook\Document();
        $doc->loadHtml('<img src="dot.png"><img src="missing.png">', plutobookTestFileUrl("$dir/"));
        // Reloading replaces the pinned asset directory.
        $doc->loadHtml('<img src="/dot.png">', documentRoot: plutobookTestFileUrl("$dir/"));
        $doc->writeToFile("$dir/out.pdf");
        // Creates the document's libcurl handle; the internal address is refused before connecting.
        $doc->loadHtml('<img src="http://127.0.0.1:9/x.png">', allowedHosts: ['*']);
        $doc->toPdf();
    }
};
$cycle(5);
$before = $open();
$cycle(50);
echo 'open descriptors changed by: ', $open() - $before, "\n";
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/lifecycle-*') as $dir) {
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
open descriptors changed by: 0
