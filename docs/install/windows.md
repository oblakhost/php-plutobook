# Windows

Install with [PIE](index.md#any-platform-pie). It picks the ZIP for your PHP
version, thread safety, compiler and architecture. For example,
`php_plutobook-VERSION-8.4-nts-vs17-x86_64.zip` is for 64-bit non-thread-safe PHP
8.4 built with Visual Studio 2022, where `VERSION` is the release.

| File | Location |
| --- | --- |
| `php_plutobook.dll` | PHP's extension folder |
| `libplutobook-0.dll` | Next to `php.exe` |
| Fonts, font configuration and licenses | `extras\plutobook` |

Restart your web server after installing.

## Fonts

On Windows the extension uses the DejaVu fonts it ships with, not the fonts in
`C:\Windows\Fonts`. To use other fonts, load them with `@font-face` from your
template's folder (see [Local images, stylesheets and fonts](../guides/local-files.md#fonts)),
or set `FONTCONFIG_FILE` or `FONTCONFIG_PATH` to your own font configuration, which
then takes precedence over the packaged one.

If fonts don't load and PHP's folder path has characters outside the Windows ANSI
code page, such as Cyrillic or Chinese, copy `fonts.conf` and the `fonts` folder
next to it to a plain path like `C:\plutobook-fonts` and set `FONTCONFIG_PATH` to
it. This only matters on drives with 8.3 short names turned off.

## File paths

Local files use drive URLs such as `file:///C:/invoices/assets/`. See
[Local images, stylesheets and fonts](../guides/local-files.md).
