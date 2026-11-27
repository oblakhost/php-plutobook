# Resource errors

`getResourceErrors()` returns one entry per skipped image, stylesheet or font, as
`['url' => …, 'reason' => …, 'detail' => …]`. `reason` is one of the stable codes
below; `detail` is human-readable text such as `HTTP 404`. See
[Missing images and fonts](../guides/missing-resources.md) for how to use it.

| Reason | Meaning |
| --- | --- |
| `network-disabled` | An `http:` or `https:` URL in a document without `allowedHosts`. |
| `unsupported-scheme` | Any other non-`file:` URL. |
| `no-base-url` | A local file without a `baseUrl` or `documentRoot`. |
| `invalid-url` | A malformed URL, such as a `file:` URL with an encoded NUL byte. |
| `missing` | The file does not exist. |
| `open-basedir` | PHP's `open_basedir` blocks the file. |
| `outside-base` | The file is outside the `documentRoot`, or outside the `baseUrl` folder when there is no document root. |
| `verification-failed` | A folder or the file changed between the checks and the read. |
| `not-regular-file` | A folder, FIFO, socket or device. |
| `empty` | A zero-byte file or response. |
| `too-large` | The file or response is larger than `plutobook.max_asset_size`, or would take the document past `plutobook.max_total_asset_size`. |
| `permission-denied` | PHP can't read the file. |
| `read-error` | Reading the file, or handing it to PlutoBook, failed. |
| `host-not-allowed` | The host, or a redirect's host, isn't in `allowedHosts`. |
| `blocked-address` | The host resolved to an internal address. |
| `http-error` | The final response status isn't 2xx. |
| `redirect-error` | Too many redirects, or a redirect from https to http or to another scheme. |
| `tls-error` | The TLS handshake or certificate check failed. |
| `timeout` | The download took longer than `plutobook.http_timeout`, the document used up `plutobook.http_document_timeout`, or PHP's time limit passed. |
| `network-error` | DNS, connection and other transfer failures, or a libcurl older than 7.68.0. |

The settings named here are described in [Configuration](configuration.md).
