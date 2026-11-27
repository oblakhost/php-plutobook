# Configuration

All settings can go in `php.ini`. `plutobook.allow_private_addresses` can't be
changed with `ini_set()`; it belongs in `php.ini`, `.user.ini` or an FPM pool.

| Setting | Default | What it does |
| --- | --- | --- |
| `plutobook.max_asset_size` | `32M` | Largest single file or download, after decompression. |
| `plutobook.max_total_asset_size` | `128M` | Most bytes one document may load in total. |
| `plutobook.http_timeout` | `10` | Seconds per download, redirects included (1–3600). |
| `plutobook.http_document_timeout` | `30` | Seconds of network time per document (up to 86400). |
| `plutobook.http_max_redirects` | `5` | Redirects followed per download (0–20). |
| `plutobook.http_ca_file` | empty | CA bundle to trust instead of libcurl's default. |
| `plutobook.allow_private_addresses` | `Off` | Allow downloads from internal addresses. |

## Sizes

Sizes accept `K`, `M` and `G`, and `0` removes the limit.
`plutobook.max_asset_size` can be at most 2147483647 bytes, PlutoBook's own
limit, so `2G` is one byte too many.

Downloaded and loaded files use memory outside `memory_limit`, which is why the
size limits exist. Each `loadHtml()` starts a new total. Files that would go over
it are skipped as `too-large` with the detail
`plutobook.max_total_asset_size reached`.

## Network time

Network time is real time. On Linux, `max_execution_time` counts CPU time, so it
barely covers waiting for downloads; these settings do. The document's budget
starts at `loadHtml()` and includes fonts and images fetched while rendering. `0` turns it off. When it runs out, the remaining downloads are
skipped as `timeout` with the detail `plutobook.http_document_timeout reached`.

DNS lookups count toward both timeouts only when libcurl can look up names in the
background, as most libcurl builds can. `php --ri plutobook` shows
`DNS lookups => bounded by the download timeouts` when it can. Otherwise a slow
DNS server can hold a download past its timeout, and the `timeout` error's detail
says how long the lookup took.

## Changing settings

Invalid values raise a warning and keep the previous setting. A setting changed
with `ini_set()` applies from the next file the document loads, including fonts
and images loaded while rendering.

`plutobook.allow_private_addresses` is for administrators; see
[Security](../security-model.md).
