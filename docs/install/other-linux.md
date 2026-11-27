# Other Linux distributions

On other distributions with glibc 2.28 or newer, such as Fedora, Arch Linux or
openSUSE, on x86_64 or arm64, install with [PIE](index.md#any-platform-pie). You
need:

- non-thread-safe PHP 8.2 to 8.6: `php -i | grep 'Thread Safety'` shows `disabled`
- libcurl 7.68.0 or newer
- Fontconfig and a font package such as DejaVu

## Fonts

PIE installs only the extension, so install fonts yourself. The Linux build's
Fontconfig reads the system configuration in `/etc/fonts`.

| Distribution | Fonts |
| --- | --- |
| Debian, Ubuntu, official `php` Docker images | `sudo apt install fontconfig-config fonts-dejavu-core` |
| RHEL family, Fedora | `sudo dnf install fontconfig dejavu-sans-fonts` |
| Arch Linux | `sudo pacman -S fontconfig ttf-dejavu` |
| openSUSE | `sudo zypper install fontconfig dejavu-fonts` |

With thread-safe PHP or musl (Alpine), PIE compiles the extension instead, which
needs the PlutoBook library installed first; see
[Building from source](from-source.md).

[What the ready-made builds contain](index.md#what-the-ready-made-builds-contain)
lists the libraries inside the Linux build and what it needs from the system.
