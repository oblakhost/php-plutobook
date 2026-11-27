--TEST--
Write filters report what they consume, so output the destination refuses after a filter goes unnoticed
--FILE--
<?php
// PHP returns the bytes the first write filter consumed and drops filtered output the
// underlying stream refuses without an error (main/streams/streams.c, PHP 8.2 to 8.6).
// Only streams without write filters can report refused writes. README: "Saving the PDF".
class RefusingStream
{
    public $context;
    public static int $offered = 0;

    public function stream_open(string $path, string $mode, int $options, ?string &$opened): bool
    {
        return true;
    }

    public function stream_write(string $data): int
    {
        self::$offered += strlen($data);
        return 0;
    }
}
stream_wrapper_register('refuse', RefusingStream::class);
$doc = new PlutoBook\Document();
$doc->loadHtml('<p>filtered output</p>');

echo "-- no filter, destination refuses\n";
$stream = fopen('refuse://pdf', 'wb');
try {
    $doc->writeToStream($stream);
    echo "returned\n";
} catch (PlutoBook\Exception $e) {
    echo $e->getMessage(), "\n";
}
fclose($stream);

echo "-- write filter, destination refuses\n";
RefusingStream::$offered = 0;
$stream = fopen('refuse://pdf', 'wb');
stream_filter_append($stream, 'convert.base64-encode', STREAM_FILTER_WRITE);
$doc->writeToStream($stream);
echo 'returned after the destination refused ', RefusingStream::$offered > 0 ? 'filtered output' : 'nothing', "\n";
fclose($stream);

echo "-- write filter, destination accepts\n";
// A filter can hold back its last bytes until the stream is closed, which the caller does.
$path = tempnam(__DIR__, 'filter-delivery');
$stream = fopen($path, 'wb');
stream_filter_append($stream, 'convert.base64-encode', STREAM_FILTER_WRITE);
$doc->writeToStream($stream);
fclose($stream);
$pdf = base64_decode(file_get_contents($path), true);
echo 'complete after fclose(): ', var_export(str_starts_with($pdf, '%PDF-') && str_ends_with(rtrim($pdf), '%%EOF'), true), "\n";
unlink($path);
?>
--EXPECT--
-- no filter, destination refuses
Unable to write the PDF output: the destination stream did not accept the data
-- write filter, destination refuses
returned after the destination refused filtered output
-- write filter, destination accepts
complete after fclose(): true
