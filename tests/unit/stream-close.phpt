--TEST--
A borrowed stream closed during rendering is freed after the renderer returns; fclose() is refused
--FILE--
<?php
class Wrapper {
    public $context;
    public static $resource;
    public static string $mode = '';
    public static int $writes = 0;
    public function stream_open($path, $mode, $options, &$opened) { return true; }
    public function stream_write($data) {
        if (++self::$writes === 3) {
            // pclose() closes any stream resource; fclose() honours PHP_STREAM_FLAG_NO_FCLOSE.
            echo self::$mode, ' inside the callback: ', var_export(@(self::$mode)(self::$resource), true), "\n";
        }
        return strlen($data);
    }
    public function stream_close() { echo "stream_close\n"; }
}
stream_wrapper_register('closetest', Wrapper::class);
$doc = new PlutoBook\Document();
$doc->loadHtml('<p>close me</p>');
foreach (['pclose', 'fclose'] as $mode) {
    Wrapper::$mode = $mode;
    Wrapper::$writes = 0;
    Wrapper::$resource = fopen('closetest://sink', 'wb');
    try {
        $doc->writeToStream(Wrapper::$resource);
        echo "rendered\n";
    } catch (PlutoBook\Exception $e) {
        echo $e->getMessage(), "\n";
    }
    echo 'resource after render: ', get_resource_type(Wrapper::$resource), "\n";
    Wrapper::$resource = null;
}
echo substr($doc->toPdf(), 0, 5), "\n";
?>
--EXPECT--
pclose inside the callback: 1
stream_close
Unable to write the PDF output: the destination stream was closed during rendering
resource after render: Unknown
fclose inside the callback: false
rendered
resource after render: stream
stream_close
%PDF-
