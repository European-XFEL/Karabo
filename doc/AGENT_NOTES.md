 Task Description

  Prepare a whatsnew release-note entry for version <VERSION>, covering the range from the previous major/minor starting point up to HEAD, and write it in the same rst style as the
  existing files under doc/whatsnew.

  Input parameter:

  - <VERSION>: the target changelog version, for example 3.1, 3.2, or 4.0

  Requirements:

  - Create doc/whatsnew/<VERSION>.rst.
  - Register the new file in doc/whatsnew/index.rst.
  - Include a short curated summary at the top with user-friendly wording mixed with technical detail.
  - Mention the total number of merge requests covered in the entry description.
  - Attach GitLab MR numbers with links.
  - Do not omit any merge request from the relevant git range.
  - Add a complete merge-log section that is exhaustive.
  - Group the exhaustive section primarily by conventional-commit categories:
    Features, Bug Fixes, Documentation, Performance, Refactor, Testing, and Miscellaneous Tasks.

  - For MRs that do not follow conventional commits, classify them on a best-effort basis from the title text.
  - Sort entries within each group by scope/category and then by message.
  - Keep the top summary curated and readable, but keep the exhaustive section complete and traceable.
  - For python dependency changes in extern/requirements-*.txt, highlight them explicitly
  - For C++ dependency changes in extern/conan* , highlight them explicitly

  Suggested execution steps:

  1. Determine the git range for <VERSION>.
  2. Extract all merge requests in that range.
  3. Count them and use that count in the changelog intro.
  4. Build curated highlights.
  5. Build the exhaustive merge log with full MR coverage.
  6. Verify:
      - all git MRs are present
      - no extra MR links were introduced
      - grouped highlight bullets do not hide unrelated issues

  Summary Template

  Create or update if present a doc/whatsnew/<VERSION>.rst with:

  - a concise release summary
  - selective curated highlights in user-language
  - MR-linked grouped changelog sections
  - a concise dependency changes summary (previous and now) and in a rst table format
  - an exhaustive complete merge log
  - verified full MR coverage for the chosen range.

