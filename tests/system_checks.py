#!/usr/bin/env python3
"""Verify real process RSS, SQLite crash recovery, and console workflows under Valgrind."""
import json
import os
from pathlib import Path
import secrets
import sqlite3
import subprocess
import sys
import tempfile
import time
import pexpect

ROOT=Path(__file__).resolve().parents[1]
binary=str(Path(sys.argv[1]).resolve())
output=ROOT/"deliverables/evidence";output.mkdir(parents=True,exist_ok=True)
with tempfile.TemporaryDirectory(prefix="hms-system-") as directory:
    database=str(Path(directory)/"demo.db")
    password=secrets.token_urlsafe(16)
    subprocess.run([binary,"--db",database,"--demo-seed"],env=dict(os.environ,HMS_DEMO_PASSWORD=password),check=True,capture_output=True)
    db=sqlite3.connect(database)
    db.executemany("INSERT INTO patients(public_id,name,age,gender,contact,blood_group) VALUES(?,?,29,'Other','9000000002','O+')",[(f"PERF{i:05}",f"Performance Patient {i}") for i in range(9999)])
    db.commit();db.close()
    app=pexpect.spawn(binary,["--db",database],echo=False,encoding="utf-8",timeout=15)
    app.expect_exact("Username (q to quit): ");app.sendline("reception")
    app.expect_exact("Password: ");app.sendline(password)
    app.expect_exact("Choice: ");app.sendline("2")
    app.expect_exact("Patient ID or name: ")
    start=time.perf_counter();app.sendline("PERF09998");app.expect_exact("PERF09998 | Performance Patient 9998")
    elapsed=time.perf_counter()-start
    app.expect_exact("Choice: ")
    app.sendline("2");app.expect_exact("Patient ID or name: ")
    start=time.perf_counter();app.sendline("Performance Patient");app.expect_exact("Choice: ")
    broad_elapsed=time.perf_counter()-start
    status=Path(f"/proc/{app.pid}/status").read_text()
    rss_kb=int(next(line.split()[1] for line in status.splitlines() if line.startswith("VmRSS:")))
    assert rss_kb<50*1024,f"Memory footprint {rss_kb}KB exceeds 50MB"
    assert elapsed<2,f"Query took {elapsed}s"
    assert broad_elapsed<2,f"Broad query and display took {broad_elapsed}s"
    app.sendline("0");app.expect_exact("Username (q to quit): ");app.sendline("q");app.expect(pexpect.EOF);app.close();assert app.exitstatus==0
    # A child using the production DAL is killed with a live uncommitted write.
    probe=str(Path(binary).parent/"hms_crash_probe")
    child=subprocess.Popen([probe,database],stdout=subprocess.PIPE,text=True)
    assert child.stdout.readline().strip()=="UNCOMMITTED", "Crash probe did not enter its transaction"
    child.kill();child.wait()
    reopened=sqlite3.connect(database)
    assert reopened.execute("PRAGMA integrity_check").fetchone()[0]=="ok"
    assert reopened.execute("SELECT contact FROM patients WHERE public_id='P000001'").fetchone()[0]=="9000000002"
    reopened.close()
    # Measure the application itself, not only the test runner, under Valgrind.
    workflow="reception\n"+password+"\n2\nP000001\n4\nP000001\n1\n2026-10-19\n09:00\n0\nq\n"
    mem=subprocess.run(["valgrind","--leak-check=full","--show-leak-kinds=definite","--errors-for-leak-kinds=definite","--error-exitcode=1",binary,"--db",database],input=workflow,capture_output=True,text=True)
    (output/"console-valgrind.txt").write_text(mem.stderr)
    assert mem.returncode==0 and "ERROR SUMMARY: 0 errors" in mem.stderr
    assert "Appointment booked" in mem.stdout
    metrics={"query_record_count":10000,"query_wall_seconds":elapsed,"broad_query_and_display_seconds":broad_elapsed,"normal_process_rss_kb":rss_kb,"performance_limit_seconds":2,"rss_limit_kb":51200,"crash_recovery":"PASS: production DAL transaction rolled back after SIGKILL; database integrity ok","console_valgrind":"PASS: no errors and no definitely lost memory","environment":"Ubuntu 24.04 under WSL2, GCC 13.3, release build","limitations":"Single workstation measurement; not a multi-platform performance guarantee"}
    (output/"system-metrics.json").write_text(json.dumps(metrics,indent=2)+"\n")
    print(json.dumps(metrics,indent=2))
