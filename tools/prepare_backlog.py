#!/usr/bin/env python3
"""Prepare local issue drafts from the submitted SRS; never calls GitHub."""
import json
import re
from pathlib import Path
from docx import Document

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"deliverables"/"planning"
NAMES={"AUTH":"Sumukh Shandilya","PAT":"Supreeth K","DOC":"Shashank K","APP":"Shashank K","REP":"Shashank D","PERF":"Shashank D","REL":"Shashank K","MAINT":"Shashank D","SEC":"Sumukh Shandilya"}
TESTS={
"FR-AUTH-01":"Core.TC_AUTH_01", "FR-AUTH-02":"Core.RolesEnforcedInServices", "FR-AUTH-03":"Core.TC_AUTH_02", "FR-AUTH-04":"Core.TC_AUTH_03",
"FR-PAT-01":"Core.TC_PAT_01", "FR-PAT-02":"Core.PatientValidationAndUniqueIds", "FR-PAT-03":"Core.TC_PAT_02; Core.TC_PAT_03; Core.SearchByNameAndLiteralWildcards", "FR-PAT-04":"Core.UpdateContact",
"FR-DOC-01":"Core.TC_DOC_01", "FR-DOC-02":"Core.TC_DOC_01", "FR-DOC-03":"Core.AvailabilityAndCalendarValidation; Core.AvailabilityUpdates", "FR-DOC-04":"Core.TC_DOC_02",
"FR-APP-01":"Core.TC_APP_01", "FR-APP-02":"Core.TC_APP_02; Core.AvailabilityAndCalendarValidation", "FR-APP-03":"Core.TC_APP_03", "FR-APP-04":"Core.RescheduleIsAtomic", "FR-APP-05":"Core.DoctorsSeeOnlyOwnSchedule",
"FR-REP-01":"Core.DailyCountsAndCompletion", "FR-REP-02":"Core.ReportContents", "FR-REP-03":"Core.ReportContents; Core.ReportErrorsAndPermissions",
"NFR-PERF-01":"Core.TC_NFR_01", "NFR-PERF-02":"tests/system_checks.py: normal process RSS", "NFR-REL-01":"tests/system_checks.py: abrupt process termination", "NFR-REL-02":"Recovery.MissingDatabaseAndPersistence",
"NFR-MAINT-01":"include/hms/*.hpp: Doxygen comments; docs/Part2/Architecture.md", "NFR-MAINT-02":"Valgrind hms_tests and real console workflow", "SEC-REQ-01":"Core.HashesAreSaltedAndNeverPlaintext", "SEC-REQ-02":"Core.TC_SEC_01; Core.SearchByNameAndLiteralWildcards", "SEC-REQ-03":"Core.RolesEnforcedInServices; Core.ReportErrorsAndPermissions"}

