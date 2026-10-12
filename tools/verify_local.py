#!/usr/bin/env python3
"""Run local verification and retain real results. This script never contacts GitHub."""
import argparse
import datetime
import json
import platform
from pathlib import Path
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/"deliverables/evidence"

def main():
    parser=argparse.ArgumentParser();parser.add_argument("--build",default="build");args=parser.parse_args()
    build=ROOT/args.build;OUT.mkdir(parents=True,exist_ok=True)
    checks=[
        ("unit-tests",[str(build/"hms_tests"),"--gtest_output=xml:"+str(OUT/"final-tests.xml")]),
        ("console",[sys.executable,str(ROOT/"tests/console_tests.py"),str(build/"hms_app")]),
        ("static-analysis",["cppcheck","--enable=warning,performance,portability","--error-exitcode=1","--std=c++17","--inline-suppr","-I",str(ROOT/"include"),str(ROOT/"src")]),
        ("valgrind",["valgrind","--leak-check=full","--show-leak-kinds=definite","--errors-for-leak-kinds=definite","--error-exitcode=1",str(build/"hms_tests")]),
        ("system",[sys.executable,str(ROOT/"tests/system_checks.py"),str(build/"hms_app")])]
    results=[]
    for name,command in checks:
        result=subprocess.run(command,capture_output=True,text=True,cwd=ROOT)
        (OUT/(name+".txt")).write_text(result.stdout+"\n"+result.stderr)
        results.append({"check":name,"exit_code":result.returncode,"result":"PASS" if result.returncode==0 else "FAIL","command":command})
        print(name+": "+results[-1]["result"],flush=True)
    test_xml=ET.parse(OUT/"final-tests.xml").getroot()
    summary={"executed_at_utc":datetime.datetime.now(datetime.timezone.utc).isoformat(),"environment":platform.platform(),"tests":int(test_xml.attrib['tests']),"failures":int(test_xml.attrib['failures']),"errors":int(test_xml.attrib.get('errors',0)),"checks":results,"remote_evidence":"Pending: no hosted CI, Project, issues, PRs, reviews, merges or uploads performed"}
    (OUT/"verification-summary.json").write_text(json.dumps(summary,indent=2)+"\n")
    if any(r['exit_code'] for r in results):raise SystemExit(1)
    print(f"All checks passed; {summary['tests']} tests",flush=True)

if __name__=="__main__":main()
