--TEST--
File resources require an accessible directory base, honor its boundary and open_basedir
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require dirname(__DIR__) . '/fixtures/file-url.php';
use PlutoBook\Document;
$dir = __DIR__ . '/assets-' . uniqid();
mkdir($dir);
mkdir("$dir/base");
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
file_put_contents("$dir/base/dot 1.png", $png);
file_put_contents("$dir/outside.png", $png);
$base = plutobookTestFileUrl("$dir/base/");
$doc = new Document();
foreach (['ftp://example.invalid/', 'relative', $base . 'dot%201.png', $base . 'absent/',
          $base . '?query/', $base . "%00/"] as $bad) {
    try { $doc->loadHtml('text', $bad); } catch (ValueError $e) { echo "invalid base\n"; }
}
try { $doc->loadHtml('text', css: "p{}\0"); } catch (ValueError $e) { echo "invalid CSS\n"; }
function imageLoaded($doc, $url, $base) {
    $doc->loadHtml('<p>dot</p><img src="' . $url . '">', $base);
    return str_contains($doc->toPdf(), '/Subtype /Image');
}
var_dump(imageLoaded($doc, 'dot%201.png', $base));
var_dump(imageLoaded($doc, '../outside.png', $base));
var_dump(imageLoaded($doc, 'dot%201.png', null));
var_dump(imageLoaded($doc, 'https://example.invalid/dot.png', $base));
var_dump(imageLoaded($doc, 'missing.png', $base));
var_dump(imageLoaded($doc, 'data:image/png;base64,' . base64_encode($png), null));
unlink("$dir/outside.png");
ini_set('open_basedir', "$dir/base");
try { $doc->loadHtml('hi', plutobookTestFileUrl("$dir/")); } catch (ValueError $e) { echo "restricted base\n"; }
try { $doc->writeToFile("$dir/out.pdf"); } catch (PlutoBook\Exception $e) { echo "restricted output\n"; }
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/assets-*') as $dir) {
    if (is_file("$dir/base/dot 1.png")) unlink("$dir/base/dot 1.png");
    if (is_dir("$dir/base")) rmdir("$dir/base");
    rmdir($dir);
}
?>
--EXPECT--
invalid base
invalid base
invalid base
invalid base
invalid base
invalid base
invalid CSS
bool(true)
bool(false)
bool(false)
bool(false)
bool(false)
bool(true)
restricted base
restricted output
