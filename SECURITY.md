# Security policy

## Supported versions

Security fixes go into the latest release. Update to it before reporting, and check
whether the problem is still there.

## Reporting a vulnerability

Please don't open a public issue. Report it privately through GitHub instead:

1. Open the repository's
   [Security tab](https://github.com/oblakhost/php-plutobook/security).
2. Click **Report a vulnerability** and describe the problem.

Only the maintainers can see the report. Replies, fixes and the eventual public
advisory are handled in that same private thread.

It helps to include:

- the output of `php --ri plutobook`, your PHP version and your operating system
- a minimal HTML document or PHP script that shows the problem
- what you expected to happen and what happened instead

## What counts

The extension's [security model](docs/security-model.md) describes what it
promises. Reports that break those promises are especially welcome, for example:

- a template that reads files outside its `baseUrl` or `documentRoot` folder, or
  past PHP's `open_basedir`
- a download from a host that isn't in `allowedHosts`, from an internal address, or
  over a redirect that should have been refused
- loaded files or downloads that get past the size or time limits
- input that crashes PHP or corrupts memory

Problems in PlutoBook or in one of the bundled libraries are welcome here too when
they affect the extension; the maintainers will take them upstream.
