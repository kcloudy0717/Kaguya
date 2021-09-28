@echo off
if not exist "Build" mkdir Build
cd Build

cmake -A x64 ../
pause
