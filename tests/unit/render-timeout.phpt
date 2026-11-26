--TEST--
A PHP time limit reached during native PDF output stops the renderer and ends in the normal fatal error
--SKIPIF--
<?php
if (PHP_OS_FAMILY === 'Windows') die('skip SIGPROF is a POSIX signal');
if (!is_executable('/bin/sh')) {
    die('skip a shell is needed to deliver SIGPROF');
}
?>
--FILE--
<?php
$doc = new PlutoBook\Document();
$doc->loadHtml(str_repeat('<p>' . str_repeat('Invoice line item description text ', 12) . '</p>', 10000));
$doc->toPdf(); // Lay out once, so the timed call below spends its time producing PDF output.
$stream = fopen('php://temp', 'w+b');
register_shutdown_function(function () use ($stream) {
    rewind($stream);
    $pdf = stream_get_contents($stream);
    echo "shutdown function ran, complete PDF: ", var_export(str_ends_with(trim($pdf), '%%EOF'), true), "\n";
});
// set_time_limit() installs PHP's SIGPROF timeout handler. Deliver that signal shortly after
// output starts, exactly as the CPU timer would, instead of burning a full second of CPU.
set_time_limit(100);
exec(sprintf('(sleep 0.05; kill -s PROF %d) > /dev/null 2>&1 &', getmypid()));
$doc->writeToStream($stream);
echo "not reached\n";
?>
--EXPECTF--
Fatal error: Maximum execution time of 100 seconds exceeded in %s on line %d
shutdown function ran, complete PDF: false
