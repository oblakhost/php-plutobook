--TEST--
Additional CSS overrides document and inline styles unless the document uses !important
--EXTENSIONS--
zlib
--FILE--
<?php
use PlutoBook\Document;
// PlutoBook 0.20.0 ranks the css: argument (user origin) above author and inline styles.
// Lock that ordering: a renderer upgrade that adopts the CSS spec's user-origin rules must fail here.
function textFill(string $html, string $css = ''): string {
    $doc = new Document();
    $doc->loadHtml($html, css: $css);
    preg_match_all('/stream\r?\n(.*?)\r?\nendstream/s', $doc->toPdf(), $streams);
    $fills = [];
    foreach ($streams[1] as $stream) {
        $content = @gzuncompress($stream);
        if ($content !== false && preg_match_all('/(\d+ \d+ \d+) rg/', $content, $match)) {
            array_push($fills, ...$match[1]);
        }
    }
    return implode(', ', array_unique($fills));
}
$red = '<style>p { color: #f00 }</style>';
$blue = 'p { color: #00f }';
echo 'document only: ', textFill("$red<p>text</p>"), "\n";
echo 'over <style>: ', textFill("$red<p>text</p>", $blue), "\n";
echo 'over inline style: ', textFill('<p style="color: #0f0">text</p>', $blue), "\n";
echo 'over higher specificity: ', textFill('<style>body p#x { color: #f00 }</style><p id="x">text</p>', $blue), "\n";
echo 'under document !important: ', textFill('<style>p { color: #f00 !important }</style><p>text</p>', $blue), "\n";
echo 'css !important over document !important: ', textFill(
    '<style>p { color: #f00 !important }</style><p style="color: #0f0 !important">text</p>',
    'p { color: #00f !important }',
), "\n";
?>
--EXPECT--
document only: 1 0 0
over <style>: 0 0 1
over inline style: 0 0 1
over higher specificity: 0 0 1
under document !important: 1 0 0
css !important over document !important: 0 0 1
