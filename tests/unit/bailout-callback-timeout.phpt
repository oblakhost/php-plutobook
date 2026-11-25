--TEST--
A time limit reached in a stream wrapper's PHP code during rendering ends in the normal fatal error
--FILE--
<?php
class SlowSink {
    public $context;
    public function stream_open($path, $mode, $options, &$opened) { return true; }
    public function stream_write($data) {
        // PHP code runs here, so the VM raises the time limit inside the renderer's callback.
        set_time_limit(1);
        for ($i = 0; ; $i++) {
        }
    }
}
stream_wrapper_register('slowsink', SlowSink::class);
$doc = new PlutoBook\Document();
$doc->loadHtml('<p>slow sink</p>');
register_shutdown_function(function () use ($doc): void {
    echo "shutdown function ran, document still renders: ",
        var_export(str_starts_with($doc->toPdf(), '%PDF-'), true), "\n";
});
$doc->writeToStream(fopen('slowsink://x', 'wb'));
echo "not reached\n";
?>
--EXPECTF--
Fatal error: Maximum execution time of 1 second exceeded in %s on line %d
shutdown function ran, document still renders: true
