const preset = "conventionalcommits";

// Commit rules and release-note sections shared with the oblak WooCommerce plugins
// (wc-unior, wc-subscription-downloads), so every project releases the same way.
const releaseRules = [
  { type: "chore", release: false },
  { type: "perf", release: "patch" },
  { type: "compat", release: "patch" },
  { type: "refactor", release: "patch" },
  { type: "style", release: "patch" },
];
const parserOpts = { noteKeywords: ["BREAKING CHANGE", "BREAKING CHANGES"] };
const types = [
  { type: "feat", section: ":sparkles: Features", hidden: false },
  { type: "fix", section: ":bug: Bug Fixes", hidden: false },
  { type: "compat", section: ":gear: Compatibility", hidden: false },
  { type: "refactor", section: ":recycle: Refactor", hidden: false },
  { type: "style", section: ":art: Code style", hidden: false },
  { type: "perf", section: ":rocket: Performance", hidden: false },
  { type: "chore", section: ":wrench: Maintenance", hidden: false },
];

export default {
  branches: ["master"],
  repositoryUrl: "https://github.com/oblakhost/php-plutobook.git",
  tagFormat: "${version}",
  // The release workflow puts the binaries and packages in artifacts/releases first.
  // Prepare steps run in order: the source archive, then the signatures over everything.
  plugins: [
    ["@semantic-release/commit-analyzer", { preset, releaseRules, parserOpts }],
    "./scripts/release-version.mjs",
    ["@semantic-release/release-notes-generator", { preset, presetConfig: { types } }],
    "./scripts/release-source.mjs",
    "./scripts/release-sign.mjs",
    ["@semantic-release/github", {
      assets: ["artifacts/releases/*"],
      successComment: false,
      failComment: false,
      releasedLabels: false,
    }],
  ],
};
