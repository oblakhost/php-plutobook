--TEST--
Paper presets, orientation and CSS shorthand lengths
--FILE--
<?php
use PlutoBook\Document;
foreach (['A3', 'a4', 'A5', 'B4', 'B5', 'Letter', 'Legal', 'Ledger'] as $paper) {
    new Document($paper, 'landscape', '0');
}
foreach (['0', '1pt 2px', '3mm 1pc 0', '0.5in 1cm 2pt 3px', '  3mm  '] as $margins) {
    new Document(margins: $margins);
}
echo "valid\n";
foreach ([['A0'], ["A4\0bad"], ['A4', 'sideways'], ['A4', 'portrait', '1%'],
          ['A4', 'portrait', '-1pt'], ['A4', 'portrait', '1mm 2mm 3mm 4mm 5mm'],
          ['A4', 'portrait', '1in2in'], ['A4', 'portrait', ''],
          ['A4', 'portrait', '300mm'], ['A4', 'portrait', "1mm\0bad"]] as $args) {
    try { new Document(...$args); echo "accepted\n"; } catch (ValueError $e) { echo "invalid\n"; }
}
?>
--EXPECT--
valid
invalid
invalid
invalid
invalid
invalid
invalid
invalid
invalid
invalid
invalid
