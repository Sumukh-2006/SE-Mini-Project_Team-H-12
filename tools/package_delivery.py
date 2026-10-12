#!/usr/bin/env python3
"""Build a local delivery archive from versioned files; never pushes or uploads."""
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT.parent/'output';OUT.mkdir(exist_ok=True)
SUMMARY=json.loads((ROOT/'deliverables/evidence/verification-summary.json').read_text())
METRICS=json.loads((ROOT/'deliverables/evidence/system-metrics.json').read_text())
backlog=json.loads((ROOT/'deliverables/planning/backlog.json').read_text())
if any(c['exit_code'] for c in SUMMARY['checks']):raise SystemExit('Do not package failing verification')

def write_overview():
    totals={i:sum(s['story_points'] for s in backlog if s['sprint']==i) for i in [1,2]}
    lines=[
        '# Hospital Management System Part 2 deliverables','',
        'Prepared locally on 12 October 2026 on branch `deliverables/part-2`. No GitHub writes, pushes, uploads, Projects, issues, PRs, approvals or merges were performed. This is an early local preparation, not a record of activity during the future scheduled sprint weeks.','',
        '## Start the application','',
        'On this Windows workstation, double-click `Start-HMS.cmd` in the repository folder. It starts the packaged Ubuntu executable through WSL. First run asks you to create an administrator; use your own credentials. See [Setup](../docs/Part2/Setup.md) for the exact build commands and optional fictional demonstration accounts.','',
        '## Delivered files','',
        '| Deliverable | Location | Local status |','|---|---|---|',
        '| Final console application and source | bin/hms_app; ../src; ../include | Implemented and verified |',
        '| Runnable Sprint-1 checkpoint | bin/hms_sprint1; local tag sprint-1-local | Implemented and verified |',
        '| Backlog and issue drafts | planning/Backlog.md; planning/backlog.json; planning/issue-drafts | 39 proposed items prepared |',
        '| Project setup | planning/project-setup.json | Prepared; GitHub creation held |',
        '| Complete traceability | planning/Traceability.md | All 29 SRS requirements mapped |',
        '| Updated SRS | ../docs/SRS/HMS_SRS_v1.1.docx | Reviewed rendered pages |',
        '| Updated architecture | ../docs/Architecture_Design/HMS_ArchDesign_v1.1.docx | Reviewed rendered pages |',
        '| Updated test plan and results | ../docs/TestPlan/HMS_TestPlan_v1.1.docx | Reviewed rendered pages |',
        '| Build and verification workflow | ../.github/workflows/ci.yml | Prepared; hosted runs pending |',
        '| Actual local test evidence | evidence/ | Passing outputs and metrics retained |',
        '| Sprint-1 demonstration | videos/Sprint-1-Demo.mp4 | 60-second real console-output replay |',
        '| Final demonstration | videos/Sprint-2-Demo.mp4 | 120-second real console-output replay |',
        '| Deviations and publication handoff | ../docs/Part2/Deviations.md; ../docs/Part2/Publishing.md | Prepared |','',
        '## Local verification results','',
        f"{SUMMARY['tests']} GoogleTests passed. Console checks and cppcheck passed. Valgrind reported zero errors and all heap blocks freed for the suite and the real application workflow. Crash injection verified rollback of an uncommitted production-DAL update and database integrity.",
        f"A 10,000-record database returned an ID query in {METRICS['query_wall_seconds']:.4f} seconds; a broad name query and 9,999 displayed results took {METRICS['broad_query_and_display_seconds']:.4f} seconds. Normal application RSS was {METRICS['normal_process_rss_kb']} KB, below 51,200 KB. These measurements apply to the documented Ubuntu WSL workstation.",'',
        '## Sprint preparation reviews','',
        f"Sprint 1 is planned for 19 to 23 October 2026 ({totals[1]} proposed points). Its checkpoint implements authentication, user provisioning, patient registration/search, doctor profiles and weekly hours, and conflict-safe booking. Eighteen core tests and the actual console walkthrough passed. The video shows login, registration, search, specialization filtering, booking, conflict rejection and logout.",'',
        f"Sprint 2 is planned for 26 to 30 October 2026 ({totals[2]} proposed points). Its checkpoint adds contact updates, cancellation, atomic rescheduling, doctor-owned daily schedules, consultation completion and report exports. The final test suite contains 29 tests plus process-level checks. The video exercises those final workflows and verifies an exported CSV file.",'',
        'The checkpoints were implemented locally under the existing Git identity. They have not been represented as teammate commits or owner-reviewed sprint outcomes. Estimated points are for planning, not hours or demonstrated team velocity.','',
        '## Final checklist','',
        '- [x] Working C++17 console product with local SQLite storage and three layers',
        '- [x] Local backlog with requirements, acceptance criteria, owners, points, dependencies and sprints',
        '- [x] Tests, workstation NFR measurements, crash rollback and zero-leak evidence',
        '- [x] Versioned updated documents; original v1.0 documents preserved',
        '- [x] Captioned demonstration videos with transcripts, checkpoint IDs and binary hashes',
        '- [x] Local branch, feature commits and runnable checkpoint tags',
        '- [ ] Publish branch to the fork after explicit push authorization',
        '- [ ] Create/link shared GitHub Project and issues after separate publication authorization',
        '- [ ] Confirm Supreeth K and Shashank K GitHub usernames and real assignments',
        '- [ ] Assigned teammates raise their own PRs; owner reviews and merges',
        '- [ ] Hosted GitHub Actions runs, video/document uploads and actual team sprint reviews',
        '- [ ] Owner-approved development freeze after Sprint 2 on 30 October','',
        '## Archive contents','',
        'The zip includes the source, original and revised documents, workflow, planning files, local evidence, both Linux executables, videos and setup/handoff guides. It excludes working databases, generated secrets, build directories, temporary QA pages and Git internals. The existing local repository retains its full Git history and tags. Nothing publishes automatically.','']
    (ROOT/'deliverables/Deliverables.md').write_text('\n'.join(lines))
    readme='''# Hospital Management System

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
'''
    (ROOT/'README.md').write_text(readme)

def archive():
    paths=subprocess.check_output(['git','ls-files','-z'],cwd=ROOT).decode().split('\0')
    paths=[p for p in paths if p and (ROOT/p).is_file() and Path(p).name!='.DS_Store']
    archive_path=OUT/'HMS-Part2-Local-Package.zip'
    with zipfile.ZipFile(archive_path,'w',zipfile.ZIP_DEFLATED) as zip:
        for path in paths:zip.write(ROOT/path,'HMS-Part2/'+path)
    with zipfile.ZipFile(archive_path) as zip:
        assert zip.testzip() is None
        assert 'HMS-Part2/deliverables/videos/Sprint-2-Demo.mp4' in zip.namelist()
        assert not any('/.local/' in p or '/.git/' in p or p.endswith('.db') for p in zip.namelist())
    (OUT/'HMS-Part2-Local-Package.sha256').write_text(hashlib.sha256(archive_path.read_bytes()).hexdigest()+'  '+archive_path.name+'\n')
    print(f'Packaged {len(paths)} files: {archive_path}')

if __name__=='__main__':
    import sys
    if '--archive' in sys.argv:archive()
    else:write_overview()
