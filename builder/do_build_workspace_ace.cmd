
set AceVersion=6.0.0
set AceNameAndVersion=ACE-src-%AceVersion%
set ACE_ROOT=%ExternalsDir%\ACE_wrappers
set MPC_ROOT=%ACE_ROOT%\MPC
set CIAO_ROOT=%ACE_ROOT%
set DANCE_ROOT=%ACE_ROOT%
set DDS_ROOT=%ACE_ROOT%
set TAO_ROOT=%ACE_ROOT%

if "%CreateProject%"=="only" goto CreateAceProject

7z x -y -bd -o"%TempDir%" "%ExternalsDir%\%AceNameAndVersion%.tar.bz2" > nul
@if %errorlevel% neq 0 goto Error

7z x -y -bd -o"%ExternalsDir%" "%TempDir%\%AceNameAndVersion%.tar" "ACE_wrappers\ace" "ACE_wrappers\bin" "ACE_wrappers\MPC" "ACE_wrappers\VERSION" > nul
@if %errorlevel% neq 0 goto Error

cd "%ACE_ROOT%"
@if %errorlevel% neq 0 goto Error
if exist "%ACE_ROOT%\ace\config.h" del /S /Q /F "%ACE_ROOT%\ace\config.h"
@if %errorlevel% neq 0 goto Error
@rem diff -crBN ACE_wrappers ACE_wrappers.TunnelEx > ACE_wrappers.patch
patch -p1 -i "%BuilderDir%\ACE_wrappers.patch"
@if %errorlevel% neq 0 goto Error

if "%CreateProject%"=="no" goto Finish
	
:CreateAceProject

cd "%ACE_ROOT%\ace"
@if %errorlevel% neq 0 goto Error
%ACE_ROOT%\bin\mwc.pl -type vc10 -expand_vars -use_env -value_template WarnAsError=no -value_template platforms=Win32 ace.mwc
@if %errorlevel% neq 0 goto Error
if "%CreateProject%"=="only" goto Finish

@if "%IsNoBuild%"=="true" goto BuildFinish
@if "%IsDebug%"=="true" goto BuildDebug
:BuildNext
@if "%IsRelease%"=="true" goto BuildRelease
:BuildFinish
@goto Finish

:Error
@exit /B 1
:Finish
@exit /B 0

:BuildDebug
devenv.com "%ACE_ROOT%\ace\ace.sln" /Build "Debug"
devenv.com "%ACE_ROOT%\ace\ace.sln" /Build "Test"
@if %errorlevel% neq 0 goto Error
@goto BuildNext

:BuildRelease
if "%IsFinalBuild%"=="true" (
	if "%IsDebug%" NEQ "true" (
		devenv.com "%ACE_ROOT%\ace\ace.sln" /Build "Test"
	)
)
devenv.com "%ACE_ROOT%\ace\ace.sln" /Build "Release"
@if %errorlevel% neq 0 goto Error
@goto Finish
