# Third-party notices

The php-pdf binding and its original project files use the Apache License 2.0.
The binding links to PlutoBook as a separate shared library through its public C API.

## PlutoBook

PlutoBook v0.20.0, commit `8df624e11bd55d8e4f276ebf559335aa29fb926c`,
is licensed under MPL-2.0. The checkout downloads its source into a private cache.
The PIE source package contains the binding and expects a separately installed
PlutoBook library. Its Apache-2.0 license applies to the project's own files.
[Exact upstream source](https://github.com/plutoprint/plutobook/tree/8df624e11bd55d8e4f276ebf559335aa29fb926c)
· [Upstream license](https://github.com/plutoprint/plutobook/blob/8df624e11bd55d8e4f276ebf559335aa29fb926c/LICENSE)

If a release bundles the engine, include its license, copyright notices,
`CREDITS`, and the relevant third-party notices. For an engine binary, tell
recipients where to obtain the corresponding MPL-covered source, including
any modifications shipped. Retain upstream notices in any copied or modified
MPL-covered files and make those files available under MPL-2.0. Keep new
Apache-licensed binding code in separate files. Check the licenses of the
libraries and fonts actually included in a binary distribution.
[Upstream credits](https://github.com/plutoprint/plutobook/blob/v0.20.0/CREDITS)
· [MPL 2.0, sections 3.1–3.4](https://www.mozilla.org/en-US/MPL/2.0/)
· [Mozilla's guidance for distributors](https://www.mozilla.org/en-US/MPL/2.0/FAQ/)

## libcurl

The binding links to libcurl as a separate shared library for remote assets.
libcurl uses the curl license, an MIT/X-derived permissive license. Distribution
builds link the system libcurl. When the host lacks one with HTTPS,
`scripts/build-plutobook` downloads curl 8.23.0 into the private cache and
installs its `COPYING` file under the private prefix's `share/licenses/libcurl`.
Binary distributions that bundle libcurl must include that notice.
[Upstream license](https://curl.se/docs/copyright.html)

## Portable binaries and packages

The Linux modules from `scripts/build-portable`, the PIE pre-packaged binaries and
the deb and rpm packages link the following statically. Each ships their license
texts: in `licenses/` of the PIE archive, `/usr/share/doc/php8.X-plutobook/licenses/`
of a deb and `/usr/share/licenses/php-plutobook/bundled/` of an rpm.

| Component | Version | License | Changes |
| --- | --- | --- | --- |
| PlutoBook | 0.20.0 | MPL-2.0 | `tools/patches/plutobook-0.20.0-static-font-variations.patch` |
| Cairo | 1.18.4 | MPL-1.1, chosen from LGPL-2.1 or MPL-1.1 | PlutoBook's `subprojects/packagefiles/cairo-fix-font-options-leaks.patch` |
| pixman | 0.46.4 | MIT | |
| HarfBuzz | 13.0.1 | MIT | |
| FreeType | 2.14.3 | FreeType License, chosen from it or GPL-2.0 | |
| Fontconfig | 2.18.3 | MIT | `tools/patches/fontconfig-2.18.3-quiet-cache-dirs.patch` |
| libpng | 1.6.59 | libpng License 2 | |
| zlib | 1.3.2 | zlib | |
| Expat | 2.8.2 | MIT | |
| ICU | 78.2 | Unicode License 3 | Data reduced to `tools/portable/icu-keep.txt` |
| libjpeg-turbo | 3.1.4.1 | IJG, BSD-3-Clause and zlib | |
| libwebp | 1.6.0 | BSD-3-Clause, with its patent grant | |
| libcurl (`el8` modules only) | 8.23.0 | curl | |

The Meson build files from WrapDB that built these libraries are MIT-licensed
(`LICENSE.build`). The modules also contain parts of the GCC runtime, which its
Runtime Library Exception allows without further conditions. The system's glibc,
libstdc++, libcurl or GnuTLS are linked dynamically and not distributed.

Portions of this software are copyright © 2026 The FreeType Project
(www.freetype.org). All rights reserved. This software is based in part on the
work of the Independent JPEG Group.

PlutoBook and Cairo are MPL-covered and modified. Every release with binaries
attaches `php_plutobook-VERSION-mpl-sources.tar.xz`: the patched PlutoBook source
as built, the changed Cairo files, the patches, and the pinned upstream archives
for everything unmodified. The modified files remain available under their MPL
versions; the binding's own files stay Apache-2.0.

The PHP sources, test client, and development tools are downloaded separately
for local builds. Each retains its upstream license.
