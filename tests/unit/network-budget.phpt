--TEST--
Remote assets share the document's byte budget and network time budget
--INI--
plutobook.allow_private_addresses=1
plutobook.http_timeout=5
--CAPTURE_STDIO--
STDOUT
--FILE--
<?php
require __DIR__ . '/network.inc';
use PlutoBook\Document;
$port = plutobook_test_server('tcp');
$origin = "http://127.0.0.1:$port";
$hosts = ['127.0.0.1'];
$errors = static fn (Document $doc): string => implode(', ', array_map(
    static fn (array $e): string => str_replace($origin, '', $e['url']) . " $e[reason] ($e[detail])",
    $doc->getResourceErrors(),
)) ?: 'none';
$doc = new Document();

echo "-- bytes\n";
// The fixture PNG is 75 bytes, so a 100-byte budget fits the first copy only.
ini_set('plutobook.max_total_asset_size', '100');
$doc->loadHtml('<img src="/img/dot.png?1"><img src="/img/dot.png?2">', "$origin/", allowedHosts: $hosts);
$doc->toPdf();
echo $errors($doc), "\n";
ini_restore('plutobook.max_total_asset_size');

echo "-- time\n";
// The slow response outlasts the 1-second document budget. The next asset is then skipped
// without a request.
ini_set('plutobook.http_document_timeout', '1');
$doc->loadHtml('<img src="/slow.png"><img src="/img/dot.png?3">', "$origin/", allowedHosts: $hosts);
$doc->toPdf();
echo $errors($doc), "\n";
var_dump(ini_set('plutobook.http_document_timeout', '-1'));
?>
--EXPECTF--
-- bytes
/img/dot.png?2 too-large (plutobook.max_total_asset_size reached)
-- time
/slow.png timeout (plutobook.http_document_timeout reached), /img/dot.png?3 timeout (plutobook.http_document_timeout reached)

Warning: Invalid "plutobook.http_document_timeout" setting. Expected an integer from 0 to 86400 in %s on line %d
bool(false)
