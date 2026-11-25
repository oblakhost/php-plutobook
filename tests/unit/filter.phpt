--TEST--
A PHP write filter can allocate replacement buckets on its destination stream
--FILE--
<?php
class CopyBuckets extends php_user_filter {
    public function filter($in, $out, &$consumed, bool $closing): int {
        while ($bucket = stream_bucket_make_writeable($in)) {
            $consumed += strlen($bucket->data);
            stream_bucket_append($out, stream_bucket_new($this->stream, $bucket->data));
        }
        return PSFS_PASS_ON;
    }
}
stream_filter_register('pdf.copy-buckets', CopyBuckets::class);
$s = fopen('php://temp', 'w+b');
$filter = stream_filter_append($s, 'pdf.copy-buckets', STREAM_FILTER_WRITE);
$d = new PlutoBook\Document();
$d->loadHtml('<p>filtered output</p>');
$d->writeToStream($s);
stream_filter_remove($filter);
rewind($s);
$pdf = stream_get_contents($s);
var_dump(str_starts_with($pdf, '%PDF-'), str_ends_with(trim($pdf), '%%EOF'));
fclose($s);
?>
--EXPECT--
bool(true)
bool(true)
