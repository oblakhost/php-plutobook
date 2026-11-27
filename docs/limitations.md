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

## Memory with older system libraries

Cairo 1.18 and PlutoBook 0.20.0 leak memory on every render
([Cairo #1000](https://gitlab.freedesktop.org/cairo/cairo/-/issues/1000),
[PlutoBook #84](https://github.com/plutoprint/plutobook/issues/84)), about
19 KiB per invoice. The ready-made Linux and Windows builds include the fixes. If
you compiled against unpatched system libraries, restart FPM workers regularly
with `pm.max_requests`. See [Building from source](install/from-source.md).
