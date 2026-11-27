--TEST--
Local assets load when /proc is not mounted, as in an FPM chroot, and a fake /proc is not trusted
--SKIPIF--
<?php
if (!is_readable('/proc/self/maps')) {
    die('skip /proc is needed to locate the loaded extension');
}
// Sanitizer runtimes read their options and run leak checks through /proc.
if (getenv('SKIP_ASAN')) {
    die('skip AddressSanitizer cannot run without /proc');
}
exec('unshare -Urm sh -c "mount -t tmpfs none /proc" 2>&1', $output, $status);
if ($status !== 0) {
    die('skip unprivileged user and mount namespaces are unavailable');
}
?>
--FILE--
<?php
// The parent still has /proc; use it once to find the module this test loaded.
if (!preg_match('~\s(/\S+/plutobook\.so)$~m', file_get_contents('/proc/self/maps'), $module)) {
    exit("module path not found\n");
}
$dir = __DIR__ . '/asset-no-proc-' . uniqid();
mkdir("$dir/assets", 0777, true);
file_put_contents("$dir/assets/dot.png", base64_decode('iVBORw0KGgoAAAANSUhEUgAAAAMAAAACCAIAAAASFvFNAAAAEElEQVR4nGNwmCAAQQxwFgA1tAVBkfyF3QAAAABJRU5ErkJggg=='));
file_put_contents("$dir/child.php", <<<'PHP'
<?php
echo 'proc mounted: ', var_export(file_exists('/proc/self/maps'), true), "\n";
$doc = new PlutoBook\Document();
$doc->loadHtml('<img src="dot.png"><img src="../child.php">', 'file://' . $argv[1] . '/');
echo 'image: ', str_contains($doc->toPdf(), '/Subtype /Image') ? 'loaded' : 'missing', "\n";
echo 'errors: ', json_encode(array_column($doc->getResourceErrors(), 'reason')), "\n";
PHP);
// A private mount namespace hides /proc behind a tmpfs for the child only. The second setup
// plants /proc/self/fd entries that all lead outside the asset folder. The loader reopens
// files through procfs only, so it falls back to opening them by name in both setups.
$setups = [
    'empty /proc' => 'mount -t tmpfs none /proc',
    'fake /proc/self/fd' => 'mount -t tmpfs none /proc && mkdir -p /proc/self/fd && i=0 && '
        . 'while [ $i -lt 64 ]; do ln -s ' . escapeshellarg("$dir/child.php") . ' /proc/self/fd/$i; i=$((i+1)); done',
];
foreach ($setups as $label => $setup) {
    echo "-- $label\n";
    $command = ['unshare', '-Urm', 'sh', '-c', $setup . ' && exec "$@"', 'sh',
                PHP_BINARY, '-n', '-d', 'extension=' . $module[1], "$dir/child.php", "$dir/assets"];
    passthru(implode(' ', array_map('escapeshellarg', $command)) . ' 2>/dev/null', $status);
    echo "exit: $status\n";
}
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-no-proc-*') as $dir) {
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
-- empty /proc
proc mounted: false
image: loaded
errors: ["outside-base"]
exit: 0
-- fake /proc/self/fd
proc mounted: false
image: loaded
errors: ["outside-base"]
exit: 0
