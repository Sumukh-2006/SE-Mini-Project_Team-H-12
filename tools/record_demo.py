#!/usr/bin/env python3
"""Capture a real PTY session, then replay its output as a captioned terminal video."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import secrets
import subprocess
import tempfile
import textwrap
import pexpect
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]

def record(binary, sprint, checkpoint):
    duration = 60 if sprint == 1 else 120
    output = ROOT / "deliverables" / "videos"
    output.mkdir(parents=True, exist_ok=True)
    scenes, transcript = [], ""
    password = secrets.token_urlsafe(16)
    with tempfile.TemporaryDirectory(prefix="hms-demo-") as directory:
        database = str(Path(directory) / "fictional.db")
        subprocess.run([str(binary), "--db", database, "--demo-seed"], env=dict(os.environ, HMS_DEMO_PASSWORD=password), check=True, capture_output=True)
        app = pexpect.spawn(str(binary), ["--db", database], echo=False, encoding="utf-8", timeout=15)
        def expect(marker):
            nonlocal transcript
            app.expect_exact(marker)
            transcript += app.before.replace("\r", "") + app.after
        def send(prompt, value, secret=False):
            nonlocal transcript
            expect(prompt)
            app.sendline(str(value))
            transcript += "[hidden]" if secret else str(value)
            transcript += "\n"
        def scene(caption, seconds):
            scenes.append({"caption": caption, "seconds": seconds, "terminal": transcript})
        def login(user):
            send("Username (q to quit): ", user)
            send("Password: ", password, True)
            expect({"admin":"Admin", "reception":"Receptionist", "doctor":"Doctor"}[user] + " menu")
        def book(time):
            send("Choice: ", 4); send("Patient ID: ", "P000001"); send("Doctor ID: ", 1)
            send("Date (YYYY-MM-DD): ", "2026-10-19"); send("Time (HH:MM): ", time)
        login("reception")
        if sprint == 1:
            scene("Receptionist login succeeds and opens the role-specific menu", 8)
            send("Choice: ", 1); send("Patient name: ", "Taylor Demo"); send("Age: ", 31)
            send("Gender (Male/Female/Other): ", "Female"); send("Contact: ", "9000000004"); send("Blood group: ", "A+")
            expect("Patient registered. Patient ID: P000002")
            scene("Register a fictional patient and receive a unique patient ID", 12)
            send("Choice: ", 2); send("Patient ID or name: ", "P000002"); expect("P000002 | Taylor Demo | age 31 | Female | 9000000004 | A+")
            scene("Search retrieves the persisted patient details", 8)
            send("Choice: ", 3); send("Specialization (blank for all): ", "Cardiologist"); expect("1 | Dr Mira Rao | Cardiologist")
            scene("Filter the doctor directory by specialization", 8)
            book("09:00"); expect("Appointment booked. Appointment ID: 1 | 2026-10-19 09:00 | Scheduled")
            scene("Book an available 15-minute appointment", 10)
            book("09:00"); expect("Error: Doctor unavailable or slot already booked")
            scene("The same doctor slot cannot be booked twice", 9)
            send("Choice: ", 0); expect("Logged out")
            scene("Explicit logout destroys the session", 5)
        else:
            scene("Sprint 2 extends the working core console application", 7)
            send("Choice: ", 5); send("Patient ID: ", "P000001"); send("New contact: ", "9000000005"); expect("Patient contact updated")
            scene("Update patient contact details as a receptionist", 8)
            book("09:00"); expect("Appointment booked. Appointment ID: 1 | 2026-10-19 09:00 | Scheduled")
            book("09:15"); expect("Appointment booked. Appointment ID: 2 | 2026-10-19 09:15 | Scheduled")
            scene("Create two appointments to exercise scheduling rules", 10)
            send("Choice: ", 7); send("Appointment ID: ", 1); send("New date (YYYY-MM-DD): ", "2026-10-19"); send("New time (HH:MM): ", "09:15"); expect("Error: Doctor unavailable or slot already booked")
            scene("A conflicting reschedule fails and preserves the original booking", 10)
            send("Choice: ", 7); send("Appointment ID: ", 1); send("New date (YYYY-MM-DD): ", "2026-10-19"); send("New time (HH:MM): ", "09:30"); expect("Appointment rescheduled")
            scene("Move the booking to an available slot atomically", 10)
            send("Choice: ", 6); send("Appointment ID: ", 2); expect("Appointment canceled")
            scene("Cancel the second appointment without deleting its history", 9)
            send("Choice: ", 0); expect("Logged out"); login("doctor")
            send("Choice: ", 2); send("Date (YYYY-MM-DD): ", "2026-10-19"); expect("1 | P000001 | Doctor 1 | 2026-10-19 09:30 | Scheduled")
            scene("The doctor sees only their own daily appointments", 12)
            send("Choice: ", 3); send("Appointment ID: ", 1); expect("Appointment completed")
            scene("The doctor completes their consultation", 9)
            send("Choice: ", 0); expect("Logged out"); login("admin")
            send("Choice: ", 5); send("Date (YYYY-MM-DD): ", "2026-10-19"); expect("Total booked: 2 | Scheduled: 0 | Canceled: 1 | Completed: 1")
            scene("The administrator receives accurate daily appointment totals", 15)
            report = str(Path(directory) / "patients.csv")
            send("Choice: ", 6); send("Output file (.csv or .txt): ", report); expect("Patient report exported")
            assert "Alex Demo" in Path(report).read_text()
            scene("Export the patient report to a real CSV file on local disk", 15)
            send("Choice: ", 0); expect("Logged out")
            scene("Working final product with persistent storage and explicit logout", 15)
        send("Username (q to quit): ", "q")
        app.expect(pexpect.EOF); app.close()
        assert app.exitstatus == 0
        assert sum(s["seconds"] for s in scenes) == duration
        font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 20)
        heading = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 29)
        caption_font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 24)
        frame_dir = ROOT / "tmp" / "video-qa" / f"sprint{sprint}"; frame_dir.mkdir(parents=True,exist_ok=True)
        lines = []
        for i, s in enumerate(scenes):
            img = Image.new("RGB", (1280,720), "#0e1726"); draw = ImageDraw.Draw(img)
            draw.text((35,25), f"Hospital Management System  /  Sprint {sprint}", fill="#ffffff", font=heading)
            draw.text((35,68), "Actual application session | fictional demo data | prepared locally", fill="#9eb1ca", font=font)
            draw.rounded_rectangle((25,115,1255,598), radius=10, fill="#162337")
            terminal_lines = []
            for line in s["terminal"].splitlines():
                terminal_lines.extend(textwrap.wrap(line, 96, replace_whitespace=False, drop_whitespace=False) or [""])
            for row, line in enumerate(terminal_lines[-19:]):
                color = "#ffb0a0" if line.startswith("Error:") else "#d7efe5"
                draw.text((44,130 + row * 24), line, fill=color, font=font)
            for row, line in enumerate(textwrap.wrap(s["caption"], 88)):
                draw.text((35,620 + row * 30), line, fill="#ffffff", font=caption_font)
            frame = frame_dir / f"scene-{i:02}.png"; img.save(frame)
            lines.extend([f"file '{frame}'", f"duration {s['seconds']}"])
        lines.append(f"file '{frame}'")
        concat = frame_dir / "frames.txt"; concat.write_text("\n".join(lines))
        mp4 = output / f"Sprint-{sprint}-Demo.mp4"
        subprocess.run(["ffmpeg", "-hide_banner", "-loglevel", "error", "-y", "-f", "concat", "-safe", "0", "-i", str(concat), "-vf", "fps=10,tpad=stop_mode=clone:stop_duration=1", "-t", str(duration), "-c:v", "libx264", "-preset", "fast", "-crf", "20", "-pix_fmt", "yuv420p", "-movflags", "+faststart", str(mp4)], check=True)
        (output / f"Sprint-{sprint}-Transcript.txt").write_text(transcript)
        manifest = {"sprint": sprint, "duration_seconds": duration, "checkpoint": checkpoint, "binary_sha256":hashlib.sha256(binary.read_bytes()).hexdigest(), "capture":"Real executable PTY output replayed with captions; scene hold durations edited for readability", "scenes":[{"caption":s["caption"],"seconds":s["seconds"]} for s in scenes]}
        (output / f"Sprint-{sprint}-Recording.json").write_text(json.dumps(manifest, indent=2)+"\n")
        print(f"Recorded {mp4.name}: {duration}s from real console interactions")

if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("binary",type=Path); parser.add_argument("--sprint",type=int,choices=[1,2],required=True);parser.add_argument("--checkpoint",required=True)
    args=parser.parse_args();record(args.binary.resolve(), args.sprint, args.checkpoint)
