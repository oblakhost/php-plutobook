import { execFileSync } from "node:child_process";
import { join } from "node:path";

// Runs after release-source.mjs has added the source archive and before the tag exists:
// signs the rpm files and writes a signed SHA256SUMS over every asset to upload.
export function prepare(_, { cwd, logger }) {
  const output = execFileSync(join(cwd, "scripts/sign-release"), [join(cwd, "artifacts/releases")], {
    cwd,
    encoding: "utf8",
  });
  logger.log(output.trim());
}
