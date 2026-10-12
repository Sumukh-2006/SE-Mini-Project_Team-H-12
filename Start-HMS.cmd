@echo off
title Hospital Management System
wsl.exe -d Ubuntu-24.04 --cd "%~dp0." --exec ./deliverables/bin/hms_app
pause
