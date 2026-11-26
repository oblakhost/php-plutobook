--TEST--
Local assets and base directories on Windows reject directory junctions
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Windows') die('skip NTFS directory junctions are specific to Windows');
?>
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require dirname(__DIR__) . '/fixtures/file-url.php';
$dir = __DIR__ . '/junctions-' . uniqid();
mkdir("$dir/base/assets", 0777, true);
mkdir("$dir/other");
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
foreach (['assets' => "$dir/base/assets", 'other' => "$dir/other"] as $name => $target) {
    file_put_contents("$target/dot.png", $png);
    $junction = str_replace('/', '\\', "$dir/base/$name-link");
    $target = str_replace('/', '\\', $target);
    exec('mklink /J ' . escapeshellarg($junction) . ' ' . escapeshellarg($target), $output, $status);
    if ($status !== 0) throw new RuntimeException("Could not create $junction");
}
$doc = new PlutoBook\Document();
foreach (['assets', 'assets-link', 'other-link'] as $path) {
    $doc->loadHtml("<img src=\"$path/dot.png\">", plutobookTestFileUrl("$dir/base/"));
    echo "$path: ", str_contains($doc->toPdf(), '/Subtype /Image')
        ? 'loaded' : implode(', ', array_column($doc->getResourceErrors(), 'reason')), "\n";
}
try {
    $doc->loadHtml('base through junction', plutobookTestFileUrl("$dir/base/assets-link/"));
} catch (ValueError $e) {
    echo "junction base rejected\n";
}
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/junctions-*') as $dir) {
    // Remove junction entries before deleting either target.
    foreach (['assets-link', 'other-link'] as $link) {
        if (is_dir("$dir/base/$link")) rmdir("$dir/base/$link");
    }
    foreach (['base/assets', 'other'] as $folder) {
        if (is_file("$dir/$folder/dot.png")) unlink("$dir/$folder/dot.png");
        if (is_dir("$dir/$folder")) rmdir("$dir/$folder");
    }
    if (is_dir("$dir/base")) rmdir("$dir/base");
    rmdir($dir);
}
?>
--EXPECT--
assets: loaded
assets-link: verification-failed
other-link: verification-failed
junction base rejected
