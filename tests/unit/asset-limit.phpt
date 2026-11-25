--TEST--
plutobook.max_asset_size caps local asset reads and rejects invalid settings
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
use PlutoBook\Document;
$dir = __DIR__ . '/asset-limit-' . uniqid();
mkdir($dir);
$png = base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg==');
file_put_contents("$dir/dot.png", $png);
$size = strlen($png);
$doc = new Document();
$image = function () use ($doc, $dir): string {
    $doc->loadHtml('<img src="dot.png">', "file://$dir/");
    return str_contains($doc->toPdf(), '/Subtype /Image') ? 'loaded' : 'skipped';
};
$entry = ini_get_all('plutobook')['plutobook.max_asset_size'];
echo 'default: ', $entry['global_value'], ', runtime changeable: ', var_export($entry['access'] === INI_ALL, true), "\n";
echo 'default limit: ', $image(), "\n";
$limits = ['one byte below' => (string) ($size - 1), 'exact size' => (string) $size, 'no cap' => '0', 'shorthand' => '1K'];
foreach ($limits as $label => $limit) {
    ini_set('plutobook.max_asset_size', $limit);
    echo "$label: ", $image(), "\n";
}
foreach (['-1', '', ' ', 'abc', '1.5M', '2G'] as $invalid) {
    var_dump(ini_set('plutobook.max_asset_size', $invalid));
}
var_dump(ini_get('plutobook.max_asset_size'));
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-limit-*') as $dir) {
    if (is_file("$dir/dot.png")) unlink("$dir/dot.png");
    rmdir($dir);
}
?>
--EXPECTF--
default: 32M, runtime changeable: true
default limit: loaded
one byte below: skipped
exact size: loaded
no cap: loaded
shorthand: loaded

Warning: Invalid "plutobook.max_asset_size" setting. Expected 0 or a byte size up to 2147483647, such as 32M in %s on line %d
bool(false)

Warning: Invalid "plutobook.max_asset_size" setting. Expected 0 or a byte size up to 2147483647, such as 32M in %s on line %d
bool(false)

Warning: Invalid "plutobook.max_asset_size" setting. Expected 0 or a byte size up to 2147483647, such as 32M in %s on line %d
bool(false)

Warning: Invalid "plutobook.max_asset_size" setting. Expected 0 or a byte size up to 2147483647, such as 32M in %s on line %d
bool(false)

Warning: Invalid "plutobook.max_asset_size" setting. Expected 0 or a byte size up to 2147483647, such as 32M in %s on line %d
bool(false)

Warning: Invalid "plutobook.max_asset_size" setting. Expected 0 or a byte size up to 2147483647, such as 32M in %s on line %d
bool(false)
string(2) "1K"
