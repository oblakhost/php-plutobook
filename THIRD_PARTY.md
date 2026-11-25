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

The PHP sources, test client, and development tools are downloaded separately
for local builds. Each retains its upstream license.
