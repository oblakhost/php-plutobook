# Limitations

## No JavaScript and no CSS Grid

Both are on
[PlutoBook's roadmap](https://github.com/plutoprint/plutobook/blob/v0.20.0/README.md#roadmap).

## One page size per document

Only the first page's size and margins are used, and `@page :first` is the only
page selector that works reliably.

## No PDF options

The API has no settings for PDF/A, encryption, file attachments (such as the XML
in Factur-X or ZUGFeRD invoices) or document metadata.

## Copying and searching text

Text in the PDF can be selected, copied and searched, including ligatures such as
"ffi", tabular figures (`font-variant-numeric: tabular-nums`) and letters a font
draws differently for a language. A few cases remain:

- **Builds from source.** PlutoBook 0.20.0 itself loses the text of such glyphs: a
  PDF looks right, but "office" copies as "o?ce" and tabular digits as "?". The
  ready-made Linux and Windows builds include the fix. If you compile PlutoBook
  yourself, apply `tools/patches/plutobook-0.20.0-text-clusters.patch`; see
  [Building from source](install/from-source.md). Without it, set
  `font-variant-ligatures: none` and avoid tabular figures where the text matters.
- **Readers that ignore ActualText.** When a font has its own character for a
  ligature, such as "ﬃ" (U+FB03) in DejaVu Sans, the PDF stores "ffi" as the
  glyph's ActualText. Poppler (`pdftotext`, Evince, Okular) reads it. Tools that
  don't, such as pdfminer and pypdf, still return the ligature character.
- **Wide letter spacing with ligatures.** PlutoBook keeps ligatures in letter-spaced
  text. Above about `0.1em` of `letter-spacing`, `pdftotext` then puts a space
  between every letter of a line that has a ligature. Add
  `font-variant-ligatures: none` next to `letter-spacing`, as browsers do by
  themselves.
- **Arabic lam-alef in pdftotext.** The two letters of a right-to-left ligature
  come out in swapped order ("ال" for "لا"). The PDF holds them in the right order.
- **Outlined SVG text.** SVG text with a `stroke` and no `fill` is drawn as shapes
  and has no text to copy.

## Memory with older system libraries

Cairo 1.18 and PlutoBook 0.20.0 leak memory on every render
([Cairo #1000](https://gitlab.freedesktop.org/cairo/cairo/-/issues/1000),
[PlutoBook #84](https://github.com/plutoprint/plutobook/issues/84)), about
19 KiB per invoice. The ready-made Linux and Windows builds include the fixes. If
you compiled against unpatched system libraries, restart FPM workers regularly
with `pm.max_requests`. See [Building from source](install/from-source.md).
