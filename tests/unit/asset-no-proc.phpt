--TEST--
Local assets load when /proc is not mounted, as in an FPM chroot
--SKIPIF--
<?php
if (!is_readable('/proc/self/maps')) {
    die('skip /proc is needed to locate the loaded extension');
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
echo 'proc mounted: ', var_export(file_exists('/proc/self/fd'), true), "\n";
$doc = new PlutoBook\Document();
$doc->loadHtml('<img src="dot.png"><img src="../child.php">', 'file://' . $argv[1] . '/');
echo 'image: ', str_contains($doc->toPdf(), '/Subtype /Image') ? 'loaded' : 'missing', "\n";
echo 'errors: ', json_encode(array_column($doc->getResourceErrors(), 'reason')), "\n";
PHP);
// A private mount namespace hides /proc behind an empty tmpfs for the child only.
$command = ['unshare', '-Urm', 'sh', '-c', 'mount -t tmpfs none /proc && exec "$@"', 'sh',
            PHP_BINARY, '-n', '-d', 'extension=' . $module[1], "$dir/child.php", "$dir/assets"];
passthru(implode(' ', array_map('escapeshellarg', $command)) . ' 2>/dev/null', $status);
echo "exit: $status\n";
?>
--CLEAN--
<?php
foreach (glob(__DIR__ . '/asset-no-proc-*') as $dir) {
    exec('rm -rf ' . escapeshellarg($dir));
}
?>
--EXPECT--
proc mounted: false
image: loaded
errors: ["outside-base"]
exit: 0
