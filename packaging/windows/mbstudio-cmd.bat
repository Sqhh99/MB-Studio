@echo off
rem MB-System Command Prompt: a console with the MB-System programs on PATH.
set "MBSTUDIO_BIN=%~dp0"
set "PATH=%MBSTUDIO_BIN%;%PATH%"
chcp 65001 >nul
echo MB-Studio command prompt - the MB-System programs (mbinfo, mbgrid, mbprocess, ...)
echo and gmt are available. Run "mbstudio" to start the graphical user interface.
echo.
