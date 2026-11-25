--TEST--
Document owns independent native state, loads malformed HTML, reloads and emits PDF bytes
--FILE--
<?php
use PlutoBook\Document;
use PlutoBook\Exception;
$a = new Document(pageSize: 'a4', margins: '15mm');
$b = new Document('Letter', 'landscape', '2mm 3pt 4px 1cm');
try { $a->toPdf(); } catch (Exception $e) { echo "empty\n"; }
$a->loadHtml('<b>first paragraph');
$b->loadHtml('<p>Second</p>', css: 'p { color: navy }');
foreach ([$a, $b] as $doc) {
    $pdf = $doc->toPdf();
    var_dump(str_starts_with($pdf, '%PDF-'), str_contains($pdf, "\0"), str_ends_with(trim($pdf), '%%EOF'));
}
$a->loadHtml('');
var_dump(strlen($a->toPdf()) > 100);
try { $a->__construct(); } catch (Exception $e) { echo "initialized\n"; }
try { $c = clone $a; } catch (Error $e) { echo "uncloneable\n"; }
try { serialize($a); } catch (\Exception $e) { echo "unserializable\n"; }
unset($a);
var_dump(str_starts_with($b->toPdf(), '%PDF-'));
?>
--EXPECT--
empty
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
initialized
uncloneable
unserializable
bool(true)