def main():
    OUT.mkdir(parents=True,exist_ok=True);drafts=OUT/"issue-drafts";drafts.mkdir(exist_ok=True)
    stories=[]
    for table in Document(ROOT/"docs/SRS/HMS_SRS_v1.0.docx").tables:
        for row in table.rows:
            cells=[c.text.strip() for c in row.cells]
            if not cells or not re.fullmatch(r"(?:FR-[A-Z]+|NFR-[A-Z]+|SEC-REQ)-\d{2}",cells[0]):continue
            rid,description=cells[:2];group=rid.split("-")[1]
            sprint=2 if rid in {"FR-PAT-04","FR-APP-03","FR-APP-04","FR-APP-05"} or group in {"REP","PERF","MAINT"} else 1
            points=5 if rid in {"FR-AUTH-01","FR-DOC-03","FR-APP-01","FR-APP-02","FR-APP-04","NFR-REL-01","SEC-REQ-01","SEC-REQ-03"} else 3
            if rid in {"FR-AUTH-04","FR-PAT-02","FR-PAT-04","FR-DOC-02","FR-APP-03"}:points=2
            if rid=="NFR-MAINT-01":points=1
            dependencies=[] if group=="AUTH" else ["DEV-BUILD","FR-AUTH-02"]
            if group=="PAT" and rid!="FR-PAT-01":dependencies.append("FR-PAT-01")
            if group=="DOC" and rid!="FR-DOC-01":dependencies.append("FR-DOC-01")
            if group=="APP":dependencies+= ["FR-PAT-01","FR-DOC-03"]
            if group=="REP":dependencies.append("FR-APP-01")
            if rid=="FR-REP-01":dependencies.append("DEV-COMPLETE")
            stories.append({"id":rid,"title":description.replace("The system shall ","").replace("The application shall ","").replace("The source code shall ",""),"requirement":description,"owner":NAMES["SEC" if rid.startswith("SEC") else group],"sprint":sprint,"story_points":points,"priority":"High" if sprint==1 else "Medium","dependencies":dependencies,"acceptance_criteria":[description,"Verification: "+TESTS[rid]],"status":"Prepared locally; not published","tests":TESTS[rid]})
    assert len(stories)==29 and len({s['id'] for s in stories})==29
    extras=[
        ("DEV-BUILD","Reproducible C++17 CMake build",1,3,"Shashank K","Configure, compile and run in Ubuntu 24.04 with SQLite and OpenSSL"),
        ("DEV-CI","Prepare pull-request and push checks",1,3,"Sumukh Shandilya","Build, tests, console checks, cppcheck, and Valgrind workflow files ready; hosted run pending publication"),
        ("DEV-TEST","Automated tests and console verification",1,5,"Shashank D","GoogleTest and real PTY workflow tests execute against the actual application"),
        ("DEV-USER","First-run and administrator user provisioning",1,3,"Sumukh Shandilya","First run can create only the initial administrator; only authenticated admins add subsequent users"),
        ("DEV-COMPLETE","Doctor marks a consultation completed",2,2,"Shashank K","Only the linked doctor can transition their Scheduled appointment to Completed"),
        ("GITHUB-BOARD","Create and link the shared GitHub Project",1,2,"Sumukh Shandilya","After publication approval, create board with points, assignees, priorities, sprint fields and views"),
        ("DOC-PACKAGE","Finalize versioned documents and traceability",2,1,"Supreeth K","Version 1.1 SRS, architecture, test plan and full requirement mappings available; preserve v1.0"),
        ("DEMO-1","Record the 60-second Sprint-1 product demo",1,2,"Sumukh Shandilya","Actual Sprint-1 console output captured with captions and verified 60-second duration"),
        ("DEMO-2","Record the 120-second final product demo",2,2,"Sumukh Shandilya","Actual final console output captured with captions and verified 120-second duration"),
        ("REL-FREEZE","Prepare the development freeze and release checklist",2,1,"Shashank D","Local verification complete; hosted evidence and owner approval remain pending until performed")]
    for rid,title,sprint,points,owner,accept in extras:
        stories.append(dict(id=rid,title=title,requirement="Supporting deliverable",owner=owner,sprint=sprint,story_points=points,priority="High",dependencies=[] if rid=="DEV-BUILD" else ["DEV-BUILD"],acceptance_criteria=[accept],status="Prepared locally; not published",tests="See delivery checklist"))
    (OUT/"backlog.json").write_text(json.dumps(stories,indent=2)+"\n",encoding="utf-8")
    table=["# Product backlog","","Prepared locally on 12 October 2026. Estimates and assignments are proposals for team review. No issues or Project have been created.","","| ID | Story | Owner | Sprint | Points |","|---|---|---|---|---|"]
    for s in stories:
        table.append(f"| {s['id']} | {s['title']} | {s['owner']} | {s['sprint']} | {s['story_points']} |")
        body=f"# {s['id']} {s['title']}\n\nOwner: {s['owner']}\nSprint: {s['sprint']}\nStory points: {s['story_points']}\nPriority: {s['priority']}\n\n## Requirement\n\n{s['requirement']}\n\n## Acceptance criteria\n\n"+"\n".join("- "+x for x in s['acceptance_criteria'])+"\n\n## Dependencies\n\n"+(", ".join(s['dependencies']) or "None")+"\n\n## Contribution evidence\n\nAssigned teammate raises their PR; repository owner reviews and merges. Links remain pending until actual publication.\n"
        (drafts/(s['id']+".md")).write_text(body,encoding="utf-8")
    table += ["", "## Sprint estimates", ""]
    for sprint in [1,2]:
        batch=[s for s in stories if s['sprint']==sprint]
        table.append(f"Sprint {sprint}: {len(batch)} items, {sum(s['story_points'] for s in batch)} points. Points describe relative complexity, not hours or proven team velocity.")
    (OUT/"Backlog.md").write_text("\n".join(table)+"\n",encoding="utf-8")
    matrix=["# Requirements traceability matrix","","All 29 submitted requirements are mapped below. Test outcomes are in ../evidence; this is local verification, not hosted CI or PR review evidence.","","| Requirement | Component | Verification |","|---|---|---|"]
    for s in stories[:29]:
        group=s['id'].split('-')[1]
        component={"AUTH":"Auth service","PAT":"Patient service","DOC":"Doctor service","APP":"Appointment service","REP":"Report service","PERF":"Business layer and DAL","REL":"SQLite DAL","MAINT":"Source and verification tools","REQ":"Auth and DAL"}[group]
        matrix.append(f"| {s['id']} | {component} | {s['tests']} |")
    (OUT/"Traceability.md").write_text("\n".join(matrix)+"\n",encoding="utf-8")
    config={"owner":"Sumukh-2006","repository":"Sumukh-2006/SE-Mini-Project_Team-H-12","title":"H-12 HMS Part 2","status":"Not created; awaiting explicit publication authorization","fields":{"Status":["Backlog","Ready","In progress","In review","Done"],"Priority":["High","Medium","Low"],"Story Points":"Number","Assignee":"Built-in","Sprint":{"type":"Iteration","iterations":[{"title":"Sprint 1","start":"2026-10-19","duration_days":5},{"title":"Sprint 2","start":"2026-10-26","duration_days":5}]}},"views":["Backlog table","Status board","Sprint 1","Sprint 2"],"assignee_mapping":{"Shashank D":"shashankd06","Sumukh Shandilya":"Sumukh-2006","Supreeth K":None,"Shashank K":None}}
    (OUT/"project-setup.json").write_text(json.dumps(config,indent=2)+"\n")
    print(f"Prepared {len(stories)} stories, 29 requirement mappings and Project setup locally")

if __name__=="__main__":main()
