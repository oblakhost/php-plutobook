--TEST--
Asset symlinks may stay inside the base tree; escapes and FIFOs are refused without blocking
--SKIPIF--
<?php
foreach (['/usr/bin/mkfifo', '/bin/mkfifo'] as $tool) {
    if (is_executable($tool)) exit;
}
echo 'skip mkfifo is unavailable';
?>
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
use PlutoBook\Document;
$dir = __DIR__ . '/asset-symlinks-' . uniqid();
mkdir("$dir/release/assets/img", 0777, true);
mkdir("$dir/secret");
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
file_put_contents("$dir/release/assets/img/dot.png", $png);
file_put_contents("$dir/secret/key.png", $png);
symlink("$dir/release", "$dir/current");
symlink('img/dot.png', "$dir/release/assets/relative.png");
symlink("$dir/release/assets/img/dot.png", "$dir/release/assets/absolute.png");
symlink("$dir/secret/key.png", "$dir/release/assets/escape.png");
symlink("$dir/secret", "$dir/release/assets/escape-dir");
exec('mkfifo ' . escapeshellarg("$dir/release/assets/pipe.png"), $output, $status);
if ($status !== 0) {
    exit("mkfifo failed\n");
}
// The base itself goes through a deploy-style symlink: current -> release.
$doc = new Document();
foreach (['img/dot.png', 'relative.png', 'absolute.png', 'escape.png', 'escape-dir/key.png', 'pipe.png'] as $src) {
    $doc->loadHtml("<img src=\"$src\">", "file://$dir/current/assets/");
    $loaded = str_contains($doc->toPdf(), '/Subtype /Image');
    printf("%-19s %s\n", $src, $loaded ? 'loaded' : implode(',', array_column($doc->getResourceErrors(), 'reason')));
}
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-symlinks-*') as $dir) {
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
img/dot.png         loaded
relative.png        loaded
absolute.png        loaded
escape.png          outside-base
escape-dir/key.png  outside-base
pipe.png            not-regular-file
