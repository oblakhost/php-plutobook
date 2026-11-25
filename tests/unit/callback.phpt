--TEST--
Output callbacks handle partial writes, preserve the original exception, and reject reentrant mutation
--FILE--
<?php
use PlutoBook\Document;
use PlutoBook\Exception;
class Sink {
    public $context;
    public static $mode;
    public static $bytes;
    public static $doc;
    function stream_open($path, $mode, $options, &$opened) { return true; }
    function stream_write($data) {
        if (self::$mode === 'zero') { return 0; }
        if (self::$mode === 'throw') { throw new DomainException('sink error'); }
        if (self::$mode === 'reload') { self::$doc->loadHtml('<p>changed</p>'); }
        $length = min(19, strlen($data));
        self::$bytes .= substr($data, 0, $length);
        return $length;
    }
    function stream_close() {}
}
stream_wrapper_register('outputtest', Sink::class);
$s = fopen('outputtest://sink', 'wb');
Sink::$doc = new Document();
Sink::$doc->loadHtml('hello');
foreach (['zero', 'throw', 'reload', 'partial'] as $mode) {
    Sink::$mode = $mode;
    try {
        Sink::$doc->writeToStream($s);
        echo substr(Sink::$bytes, 0, 5), ' ', substr(trim(Sink::$bytes), -5), "\n";
    } catch (Exception $e) {
        echo $e->getMessage(), "\n";
    } catch (DomainException $e) {
        echo $e->getMessage(), "\n";
    }
}
fclose($s);
?>
--EXPECT--
Unable to write the PDF output
sink error
The document is already rendering
%PDF- %%EOF
