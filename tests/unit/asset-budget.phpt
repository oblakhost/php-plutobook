--TEST--
plutobook.max_total_asset_size bounds the asset bytes one document loads, per load
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
use PlutoBook\Document;
$dir = __DIR__ . '/asset-budget-' . uniqid();
mkdir($dir);
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
foreach (['a', 'b', 'c'] as $name) {
    file_put_contents("$dir/$name.png", $png);
}
$size = strlen($png);
$doc = new Document();
$skipped = function () use ($doc, $dir): string {
    // Images are fetched in document order while the renderer builds its boxes.
    $doc->loadHtml('<img src="a.png"><img src="b.png"><img src="c.png">', "file://$dir/");
    $doc->toPdf();
    $errors = array_map(fn (array $e): string => basename($e['url']) . " $e[reason] ($e[detail])", $doc->getResourceErrors());
    return $errors ? implode(', ', $errors) : 'none';
};
echo 'default: ', ini_get('plutobook.max_total_asset_size'), "\n";
ini_set('plutobook.max_total_asset_size', (string) (2 * $size));
echo 'room for two: ', $skipped(), "\n";
echo 'reload resets the budget: ', $skipped(), "\n";
ini_set('plutobook.max_total_asset_size', '0');
echo 'no cap: ', $skipped(), "\n";
foreach (['-1', 'abc'] as $invalid) {
    var_dump(ini_set('plutobook.max_total_asset_size', $invalid));
}
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-budget-*') as $dir) {
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECTF--
default: 128M
room for two: c.png too-large (plutobook.max_total_asset_size reached)
reload resets the budget: c.png too-large (plutobook.max_total_asset_size reached)
no cap: none

Warning: Invalid "plutobook.max_total_asset_size" setting. Expected 0 or a byte size such as 128M in %s on line %d
bool(false)

Warning: Invalid "plutobook.max_total_asset_size" setting. Expected 0 or a byte size such as 128M in %s on line %d
bool(false)
