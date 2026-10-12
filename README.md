# Hospital Management System

C++17 console application with local SQLite persistence, prepared as the Software Engineering mini-project Part 2 package for Team H-12.

On this workstation, double-click **Start-HMS.cmd** to run through Ubuntu in WSL. First run creates an administrator using credentials you choose. For another workstation, follow [build and setup instructions](docs/Part2/Setup.md).

Administrators manage doctors, weekly hours and user accounts and export reports. Receptionists register/search patients, update contacts and book/cancel/reschedule appointments. Doctors search records, see their own schedules and complete their own appointments. Service-side permissions, salted SHA-256 credentials, prepared SQL statements and transactional scheduling are implemented. Billing, pharmacy, online payments and audit logging are outside the delivered numbered-SRS scope.

See the [complete delivery index and checklist](deliverables/Deliverables.md), [backlog](deliverables/planning/Backlog.md), [traceability](deliverables/planning/Traceability.md), [architecture](docs/Part2/Architecture.md), [deviations](docs/Part2/Deviations.md) and [publication handoff](docs/Part2/Publishing.md).

| Updated document | File |
|---|---|
| Software Requirements Specification | [Version 1.1](docs/SRS/HMS_SRS_v1.1.docx) |
| Architecture and Design | [Version 1.1](docs/Architecture_Design/HMS_ArchDesign_v1.1.docx) |
| Test Plan and actual local results | [Version 1.1](docs/TestPlan/HMS_TestPlan_v1.1.docx) |

Original version 1.0 documents and diagrams are preserved. The SRS contains 20 functional, 6 non-functional and 3 security requirements. The delivered roles are Admin, Receptionist and Doctor; patients are records rather than a separate login role.

The local verification package records 29 passing GoogleTests, console and static checks, real process performance/RSS, crash recovery and Valgrind results. The videos replay real console output with captions and edited reading durations: [Sprint 1](deliverables/videos/Sprint-1-Demo.mp4), [Sprint 2](deliverables/videos/Sprint-2-Demo.mp4).

| Team member | Proposed Part-2 responsibility |
|---|---|
| Supreeth K | Patient features and user documentation |
| Shashank K | Database, doctors and appointments |
| Shashank D | Tests, traceability and reports |
| Sumukh Shandilya | Authentication, security, CI and demo tooling |

This package was prepared locally on 12 October 2026. Sprint 1 is scheduled for 19-23 October and Sprint 2 for 26-30 October. The local branch is `deliverables/part-2`; checkpoints are `sprint-1-local` and `sprint-2-local`. No push or GitHub changes have been made. Team-authored PRs, owner reviews, hosted CI, publication and the final freeze remain pending until genuinely performed and authorized.

Academic use: Software Engineering mini-project, PES University, Team H-12.
