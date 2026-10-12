# Build and run the Hospital Management System

The project is a C++17 console application with a local SQLite database. The submitted version 1.0 documents remain unchanged; version 1.1 records the implementation details and added test coverage.

## Windows using Ubuntu in WSL

Ubuntu 24.04 is installed on this workstation. Open Windows Terminal and run `wsl -d Ubuntu-24.04`. No changes were made to Docker's internal distribution or to the WSL default distribution.

Inside Ubuntu, enter the checkout directory:

```sh
cd /mnt/c/Users/Shashank/.codex/.chatgpt-projects/g-p-6ac9a2734540819188fab92cbe1c7192/work
```

For another workstation, clone the repository and enter its directory instead. Install the development dependencies inside Ubuntu:

```sh
sudo apt-get update
sudo apt-get install -y build-essential cmake libsqlite3-dev libssl-dev libgtest-dev cppcheck valgrind python3-pexpect time
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
./build/hms_app
```

The first run asks you to create an administrator. Choose your own username and a password of 8 to 128 characters. Login as that administrator, add doctors and weekly availability, and create Receptionist and Doctor user accounts. Each Doctor account must link to its Doctor ID. No production credentials are included in the repository.

The database defaults to `.local/hms.db`. Select another file with `./build/hms_app --db .local/example.db`. Parent directories and an empty schema are created on startup. Existing data is preserved. Keep the database on a local disk. The app uses only Admin, Receptionist and Doctor logins; patients are managed records, not a fourth login role.

## Optional fictional demonstration database

Use a new file, separate from working records. The following prompts for a password without saving it in source control or showing it in terminal history:

```sh
read -rs -p 'Choose a demonstration password: ' HMS_DEMO_PASSWORD; echo
export HMS_DEMO_PASSWORD
./build/hms_app --db .local/demo.db --demo-seed
unset HMS_DEMO_PASSWORD
./build/hms_app --db .local/demo.db
```

The fictional accounts are `admin`, `reception`, and `doctor`, using the password you supplied. The sample patient is `P000001`, the sample doctor is `1`, and availability is 09:00 to 17:00 every day. Seeding refuses a database that already contains users.

## Working with the console

- Administrator: add doctors, set weekly hours, create user accounts, inspect daily totals, and export patient or daily reports.
- Receptionist: register/search patients, update contact details, filter doctors, book/cancel/reschedule appointments.
- Doctor: search patient records, view their own daily schedule, and complete their own consultations.
- Enter `0` to logout and `q` at the username prompt to quit. Three failed logins end the process with access denied. A new process starts a new authentication attempt session.

Dates use `YYYY-MM-DD`. Times use `HH:MM`. Appointments last 15 minutes and must start on a quarter-hour boundary. Sunday is weekday 0. The availability end time is exclusive for slot starts; 16:45 is the final slot in a 09:00 to 17:00 schedule. A reschedule keeps the same patient and doctor and changes the date/time only. Cancellation preserves history and releases the slot. Completed appointments retain their occupied slots.

Search matches an exact patient ID or a case-insensitive name substring. Ages range from 0 to 130. Gender values are Male, Female and Other. Blood groups are A+, A-, B+, B-, AB+, AB-, O+, O-. Contacts contain 7 to 15 digits, with optional +, - and spaces within a 15-character field.

Report filenames must end in `.csv` or `.txt`. Export uses the path you enter and replaces an existing report at that path. Daily totals count appointments whose current scheduled date matches the requested date, with current Scheduled/Canceled/Completed status; these are not historical booking-event totals.

## Verification

```sh
ctest --test-dir build --output-on-failure
python3 tools/verify_local.py
```

The verifier stores actual output under `deliverables/evidence/`. It checks the core suite, real console behavior, static analysis, Valgrind, crash rollback and workstation performance. GitHub Actions use the same toolchain; no hosted run has been performed while publication is held.

## Sprint checkpoints

`sprint-1-local` identifies the runnable core product. To build it without changing this checkout, create a separate Git worktree at that tag and use the same CMake commands there. `sprint-2-local` identifies the completed product code. Local tags and commits have not been pushed.

## Recreating videos

Install `ffmpeg`, `fonts-dejavu-core`, `python3-pexpect` and `python3-pil` in Ubuntu. Run `tools/record_demo.py` with the appropriate checkpoint binary, `--sprint 1` or `--sprint 2`, and `--checkpoint` identifying its Git tag. Each video replays genuine terminal output with captions and edited scene hold times. It has no narration. The transcript and binary hash accompany the video.
