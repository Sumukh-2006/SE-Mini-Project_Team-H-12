# Implementation clarifications and deviations

The version 1.0 submission is preserved. These additions close gaps between its requirements and architecture; the application remains a C++ console program with local SQLite storage. They should be explained during the final demo.

| Topic | Submitted baseline | Implemented clarification | Reason |
|---|---|---|---|
| Storage | SRS permits local files or SQLite | SQLite only, foreign keys, WAL and full synchronization | Follows the SAD and supports crash rollback |
| Patient ID | SRS requires alphanumeric; SAD shows integer key | Integer primary key plus unique public ID such as P000001 | Satisfies both documents |
| Availability | Weekly hours required but missing from ERD | Availability table indexed by doctor and weekday | Needed to enforce booking rules |
| Doctor login | Three roles, but no user-to-doctor association | Unique Users.doctor_id foreign key for Doctor accounts | Restricts schedules and completion to the logged-in doctor |
| Completed reports | Completed totals required, with no completion action | Linked doctor can complete their Scheduled appointment | Gives completed totals a real source |
| Password hashing | SHA-256 in SAD | Random 128-bit salt plus SHA-256 using OpenSSL | Preserves the specified algorithm while preventing identical hashes for identical passwords; not a production-grade slow password KDF |
| Login failures | Account/session lock or denial after three failures | Process-level attempt session ends after three failures | Matches the submitted test alternative; restarting begins a new attempt session |
| Slot rules | No slot duration specified | Fixed 15-minute appointments and quarter-hour starts | Makes overlap validation deterministic |
| Availability updates | Behavior for existing appointments unspecified | Reject changes that exclude an existing Scheduled appointment | Prevents invalidating booked consultations |
| Provisioning | Administrator manages users; workflow unspecified | First-run administrator plus authenticated user creation | Provides usable logins without hardcoded production accounts |
| Reports | Daily booked/canceled/completed totals | Current scheduled date and current status; booked includes all three statuses | Clarifies totals and reschedule behavior |
| Test coverage | 14 test cases and partial RTM | Retain those cases and add full 29-requirement mapping | Verifies requirements omitted from the initial plan |
| README claims | Four actors and selected rows marked Verified | Three login roles; verification backed by actual evidence | Patients are records and original design claims were not execution evidence |

No billing, pharmacy, online payments, network server or graphical/web interface was added. Audit logging mentioned in the original README was not a numbered SRS requirement and has not been implemented; the corrected README describes the delivered scope.

The videos and reviews are early local preparations made on 12 October 2026. They are not evidence of team activity during the future scheduled sprint weeks. Hosted CI, board publication, assignee account confirmation, teammate PRs, owner reviews, merges and uploads remain pending.
