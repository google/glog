@echo off
setlocal EnableDelayedExpansion

cd /d %~dp0
set PROGFILES=%ProgramFiles%
if not "%ProgramFiles(x86)%" == "" set PROGFILES=%ProgramFiles(x86)%

REM Check if Visual Studio 2008 is installed
set MSVCDIR="%PROGFILES%\Microsoft Visual Studio 9.0"
if exist %MSVCDIR% (
    set COMPILER_VER="2008"
	goto setup_env
)

echo No compiler : Microsoft Visual Studio (2008) is not installed.
goto end

:setup_env

call %MSVCDIR%\VC\vcvarsall.bat x86

:begin

REM Variables
set SOLUTION_FILE="google-glog.sln"

if %COMPILER_VER% == "2008" (
	set VCVERSION = 9
	goto buildnow
)

:buildnow
REM Build glog!
msbuild %SOLUTION_FILE% /p:Configuration="Release";Platform="Win32"

:end
exit /b
