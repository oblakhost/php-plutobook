import { appendFileSync } from "node:fs";

// The release workflow builds the binaries for the version a dry run reports, then
// publishes in a separate job. With RELEASE_VERSION_OUTPUT set, the dry run writes the
// version to the GitHub Actions job outputs. With PLUTOBOOK_RELEASE_VERSION set, the
// release stops before anything is tagged if the version is no longer the one built.
export function verifyRelease(_, { nextRelease: { version }, logger }) {
  const built = process.env.PLUTOBOOK_RELEASE_VERSION;
  if (built && built !== version) {
    throw new Error(`The assets were built as ${built}, but this release would be ${version}`);
  }
  if (process.env.RELEASE_VERSION_OUTPUT) {
    appendFileSync(process.env.RELEASE_VERSION_OUTPUT, `version=${version}\n`);
    logger.log("Wrote version %s to %s", version, process.env.RELEASE_VERSION_OUTPUT);
  }
}
