# Implemented architecture

The console calls the HMS business services, which enforce the private session's role and validate inputs before using the SQLite data access layer. Public interfaces and ownership rules are documented with Doxygen-style comments in `include/hms/hms.hpp` and `include/hms/database.hpp`. The complete design specification is `docs/Architecture_Design/HMS_ArchDesign_v1.1.docx`.

```mermaid
flowchart TD
    UI[Console presentation] --> AUTH[Authentication]
    UI --> PAT[Patients]
    UI --> DOC[Doctors]
    UI --> APP[Appointments]
    UI --> REP[Reports]
    AUTH --> DAL[Database / Statement / Transaction]
    PAT --> DAL
    DOC --> DAL
    APP --> DAL
    REP --> DAL
    DAL --> SQL[(Local SQLite)]
```

```mermaid
erDiagram
    DOCTORS ||--o| USERS : login
    DOCTORS ||--o{ AVAILABILITY : weekly_hours
    DOCTORS ||--o{ APPOINTMENTS : consultations
    PATIENTS ||--o{ APPOINTMENTS : bookings
    USERS {
        int id PK
        string username UK
        string password_hash
        string salt
        string role
        int doctor_id FK
    }
    PATIENTS {
        int id PK
        string public_id UK
        string name
        int age
        string gender
        string contact
        string blood_group
    }
    DOCTORS {
        int id PK
        string name
        string specialization
        string contact
    }
    AVAILABILITY {
        int doctor_id PK,FK
        int weekday PK
        int start_min
        int end_min
    }
    APPOINTMENTS {
        int id PK
        int patient_id FK
        int doctor_id FK
        string appt_date
        string appt_time
        string status
    }
```

```mermaid
sequenceDiagram
    actor R as Receptionist
    participant UI as Console
    participant B as Appointment service
    participant DB as SQLite DAL
    R->>UI: Book patient / doctor / date / time
    UI->>B: book(...)
    B->>B: Check role and input
    B->>DB: BEGIN IMMEDIATE
    B->>DB: Resolve patient, hours and occupied slot
    alt Available
        B->>DB: Bound INSERT, COMMIT
        B-->>UI: Scheduled appointment
        UI-->>R: Appointment ID and confirmation
    else Invalid or occupied
        B->>DB: ROLLBACK
        B-->>UI: Error
        UI-->>R: Clear rejection
    end
```

```mermaid
sequenceDiagram
    actor R as Receptionist
    participant B as Appointment service
    participant DB as SQLite DAL
    R->>B: Reschedule appointment / date / time
    B->>B: Check role and calendar
    B->>DB: BEGIN IMMEDIATE; read Scheduled record
    B->>DB: Check availability excluding this appointment
    alt Available
        B->>DB: Bound UPDATE; COMMIT
        B-->>R: Rescheduled
    else Conflict or validation failure
        B->>DB: ROLLBACK
        B-->>R: Rejected; original booking preserved
    end
```
