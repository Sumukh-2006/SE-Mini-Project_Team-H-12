# Proposed pull request descriptions

These descriptions accompany locally prepared work. They are drafts for the assigned teammates to review and adapt to their own actual changes. No PR has been created, no issue numbers exist yet, and no teammate authorship or owner approval is claimed. Replace requirement references with real issue links after the upstream backlog is published.

## Core product integration

Proposed coordinator: Sumukh Shandilya. Dependency group: 10f6641, ebdd314, e182d2c, 9233791, 8de0dd8, 059c32a. These six commits form the runnable Sprint-1 baseline; the initial build/interfaces commit alone is not buildable.

Title: Implement the HMS console core for Sprint 1

The HMS now supports administrator setup and account provisioning, role-specific login/logout, receptionist patient registration and search, administrator doctor registration and weekly hours, and receptionist appointment booking. Occupied or unavailable slots are rejected without creating a booking. The implementation uses C++17, local SQLite storage and separate presentation, business and data access layers.

Requirement coverage: FR-AUTH-01 through 04, FR-PAT-01 through 03, FR-DOC-01 through 04, FR-APP-01 and 02, SQLite initialization/reliability and security requirements. The owner should review the documented process-level login denial and the 15-minute slot convention.

Local validation: 18 GoogleTests and the actual console walkthrough passed. The prepared GitHub Actions workflow performs builds, tests, static checks and Valgrind. Hosted results and teammate contribution evidence are pending; attach actual PR-specific evidence before submitting.

## Patient contact updates

Assigned feature owner: Supreeth K. Prepared change: 98bc48d adds contact updates and the final service declarations. That commit's shared declarations also support later modules; only adopt the agreed contact change and necessary interfaces into a buildable branch.

Title: Allow receptionists to update patient contact details

Receptionists can update a registered patient's contact using its public patient ID. Invalid contacts and unknown patients produce an error; unauthorized roles are denied by the business service. The registration and existing search workflows remain available.

Requirement coverage: FR-PAT-04. Local final-suite verification includes Core.UpdateContact, existing patient tests and real console-output capture. Include the contact menu wiring and its tests with the actual proposed branch; do not submit an isolated service declaration without its dependencies.

## Appointment lifecycle and doctor schedules

Assigned feature owner: Shashank K. Prepared service change: e412548, with required declarations from 98bc48d and appropriate console/tests from 8191fda.

Title: Add cancellation rescheduling and doctor appointment workflows

Receptionists can cancel or reschedule Scheduled appointments. Cancellation preserves the record and frees its slot. A conflicting reschedule rolls back, preserving the original booking. Doctor accounts see their own daily schedules and may complete only their own Scheduled consultation. Availability changes that exclude existing Scheduled appointments are rejected.

Requirement coverage: FR-APP-03 through 05, FR-DOC-03 and DEV-COMPLETE. The completion action supplies the completed counts required by reporting and is documented as an architecture clarification.

Local final-suite validation: Core.TC_APP_03, Core.RescheduleIsAtomic, Core.DoctorsSeeOnlyOwnSchedule, Core.DailyCountsAndCompletion and Core.AvailabilityUpdates passed. Final recording also exercises the actual console workflows. Test the assembled PR branch before submitting; source-service commits need their UI and test dependencies.

## Reports and final verification

Assigned feature owner: Shashank D. Prepared changes: 80e4e19 and 8191fda, dependent on the complete core and appointment lifecycle. The final delivery commit adds the documentation, evidence and demo artifacts.

Title: Add administrator reports and complete HMS verification

Administrators can view daily appointment totals and export daily or patient reports as CSV/text. CSV fields are quoted correctly; invalid extensions, write failures and unauthorized exports are rejected. Daily totals use the current scheduled date and status, as documented.

Requirement coverage: FR-REP-01 through 03 and remaining non-functional verification. The suite retains the original 14 test cases and maps all 29 submitted requirements to concrete verification.

Local validation: all 29 GoogleTests, real console checks, cppcheck, Valgrind on both the suite and executable, crash rollback and performance/RSS checks passed. Actual raw evidence is under deliverables/evidence. These are local results; hosted CI, owner review and final team freeze remain pending.

## Review and submission

Every teammate submits through their own account with their actual changes, validation, issue links and dependencies. The upstream repository owner reviews and merges. Refresh descriptions and evidence if the proposed PR differs from this prepared integration package. Branch publication requires the user's explicit instruction; creating PRs or publishing the backlog requires its own authorization.
