import { execFileSync } from "node:child_process";
import { mkdirSync } from "node:fs";
import { join } from "node:path";

export function prepare(_, { cwd, nextRelease: { version, gitHead } }) {
  const git = (...args) => execFileSync("git", args, { cwd, stdio: "pipe" });
  // Packaging an initialization-only checkout must fail before any tag is made.
  git("cat-file", "-e", `${gitHead}:ext/config.m4`);

  const directory = join(cwd, "artifacts/releases");
  mkdirSync(directory, { recursive: true });
  const name = `php_plutobook-${version}-src`;
  const header = `#define PHP_PLUTOBOOK_VERSION "${version}"\n`;
  git("archive", "--format=tar.gz", `--output=${join(directory, name)}.tgz`,
    `--prefix=${name}/`, `--add-virtual-file=${name}/ext/release-version.h:${header}`, gitHead);
}
