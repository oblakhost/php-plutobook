# Errors and time limits

## Errors

- Invalid arguments, such as an unknown page size, throw `ValueError` or `TypeError`.
- Rendering and writing failures throw `PlutoBook\Exception`, a
  `RuntimeException`. Its message says what went wrong when that is known, for
  example `Unable to write the PDF output: the destination stream did not accept the data`.
- If you write to a custom stream, such as an S3 stream wrapper, and it throws, you
  get its exception, not a `PlutoBook\Exception`.

A missing image or font is not an error: the page renders without it. See
[Missing images and fonts](missing-resources.md).

## Time limits

Rendering counts toward `max_execution_time`. PHP can't stop native code midway,
so the extension checks the limit each time PlutoBook loads a resource or writes
part of the PDF. Once the limit has passed, rendering stops with PHP's usual
`Maximum execution time` error, and shutdown functions still run.

Laying out the pages, before anything is written, can't be interrupted. If that
runs past `max_execution_time` plus `hard_timeout` (a `php.ini` setting, 2 seconds
by default), PHP ends the process. For very large documents, raise the limit with
`set_time_limit()` or render them in a queue worker.

On Linux, PHP's time limit counts CPU time, so time spent waiting for downloads
barely counts toward it. Downloads have their own limits instead; see
[Configuration](../reference/configuration.md#network-time).
