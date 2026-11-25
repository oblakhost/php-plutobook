const preset = "conventionalcommits";

export default {
  branches: ["master"],
  repositoryUrl: "https://github.com/oblakhost/php-pdf.git",
  tagFormat: "${version}",
  plugins: [
    ["@semantic-release/commit-analyzer", { preset }],
    ["@semantic-release/release-notes-generator", { preset }],
    "./scripts/release-source.mjs",
    ["@semantic-release/github", {
      assets: ["artifacts/releases/*.tgz"],
      successComment: false,
      failComment: false,
      releasedLabels: false,
    }],
  ],
};
