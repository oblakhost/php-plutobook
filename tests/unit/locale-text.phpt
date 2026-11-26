--TEST--
Text in many scripts renders under any lang value, with line, word and character breaks
--DESCRIPTION--
PlutoBook creates ICU break iterators for each document language and only asserts that
creation succeeded, which release builds compile out. A missing iterator would crash.
The portable modules ship a reduced ICU data package (tools/portable/icu-keep.txt).
--FILE--
<?php
use PlutoBook\Document;

// Dictionary-based scripts without spaces, CJK, right-to-left scripts, emoji and
// language-specific case mappings, in a narrow box so every break opportunity is used.
$text = 'Zdravo svete, Здраво свете. ภาษาไทยไม่เว้นวรรคระหว่างคำ ພາສາລາວບໍ່ມີຍະຫວ່າງຄຳ '
    . 'ភាសាខ្មែរមិនដកឃ្លា မြန်မာစာမှာ စကားလုံးကြား နေရာမခြားပါ '
    . '日本語の文章は単語の間に空白を入れません。中文句子的词之间没有空格。한국어 문장입니다 '
    . 'العربية من اليمين إلى اليسار עברית מימין לשמאל 👍🏽👨‍👩‍👧 istanbul ĳsselmeer straße';
$langs = ['', 'en', 'en-US', 'sr-Latn-RS', 'sr-Cyrl', 'th', 'lo', 'km', 'my', 'ja', 'ja-JP',
    'zh-Hant-TW', 'zh-Hans', 'yue', 'ko', 'ar', 'he', 'fa', 'tr', 'az', 'lt', 'el', 'nl', 'de-1996',
    'es-419', 'pt-BR', 'ja-u-lb-loose', 'de-u-co-phonebk', 'x-private', 'i-default', 'und', 'root',
    'zz-ZZ', 'qaa', 'en_US_POSIX', 'sr@latin', '1234', str_repeat('ab-', 40) . 'cd', '!!', ' en '];
$failed = [];
foreach ($langs as $lang) {
    $html = '';
    foreach (['none', 'uppercase', 'lowercase', 'capitalize'] as $transform) {
        $html .= sprintf('<div lang="%s" style="width: 40px; text-transform: %s">%s</div>',
            htmlspecialchars($lang), $transform, $text);
    }
    $doc = new Document();
    $doc->loadHtml($html);
    if (!str_starts_with($doc->toPdf(), '%PDF-')) {
        $failed[] = $lang;
    }
}
echo count($langs), " languages, failed: ", json_encode($failed), "\n";
?>
--EXPECT--
40 languages, failed: []
