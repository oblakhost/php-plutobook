--TEST--
A stream that is already receiving a PDF cannot be borrowed again from its write callback
--FILE--
<?php
class Wrapper {
    public $context;
    public static $resource;
    public static PlutoBook\Document $other;
    public static string $bytes = '';
    public static ?string $nested = null;
    public function stream_open($path, $mode, $options, &$opened) { return true; }
    public function stream_write($data) {
        if (self::$nested === null) {
            try {
                self::$other->writeToStream(self::$resource);
                self::$nested = 'rendered';
            } catch (PlutoBook\Exception $e) {
                self::$nested = $e->getMessage();
            }
        }
        self::$bytes .= $data;
        return strlen($data);
    }
}
stream_wrapper_register('busytest', Wrapper::class);
Wrapper::$other = new PlutoBook\Document();
Wrapper::$other->loadHtml('<p>inner</p>');
$doc = new PlutoBook\Document();
$doc->loadHtml('<p>outer</p>');
Wrapper::$resource = fopen('busytest://sink', 'wb');
$doc->writeToStream(Wrapper::$resource);
echo 'nested render: ', Wrapper::$nested, "\n";
echo 'outer PDF complete: ', var_export(str_ends_with(trim(Wrapper::$bytes), '%%EOF'), true), "\n";
?>
--EXPECT--
nested render: The destination stream is already in use
outer PDF complete: true
