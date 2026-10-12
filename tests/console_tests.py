#!/usr/bin/env python3
"""Exercise actual console workflows through a pseudoterminal; passwords are never logged."""
import argparse
import os
from pathlib import Path
import tempfile
import pexpect

def run(binary):
    with tempfile.TemporaryDirectory(prefix="hms-console-") as directory:
        database = str(Path(directory) / "demo.db")
        password = "ConsoleTest93"
        env = dict(os.environ, HMS_DEMO_PASSWORD=password)
        seed = pexpect.spawn(binary, ["--db", database, "--demo-seed"], env=env, encoding="utf-8", timeout=10, echo=False)
        seed.expect("Demo database initialized")
        seed.expect(pexpect.EOF)
        seed.close()
        assert seed.exitstatus == 0
        app = pexpect.spawn(binary, ["--db", database], encoding="utf-8", timeout=10, echo=False)
        def send(prompt, value):
            app.expect_exact(prompt)
            app.sendline(str(value))
        send("Username (q to quit): ", "reception")
        send("Password: ", password)
        app.expect_exact("Receptionist menu")
        send("Choice: ", 1)
        send("Patient name: ", "Taylor Console")
        send("Age: ", "invalid")
        app.expect_exact("Error: Enter a whole number")
        send("Choice: ", 1)
        send("Patient name: ", "Taylor Console")
        send("Age: ", 31)
        send("Gender (Male/Female/Other): ", "Female")
        send("Contact: ", "9000000004")
        send("Blood group: ", "A+")
        app.expect_exact("Patient registered. Patient ID: P000002")
        send("Choice: ", 2)
        send("Patient ID or name: ", "P000002")
        app.expect_exact("P000002 | Taylor Console")
        send("Choice: ", 4)
        send("Patient ID: ", "P000002")
        send("Doctor ID: ", 1)
        send("Date (YYYY-MM-DD): ", "2026-10-19")
        send("Time (HH:MM): ", "09:00")
        app.expect_exact("Appointment booked. Appointment ID: 1")
        send("Choice: ", 0)
        app.expect_exact("Logged out")
        send("Username (q to quit): ", "q")
        app.expect(pexpect.EOF)
        app.close()
        assert app.exitstatus == 0
        locked = pexpect.spawn(binary, ["--db", database], encoding="utf-8", timeout=10, echo=False)
        for _ in range(3):
            locked.expect_exact("Username (q to quit): ")
            locked.sendline("admin")
            locked.expect_exact("Password: ")
            locked.sendline("wrong")
        locked.expect_exact("Three attempts used; access denied.")
        locked.expect(pexpect.EOF)
        locked.close()
        assert locked.exitstatus == 2
        print("PASS: actual console menus, invalid numeric input, registration, search, booking, logout, three-attempt denial")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("binary")
    run(str(Path(parser.parse_args().binary).resolve()))
