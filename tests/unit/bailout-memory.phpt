--TEST--
A memory_limit fatal error while buffering PDF output unwinds the renderer before PHP bails out
--INI--
memory_limit=128M
--FILE--
<?php
$small = new PlutoBook\Document();
$small->loadHtml('<p>small</p>');
// Keep the 13 MB HTML string alive. PHP kills the process outright once usage passes twice the
// limit (memory_hard_limit_percent), so the limit must start well above the buffer's growth.
$html = str_repeat('<p>' . str_repeat('Lorem ipsum dolor sit amet ', 40) . '</p>', 12000);
$large = new PlutoBook\Document();
$large->loadHtml($html);
register_shutdown_function(function () use ($small): void {
    echo "shutdown function ran, other document renders: ",
        var_export(str_starts_with($small->toPdf(), '%PDF-'), true), "\n";
});
// The PDF is several megabytes, so its buffer outgrows a limit just above current usage.
ini_set('memory_limit', (string) (memory_get_usage(true) + 64 * 1024));
$large->toPdf();
echo "not reached\n";
?>
--EXPECTF--
Fatal error: Allowed memory size of %d bytes exhausted%s(tried to allocate %d bytes) in %s on line %d
shutdown function ran, other document renders: true
