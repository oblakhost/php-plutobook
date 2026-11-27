# Security

Templates often contain user data, so the extension is strict by default.

## Local files

- Files are only read from the `baseUrl` or `documentRoot` folder and its
  subfolders, and PHP's `open_basedir` is respected.
- On Linux, symlinks within the folder work, and symlinks leading out of it are
  refused. A path swapped for a symlink after the checks is refused too. None of
  this needs `/proc`, so it works in an FPM `chroot`.
- On Windows, the folder is held open while in use, so it can't be swapped for
  another one mid-render. Junctions, symlinks, UNC paths, device names and
  alternate data streams are refused.
- Devices, FIFOs and sockets are never read. On Linux with `/proc` mounted, a file
  swapped for one after the checks isn't even opened. Without `/proc`, such as in an
  FPM `chroot`, it can be opened for a moment, without waiting on it, and is then
  closed unread.

## Downloads

- Only `http` and `https` are used, and only for hosts in `allowedHosts`.
- Every redirect is checked against `allowedHosts`. Redirects from https to http,
  or to other schemes, are refused.
- TLS 1.2 or newer is required, and certificates and host names are always verified.
- Proxy variables such as `http_proxy` are ignored, and no cookies are kept.
  Connections are reused within one document, never shared between documents.
- Internal addresses are refused: loopback, private networks (10/8, 172.16/12,
  192.168/16, 100.64/10, fc00::/7), link-local addresses (169.254/16, including
  cloud metadata, and fe80::/10), multicast, reserved and unspecified addresses.
  The check uses the address actually connected to, so DNS tricks don't get
  around it. An administrator can allow them with `plutobook.allow_private_addresses`.

## Memory and time

Loaded files live outside `memory_limit`, so the extension caps their size per
file and per document, and limits network time per download and per document. See
[Configuration](reference/configuration.md) for the settings and their defaults.
