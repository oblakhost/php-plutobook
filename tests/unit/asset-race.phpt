--TEST--
Swapping an asset directory for a symlink while rendering never exposes files outside the base
--SKIPIF--
<?php
if (!is_executable('/bin/sh')) {
    die('skip a shell is needed for the racing process');
}
?>
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
$dir = __DIR__ . '/asset-race-' . uniqid();
mkdir("$dir/base/sub", 0777, true);
mkdir("$dir/outside/sub", 0777, true);
// The inside image is 3x2 and the outside one 1x1, so the embedded width tells them apart.
file_put_contents("$dir/base/sub/dot.png", base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg=='));
file_put_contents("$dir/outside/sub/dot.png", base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8DwHwAFBQIAX8jx0gAAAABJRU5ErkJggg=='));
file_put_contents("$dir/racer.sh", <<<'SH'
cd "$1/base" || exit 1
: > "$1/racing"
while [ ! -e "$1/stop" ]; do
  mv sub sub.real && ln -s ../outside/sub sub && rm sub && mv sub.real sub
done
: > "$1/stopped"
SH);
exec(sprintf('sh %s %s > /dev/null 2>&1 &', escapeshellarg("$dir/racer.sh"), escapeshellarg($dir)));
for ($i = 0; $i < 200 && !file_exists("$dir/racing"); $i++) {
    usleep(10000);
}
$doc = new PlutoBook\Document();
$outside = 0;
for ($i = 0; $i < 500; $i++) {
    $doc->loadHtml('<img src="sub/dot.png">', "file://$dir/base/");
    if (str_contains($doc->toPdf(), '/Width 1')) {
        $outside++;
    }
}
touch("$dir/stop");
for ($i = 0; $i < 500 && !file_exists("$dir/stopped"); $i++) {
    usleep(10000);
}
echo 'racer ran: ', var_export(file_exists("$dir/racing"), true), "\n";
echo "outside file loaded: $outside times\n";
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-race-*') as $dir) {
    touch("$dir/stop");
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
racer ran: true
outside file loaded: 0 times
