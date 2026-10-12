# Hospital Management System Part 2 deliverables

Prepared locally on 12 October 2026 on branch `deliverables/part-2`. No GitHub writes, pushes, uploads, Projects, issues, PRs, approvals or merges were performed. This is an early local preparation, not a record of activity during the future scheduled sprint weeks.

## Start the application

On this Windows workstation, double-click `Start-HMS.cmd` in the repository folder. It starts the packaged Ubuntu executable through WSL. First run asks you to create an administrator; use your own credentials. See [Setup](../docs/Part2/Setup.md) for the exact build commands and optional fictional demonstration accounts.

## Delivered files

| Deliverable | Location | Local status |
|---|---|---|
| Final console application and source | bin/hms_app; ../src; ../include | Implemented and verified |
| Runnable Sprint-1 checkpoint | bin/hms_sprint1; local tag sprint-1-local | Implemented and verified |
| Backlog and issue drafts | planning/Backlog.md; planning/backlog.json; planning/issue-drafts | 39 proposed items prepared |
| Project setup | planning/project-setup.json | Prepared; GitHub creation held |
| Complete traceability | planning/Traceability.md | All 29 SRS requirements mapped |
| Updated SRS | ../docs/SRS/HMS_SRS_v1.1.docx | Reviewed rendered pages |
| Updated architecture | ../docs/Architecture_Design/HMS_ArchDesign_v1.1.docx | Reviewed rendered pages |
| Updated test plan and results | ../docs/TestPlan/HMS_TestPlan_v1.1.docx | Reviewed rendered pages |
| Build and verification workflow | ../.github/workflows/ci.yml | Prepared; hosted runs pending |
| Actual local test evidence | evidence/ | Passing outputs and metrics retained |
| Sprint-1 demonstration | videos/Sprint-1-Demo.mp4 | 60-second real console-output replay |
| Final demonstration | videos/Sprint-2-Demo.mp4 | 120-second real console-output replay |
| Deviations and publication handoff | ../docs/Part2/Deviations.md; ../docs/Part2/Publishing.md | Prepared |

## Local verification results

29 GoogleTests passed. Console checks and cppcheck passed. Valgrind reported zero errors and all heap blocks freed for the suite and the real application workflow. Crash injection verified rollback of an uncommitted production-DAL update and database integrity.
A 10,000-record database returned an ID query in 0.0552 seconds; a broad name query and 9,999 displayed results took 0.1360 seconds. Normal application RSS was 10076 KB, below 51,200 KB. These measurements apply to the documented Ubuntu WSL workstation.

## Sprint preparation reviews

Sprint 1 is planned for 19 to 23 October 2026 (83 proposed points). Its checkpoint implements authentication, user provisioning, patient registration/search, doctor profiles and weekly hours, and conflict-safe booking. Eighteen core tests and the actual console walkthrough passed. The video shows login, registration, search, specialization filtering, booking, conflict rejection and logout.

Sprint 2 is planned for 26 to 30 October 2026 (37 proposed points). Its checkpoint adds contact updates, cancellation, atomic rescheduling, doctor-owned daily schedules, consultation completion and report exports. The final test suite contains 29 tests plus process-level checks. The video exercises those final workflows and verifies an exported CSV file.

The checkpoints were implemented locally under the existing Git identity. They have not been represented as teammate commits or owner-reviewed sprint outcomes. Estimated points are for planning, not hours or demonstrated team velocity.

## Final checklist

- [x] Working C++17 console product with local SQLite storage and three layers
- [x] Local backlog with requirements, acceptance criteria, owners, points, dependencies and sprints
- [x] Tests, workstation NFR measurements, crash rollback and zero-leak evidence
- [x] Versioned updated documents; original v1.0 documents preserved
- [x] Captioned demonstration videos with transcripts, checkpoint IDs and binary hashes
- [x] Local branch, feature commits and runnable checkpoint tags
- [ ] Publish branch to the fork after explicit push authorization
- [ ] Create/link shared GitHub Project and issues after separate publication authorization
- [ ] Confirm Supreeth K and Shashank K GitHub usernames and real assignments
- [ ] Assigned teammates raise their own PRs; owner reviews and merges
- [ ] Hosted GitHub Actions runs, video/document uploads and actual team sprint reviews
- [ ] Owner-approved development freeze after Sprint 2 on 30 October

## Archive contents

The zip includes the source, original and revised documents, workflow, planning files, local evidence, both Linux executables, videos and setup/handoff guides. It excludes working databases, generated secrets, build directories, temporary QA pages and Git internals. The existing local repository retains its full Git history and tags. Nothing publishes automatically.
