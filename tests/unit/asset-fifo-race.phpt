--TEST--
An asset swapped for a FIFO after its checks is never opened where /proc is mounted
--SKIPIF--
<?php
if (PHP_OS_FAMILY !== 'Linux') die('skip Linux reopens the inspected file through /proc');
if (!is_dir('/proc/self/fd')) die('skip /proc is not mounted');
exec('command -v mkfifo', $output, $status);
if ($status !== 0) die('skip mkfifo is needed for the racing process');
?>
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require dirname(__DIR__) . '/fixtures/file-url.php';
$dir = __DIR__ . '/asset-fifo-race-' . uniqid();
mkdir("$dir/base", 0777, true);
file_put_contents("$dir/base/regular", base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg=='));
exec('mkfifo ' . escapeshellarg("$dir/base/fifo"));
link("$dir/base/regular", "$dir/base/dot.png");
// Replace dot.png atomically with the FIFO and back, through hard links, as fast as PHP can.
// A replacement that lands between the inspection and the read open used to be opened
// and then rejected as verification-failed.
file_put_contents("$dir/racer.php", <<<'PHP'
<?php
chdir($argv[1] . '/base');
touch('../racing');
while (!file_exists('../stop')) {
    link('fifo', 'next') && rename('next', 'dot.png');
    link('regular', 'next') && rename('next', 'dot.png');
}
touch('../stopped');
PHP);
exec(sprintf('%s -n %s %s > /dev/null 2>&1 &', escapeshellarg(PHP_BINARY),
             escapeshellarg("$dir/racer.php"), escapeshellarg($dir)));
for ($i = 0; $i < 200 && !file_exists("$dir/racing"); $i++) {
    usleep(10000);
}
$doc = new PlutoBook\Document();
$reasons = [];
for ($i = 0; $i < 2000; $i++) {
    $doc->loadHtml('<img src="dot.png">', plutobookTestFileUrl("$dir/base/"));
    $doc->toPdf();
    foreach ($doc->getResourceErrors() as $error) {
        $reasons[$error['reason']] = ($reasons[$error['reason']] ?? 0) + 1;
    }
}
touch("$dir/stop");
for ($i = 0; $i < 500 && !file_exists("$dir/stopped"); $i++) {
    usleep(10000);
}
echo 'racer ran: ', var_export(file_exists("$dir/racing"), true), "\n";
echo 'FIFO seen at the inspection: ', var_export(($reasons['not-regular-file'] ?? 0) > 0, true), "\n";
echo 'replacement opened: ', $reasons['verification-failed'] ?? 0, " times\n";
unset($reasons['not-regular-file'], $reasons['verification-failed']);
echo 'other errors: ', json_encode($reasons), "\n";
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-fifo-race-*') as $dir) {
    touch("$dir/stop");
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
racer ran: true
FIFO seen at the inspection: true
replacement opened: 0 times
other errors: []
