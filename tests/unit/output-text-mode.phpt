--TEST--
Plain files opened in text mode are refused, since Windows would translate line endings in the PDF
--FILE--
<?php
use PlutoBook\Exception;
$doc = new PlutoBook\Document();
$doc->loadHtml('<p>text mode</p>');
$path = tempnam(__DIR__, 'text-mode');
foreach (['wt', 'w+t', 'at', 'xt', 'ct', 'wb', 'w', 'r+'] as $mode) {
    if ($mode === 'xt') {
        unlink($path);
    }
    $stream = fopen($path, $mode);
    try {
        $doc->writeToStream($stream);
        echo "$mode: written\n";
    } catch (Exception $e) {
        echo "$mode: ", $e->getMessage(), "\n";
    }
    fclose($stream);
}
// Only the plain file wrapper translates line endings; other streams ignore the flag.
$memory = fopen('php://memory', 'w+t');
$doc->writeToStream($memory);
rewind($memory);
echo 'php://memory w+t: ', fread($memory, 5), "\n";
fclose($memory);
unlink($path);
?>
--EXPECT--
wt: Expected a PHP stream opened in binary mode, without "t"
w+t: Expected a PHP stream opened in binary mode, without "t"
at: Expected a PHP stream opened in binary mode, without "t"
xt: Expected a PHP stream opened in binary mode, without "t"
ct: Expected a PHP stream opened in binary mode, without "t"
wb: written
w: written
r+: written
php://memory w+t: %PDF-
