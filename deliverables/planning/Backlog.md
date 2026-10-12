# Product backlog

Prepared locally on 12 October 2026. Estimates and assignments are proposals for team review. No issues or Project have been created.

| ID | Story | Owner | Sprint | Points |
|---|---|---|---|---|
| FR-AUTH-01 | allow users to log in using a unique username and password. | Sumukh Shandilya | 1 | 5 |
| FR-AUTH-02 | support three distinct roles: Admin, Receptionist, and Doctor. | Sumukh Shandilya | 1 | 3 |
| FR-AUTH-03 | deny access to unauthorized users after three incorrect login attempts. | Sumukh Shandilya | 1 | 3 |
| FR-AUTH-04 | enforce session termination upon explicit user logout. | Sumukh Shandilya | 1 | 2 |
| FR-PAT-01 | allow Receptionists to register a new patient with Name, Age, Gender, Contact, and Blood Group. | Supreeth K | 1 | 3 |
| FR-PAT-02 | generate a unique alphanumeric Patient ID upon successful registration. | Supreeth K | 1 | 2 |
| FR-PAT-03 | allow Receptionists and Doctors to search for a patient by Patient ID or Name. | Supreeth K | 1 | 3 |
| FR-PAT-04 | allow Receptionists to update patient contact details. | Supreeth K | 2 | 2 |
| FR-DOC-01 | allow Admins to add new doctors with Name, Specialization, and Contact Info. | Shashank K | 1 | 3 |
| FR-DOC-02 | generate a unique Doctor ID upon registration. | Shashank K | 1 | 2 |
| FR-DOC-03 | allow Admins to configure a doctor's weekly available hours. | Shashank K | 1 | 5 |
| FR-DOC-04 | allow Receptionists to view a list of all doctors filtered by Specialization. | Shashank K | 1 | 3 |
| FR-APP-01 | allow Receptionists to book an appointment by providing Patient ID, Doctor ID, Date, and Time. | Shashank K | 1 | 5 |
| FR-APP-02 | reject appointment bookings if the doctor is unavailable or already booked for the specified time. | Shashank K | 1 | 5 |
| FR-APP-03 | allow Receptionists to cancel an existing appointment using the Appointment ID. | Shashank K | 2 | 2 |
| FR-APP-04 | allow Receptionists to reschedule an appointment, subject to doctor availability. | Shashank K | 2 | 5 |
| FR-APP-05 | allow Doctors to view their scheduled appointments for a specific date. | Shashank K | 2 | 3 |
| FR-REP-01 | allow Admins to generate a daily report of total appointments booked, canceled, and completed. | Shashank D | 2 | 3 |
| FR-REP-02 | allow Admins to generate a report of all registered patients. | Shashank D | 2 | 3 |
| FR-REP-03 | export reports as basic text/CSV files to the local disk. | Shashank D | 2 | 3 |
| NFR-PERF-01 | process queries and display results within 2 seconds for a database of up to 10,000 records. | Shashank D | 2 | 3 |
| NFR-PERF-02 | The system footprint shall not exceed 50 MB in RAM during normal operations. | Shashank D | 2 | 3 |
| NFR-REL-01 | prevent data corruption in the event of an abrupt application crash (ACID compliance via SQLite). | Shashank K | 1 | 5 |
| NFR-REL-02 | handle missing data files gracefully by creating necessary empty files on startup. | Shashank K | 1 | 3 |
| SEC-REQ-01 | Passwords shall not be stored in plain text (must be hashed, e.g., SHA-256). | Sumukh Shandilya | 1 | 5 |
| SEC-REQ-02 | use parameterized queries for all SQLite interactions to prevent SQL Injection. | Sumukh Shandilya | 1 | 3 |
| SEC-REQ-03 | Role-based access checks shall occur server/backend-side before executing any sensitive operation. | Sumukh Shandilya | 1 | 5 |
| NFR-MAINT-01 | be documented using Doxygen-style comments. | Shashank D | 2 | 1 |
| NFR-MAINT-02 | Memory leaks shall be verified as zero using Valgrind before release. | Shashank D | 2 | 3 |
| DEV-BUILD | Reproducible C++17 CMake build | Shashank K | 1 | 3 |
| DEV-CI | Prepare pull-request and push checks | Sumukh Shandilya | 1 | 3 |
| DEV-TEST | Automated tests and console verification | Shashank D | 1 | 5 |
| DEV-USER | First-run and administrator user provisioning | Sumukh Shandilya | 1 | 3 |
| DEV-COMPLETE | Doctor marks a consultation completed | Shashank K | 2 | 2 |
| GITHUB-BOARD | Create and link the shared GitHub Project | Sumukh Shandilya | 1 | 2 |
| DOC-PACKAGE | Finalize versioned documents and traceability | Supreeth K | 2 | 1 |
| DEMO-1 | Record the 60-second Sprint-1 product demo | Sumukh Shandilya | 1 | 2 |
| DEMO-2 | Record the 120-second final product demo | Sumukh Shandilya | 2 | 2 |
| REL-FREEZE | Prepare the development freeze and release checklist | Shashank D | 2 | 1 |

## Sprint estimates

Sprint 1: 24 items, 83 points. Points describe relative complexity, not hours or proven team velocity.
Sprint 2: 15 items, 37 points. Points describe relative complexity, not hours or proven team velocity.
