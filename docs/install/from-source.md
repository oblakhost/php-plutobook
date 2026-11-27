# Building from source

PIE compiles the extension when no ready-made build fits, and distribution
packagers can build it the same way, with `phpize`, `./configure --with-plutobook`
and `make`. `configure` finds the libraries through `pkg-config`:

- **PlutoBook 0.20.0** (`plutobook.pc`). It builds with Meson. This project uses
  `-Ddefault_library=shared -Dturbojpeg=enabled -Dwebp=enabled`. Add
  `-Dcurl=disabled`: the extension downloads remote files itself and never uses
  PlutoBook's own HTTP client.
- **libcurl 7.68.0 or newer with HTTP and HTTPS** (`libcurl.pc`, from a package such
  as `libcurl4-openssl-dev` or `libcurl-devel`). `configure` stops with an error if
  libcurl is missing or lacks either protocol. If the libcurl loaded at run time is
  older than 7.68.0, remote files are skipped as `network-error`, and
  `php --ri plutobook` shows the version it found.

The usual `CPPFLAGS`, `CFLAGS`, `LDFLAGS` and `PKG_CONFIG_PATH` variables apply to
`configure`. Meson and CMake take `CFLAGS`, `CXXFLAGS` and `LDFLAGS` the same way.

Unpatched Cairo 1.18 and PlutoBook 0.20.0 leak memory on every render. See
[Limitations](../limitations.md).

To work on the extension itself, see
[CONTRIBUTING.md](https://github.com/oblakhost/php-plutobook/blob/master/CONTRIBUTING.md).
