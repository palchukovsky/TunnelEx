
@echo off


set PROJECT_CONFIGURATION_SUFFIX=""

set SolutionDir=%CD%
set OutputDir=%SolutionDir%\output
set BinDir=%OutputDir%\bin
set TestsOutDir=%OutputDir%\tests

set IncludeTag=--include=auto


:GetKey
if "%1"=="" goto GetKeyEnd
if "%1"=="auto" goto TestAuto
if "%1"=="all" goto TestAll
if "%1"=="conf" goto GetConfigurationType
if "%1"=="help" goto ShowHelp
echo Error: "%1" - unknown parameter.
goto ShowHelp


:GetNextKey
shift
goto GetKey


:TestAuto
set IncludeTag=--include=auto
goto GetNextKey


:TestAll
set IncludeTag=
goto GetNextKey


:GetConfigurationType
shift
if "%1"=="release" (
	set PROJECT_CONFIGURATION_SUFFIX=""
	echo Project configuration: Release
	goto GetNextKey
)
if "%1"=="debug" (
	set PROJECT_CONFIGURATION_SUFFIX=_dbg
	echo Project configuration: Debug
	goto GetNextKey
)
if "%1"=="test" (
	set PROJECT_CONFIGURATION_SUFFIX=_test
	echo Project configuration: Test
	goto GetNextKey
)
echo Error: "%1" - unknown configuration type.
goto ShowHelp


:ShowHelp
echo Parameters:
echo     auto - start auto-testing (default)
echo     conf - configuration type
echo            values: "release" (default), "test" or "debug"
echo on
@exit /B 1


:GetKeyEnd
if "%PROJECT_CONFIGURATION_SUFFIX%"=="""" (
	echo Project file suffix: no suffix
) else (
	echo Project file suffix: "%PROJECT_CONFIGURATION_SUFFIX%"
)

if %TIME:~0,1%_==_ goto SkipSpaceInTime
set Stamp=%TIME:~0,2%.%TIME:~3,2%.%TIME:~6,2%
goto EndSpaceInTime
:SkipSpaceInTime
set Stamp=0%TIME:~1,1%.%TIME:~3,2%.%TIME:~6,2%
:EndSpaceInTime
if %DATE:~3,1%_==_ goto SpaceInTime3
if %DATE:~2,1%_==_ goto SpaceInTime2
set Stamp=%DATE:~6,4%.%DATE:~3,2%.%DATE:~0,2%_%Stamp%
goto EndSpaceInTime
:SpaceInTime3
set Stamp=%DATE:~10,4%.%DATE:~7,2%.%DATE:~4,2%_%Stamp%
goto EndSpaceInTime
:SpaceInTime2
set Stamp=%DATE:~9,4%.%DATE:~6,2%.%DATE:~3,2%_%Stamp%
:EndSpaceInTime
if "%PROJECT_CONFIGURATION_SUFFIX%"=="""" (
	set ResultDir=%Stamp%
) else (
	set ResultDir=%Stamp%%PROJECT_CONFIGURATION_SUFFIX%
)


call pybot.bat %IncludeTag% --outputdir=%TestsOutDir%\%ResultDir% tests/TunnelEx.tsv
