--TEST--
PHP streams and local files receive binary PDF content and failures leave the document usable
--FILE--
<?php
use PlutoBook\Document;
use PlutoBook\Exception;
$doc = new Document();
$doc->loadHtml('<p>stream test</p>');
$stream = fopen('php://temp', 'w+b');
$doc->writeToStream($stream);
rewind($stream);
echo fread($stream, 5), "\n";
fclose($stream);
try { $doc->writeToStream($stream); } catch (TypeError $e) { echo "closed stream\n"; }
try { $doc->writeToStream('no'); } catch (TypeError $e) { echo "wrong type\n"; }
$path = tempnam(__DIR__, 'output');
$doc->writeToFile($path);
echo file_get_contents($path, false, null, 0, 5), "\n";
$ro = fopen($path, 'rb');
try { $doc->writeToStream($ro); } catch (Exception $e) { echo "read-only\n"; }
fclose($ro);
unlink($path);
foreach (['', 'php://memory', "bad\0path"] as $invalid) {
    try { $doc->writeToFile($invalid); } catch (ValueError $e) { echo "invalid path\n"; }
}
try { $doc->writeToFile(__DIR__); } catch (Exception $e) { echo "open failed\n"; }
echo substr($doc->toPdf(), 0, 5), "\n";
?>
--EXPECT--
%PDF-
closed stream
wrong type
%PDF-
read-only
invalid path
invalid path
invalid path
open failed
%PDF-
