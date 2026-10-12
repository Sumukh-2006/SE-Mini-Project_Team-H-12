# Publication and team handoff

Nothing in this package publishes automatically. The user's explicit instruction is to hold pushes and all GitHub changes until requested.

## Repository destinations

- Origin: https://github.com/shashankd06/SE-Mini-Project_Team-H-12.git
- Upstream: https://github.com/Sumukh-2006/SE-Mini-Project_Team-H-12.git
- Local implementation branch: deliverables/part-2
- Local checkpoints: sprint-1-local and sprint-2-local

When a push is requested, recheck the destination and publish only the approved branch and approved commits to the user's fork. Do not push directly to upstream or main. Do not automatically create a PR, publish issues, or merge merely because a branch push was authorized.

## Shared Project publication

After authorization to create the board and issues, use `deliverables/planning/project-setup.json` and `backlog.json`. Sumukh must own the Project and link it to the upstream team repository. Confirm the GitHub usernames for Supreeth K and Shashank K; they are intentionally unset in the manifest. Confirm team access before applying real assignments.

Create requirement issues in the team repository and add them to the Project. Each issue draft already has its requirement, acceptance criteria, dependencies, proposed owner, sprint and story points. Set the Project number field Story Points and iteration field Sprint; labels alone do not substitute for these fields. Create the two five-day iterations starting 19 October and 26 October 2026, with Backlog, Status board, Sprint 1 and Sprint 2 views. Use actual current status; do not imply that an assigned teammate has already completed or reviewed work.

The backlog requirement is due on 12 October. Its remote completion cannot be claimed while publication remains held. The optional dry run is 12 to 16 October; entries may still be revised on 19 October according to the assignment.

## Genuine teammate contribution workflow

All local implementation commits were prepared in this session under the existing local Git identity. They are not attributed to other teammates. Each teammate reviews their proposed module work, makes or adopts changes on their own branch, tests it, and raises their own PR. For existing prepared changes, use explicit, agreed commit cherry-picks or module patches; do not cherry-pick an entire integration branch multiple times into overlapping PRs.

The local commits form an integration history; early individual commits are not independently buildable because the build configuration references later modules. Do not submit the first foundation commit alone as a passing PR. Coordinate the complete core dependency group, then contact updates and appointment lifecycle, reports, final evidence and documentation. For separate teammate PRs, establish a buildable shared scaffold and transfer the agreed module changes with their dependencies. Each teammate must test the complete proposed PR and make or adopt their actual changes. The owner reviews each PR and merges after checks pass. Sumukh's own changes need another appropriate reviewer where possible. No review comments, approvals, merges or completion dates have been fabricated.

GitHub Actions will run on pushes and PRs after the workflow is published. Enable Actions in the fork and team repository as required. Fork PR checks may need owner approval to run. The workflow grants read-only content access and performs build/tests, console/system checks, cppcheck and Valgrind; it does not create commits, PRs, approvals or merges.

## Review template

PR title: HMS <module> for Sprint <number>

Description: State the concrete feature, linked upstream issue IDs, what the user can now do, relevant requirement IDs, test commands/results, and any documented deviation. Include the teammate's actual changes and evidence. Request owner review. Replace placeholders with real issue links only after issues exist.

## Upload and freeze

Upload the one-minute video from the Sprint-1 checkpoint, then the two-minute final video and completed documents, when upload is authorized. Use the actual sprint outcome after team integration; refresh the recordings if team changes alter the product. The current recordings are local product preparations.

On 30 October, after the planned second sprint and owner review, freeze development and record the final agreed commit. The local ready-to-review snapshot is not an owner-approved development freeze or published release.
