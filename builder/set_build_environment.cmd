
@echo off

@rem see also default.rel for pathes
set SolutionDir=%CD%
set OutputDir=%SolutionDir%\output
set BinDir=%OutputDir%\bin
set ToolsDir=%BinDir%\tools
set IntDir=%OutputDir%\int
set LibDir=%OutputDir%\lib
set ExternalsDir=%SolutionDir%\externals
set InitialDir=%CD%
set TempDir=%OutputDir%\temp
set BuilderDir=%SolutionDir%\builder

set OpenSslVersion=1.0.0d
set OpenSslDir=%ExternalsDir%\openssl

set IsDebug=false
set IsRelease=false
set CreateProject=yes
set BuildWhat=none

set IsHelpMode=false
set IsNoClean=false
set IsNoBuild=false
set IsFinalBuild=false

set IsFirstKey=true

:GetKey
if "%1"=="" goto GetKeyEnd
if "%IsFinalBuild%"=="true" (
	echo Error: failed to set additional parameters for final build.
	goto Help
)
if "%1"=="release" goto SetFinalBuildType
if "%1"=="final" goto SetFinalBuildType
set IsFirstKey=false
if "%1"=="conf" goto GetBuildType
if "%1"=="build" goto GetWhatToBuild
if "%1"=="full" goto GetFullBuild
if "%1"=="no_clean" goto GetNoClean
if "%1"=="no_build" goto GetNoBuild
if "%1"=="create_project" goto GetCreateProject
if "%1"=="create_projects" goto GetCreateProject
echo Error: "%1" - unknown parameter.
goto Help

:GetBuildType
shift
if "%1"=="release" (
	set IsRelease=true
	goto GetNextKey
)
if "%1"=="full" (
	set IsRelease=true
	set IsDebug=true
	goto GetNextKey
)
echo Error: "%1" - unknown build type.
goto Help

:SetFinalBuildType
shift
if "%IsFirstKey%" NEQ "true" (
	echo Error: failed to set additional parameters for final build.
	goto Help
)
set IsFinalBuild=true
set IsRelease=true
set BuildWhat=all
goto GetNextKey

:GetWhatToBuild
shift
if "%1"=="all" (
	set BuildWhat=all
	goto GetNextKey
)
if "%1"=="ws" (
	set BuildWhat=workspace
	goto GetNextKey
)
if "%1"=="workspace" (
	set BuildWhat=workspace
	goto GetNextKey
)
if "%1"=="ace" (
	set BuildWhat=ACE
	goto GetNextKey
)
if "%1"=="wxwidgets" (
	set BuildWhat=wxWidgets
	goto GetNextKey
)
if "%1"=="libxml" (
	set BuildWhat=Libxml
	goto GetNextKey
)
if "%1"=="gsoap" (
	set BuildWhat=gSOAP
	goto GetNextKey
)
if "%1"=="openssl" (
	set BuildWhat=OpenSSL
	goto GetNextKey
)
if "%1"=="miniupnp" (
	set BuildWhat=MiniUPnP
	goto GetNextKey
)
if "%1"=="tunnelex" (
	set BuildWhat=TunnelEx
	goto GetNextKey
)

echo Error: "%1" - unknown component for build.
goto Help

:GetFullBuild
shift
if "%1" neq "" goto GetFullBuildError
if "%BuildWhat%" neq "none" goto GetFullBuildError
if "%IsRelease%" neq "false" goto GetFullBuildError
if "%IsDebug%" neq "false" goto GetFullBuildError
if "%IsNoClean%" neq "false" goto GetFullBuildError
set BuildWhat=all
set IsRelease=true
set IsDebug=true
goto GetKeyEnd

:GetNoClean
set IsNoClean=true
goto GetNextKey

:GetNoBuild
set IsNoBuild=true
goto GetNextKey

:GetCreateProject
shift
if "%1"=="yes" (
	set CreateProject=%1%
	goto GetNextKey
)
if "%1"=="no" (
	set CreateProject=%1%
	goto GetNextKey
)
if "%1"=="only" (
	set CreateProject=%1%
	goto GetNextKey
)
echo Error: "%1" - unknown mode for create_project.
goto Help

:GetFullBuildError
echo Error: the key 'full' must be only one parameter.
goto Help

:GetNextKey
shift
goto GetKey

:GetKeyEnd
if "%IsDebug%"=="false" if "%IsRelease%"=="false" if "%IsNoBuild%"=="false" (
	echo Please, specify the configuration type.
	goto Help
)
if "%BuildWhat%"=="none" (
	echo Please, specify what component should be build.
	goto Help
)

echo Solution dir is "%SolutionDir%".
goto Finish

:Finish
Verify > nul
echo on
@exit /B 0

:Error
echo on
@exit /B 1

:Help
set IsHelpMode=true
echo Parameters:
echo     final or release - build final release
echo     conf             - configuration type
echo                        values: "release", "debug" or "full"
echo                        ex.: conf=release
echo     build            - specify what component should be build
echo                        values: "all", "ws" (workspace), "openssl", "ace",
echo                        "wxwidgets", "libxml", "gsoap", "miniupnp" or
echo                        "tunnelex"
echo                        ex.: build=all
echo     full             - build all components for all configuraions
echo     create_project   - projects generation mode
echo                        values: "yes" (default), "no" or "only"
echo     no_clean         - do not clean temp-files after build
echo     no_build         - do not build binaries, only urarc, patch files, create projects and so on
echo on
@exit /B 1
