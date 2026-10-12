# Requirements traceability matrix

All 29 submitted requirements are mapped below. Test outcomes are in ../evidence; this is local verification, not hosted CI or PR review evidence.

| Requirement | Component | Verification |
|---|---|---|
| FR-AUTH-01 | Auth service | Core.TC_AUTH_01 |
| FR-AUTH-02 | Auth service | Core.RolesEnforcedInServices |
| FR-AUTH-03 | Auth service | Core.TC_AUTH_02 |
| FR-AUTH-04 | Auth service | Core.TC_AUTH_03 |
| FR-PAT-01 | Patient service | Core.TC_PAT_01 |
| FR-PAT-02 | Patient service | Core.PatientValidationAndUniqueIds |
| FR-PAT-03 | Patient service | Core.TC_PAT_02; Core.TC_PAT_03; Core.SearchByNameAndLiteralWildcards |
| FR-PAT-04 | Patient service | Core.UpdateContact |
| FR-DOC-01 | Doctor service | Core.TC_DOC_01 |
| FR-DOC-02 | Doctor service | Core.TC_DOC_01 |
| FR-DOC-03 | Doctor service | Core.AvailabilityAndCalendarValidation; Core.AvailabilityUpdates |
| FR-DOC-04 | Doctor service | Core.TC_DOC_02 |
| FR-APP-01 | Appointment service | Core.TC_APP_01 |
| FR-APP-02 | Appointment service | Core.TC_APP_02; Core.AvailabilityAndCalendarValidation |
| FR-APP-03 | Appointment service | Core.TC_APP_03 |
| FR-APP-04 | Appointment service | Core.RescheduleIsAtomic |
| FR-APP-05 | Appointment service | Core.DoctorsSeeOnlyOwnSchedule |
| FR-REP-01 | Report service | Core.DailyCountsAndCompletion |
| FR-REP-02 | Report service | Core.ReportContents |
| FR-REP-03 | Report service | Core.ReportContents; Core.ReportErrorsAndPermissions |
| NFR-PERF-01 | Business layer and DAL | Core.TC_NFR_01 |
| NFR-PERF-02 | Business layer and DAL | tests/system_checks.py: normal process RSS |
| NFR-REL-01 | SQLite DAL | tests/system_checks.py: abrupt process termination |
| NFR-REL-02 | SQLite DAL | Recovery.MissingDatabaseAndPersistence |
| SEC-REQ-01 | Auth and DAL | Core.HashesAreSaltedAndNeverPlaintext |
| SEC-REQ-02 | Auth and DAL | Core.TC_SEC_01; Core.SearchByNameAndLiteralWildcards |
| SEC-REQ-03 | Auth and DAL | Core.RolesEnforcedInServices; Core.ReportErrorsAndPermissions |
| NFR-MAINT-01 | Source and verification tools | include/hms/*.hpp: Doxygen comments; docs/Part2/Architecture.md |
| NFR-MAINT-02 | Source and verification tools | Valgrind hms_tests and real console workflow |
