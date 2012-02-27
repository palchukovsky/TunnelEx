
Call "%SolutionDir%Version\SetVersion.cmd"

set UpgradeCode=CAF68BDD-A264-4849-B888-15A30775E77D
set ProductName=%TunnelExName%
set ProductFullName=%ProductName%
set CompressionLevel=none
set MsFrameworkDir=%ProgramFiles%\Microsoft Visual Studio 10.0\VC\redist\x86\Microsoft.VC100.CRT\
set TexFileNameSuffix=
set TexExternalsFileNameSuffix=
set MsFileNameSuffix=
set MsManifestNameSuffix=

if %TIME:~0,1%_==_ goto skipSpace
set Stamp=%TIME:~0,2%.%TIME:~3,2%.%TIME:~6,2%
goto endSp
:skipSpace
set Stamp=0%TIME:~1,1%.%TIME:~3,2%.%TIME:~6,2%
:endSp

if %DATE:~3,1%_==_ goto Space3
if %DATE:~2,1%_==_ goto Space2
set Stamp=%DATE:~6,4%.%DATE:~3,2%.%DATE:~0,2%.%Stamp%
goto endSp
:Space3
set Stamp=%DATE:~10,4%.%DATE:~7,2%.%DATE:~4,2%.%Stamp%
goto endSp
:Space2
set Stamp=%DATE:~9,4%.%DATE:~6,2%.%DATE:~3,2%.%Stamp%
:endSp

set TexBuildIdentity=%ConfigurationName% %TunnelExVersionFull% %USERDOMAIN% %Stamp%

if "%ConfigurationName%" neq "Release" (
	set ProductFullName=%ProductFullName% [%TexBuildIdentity%]
)

if "%ConfigurationName%" neq "Debug" (
	set CompressionLevel=high
)

if "%ConfigurationName%" == "Debug" goto SetDebug
if "%ConfigurationName%" == "Test" goto SetTest
goto EndSet

:SetDebug
set TexFileNameSuffix=_dbg
set TexExternalsFileNameSuffix=%TexFileNameSuffix%
set MsFileNameSuffix=d
set MsManifestNameSuffix=Debug
set MsFrameworkDir=%ProgramFiles%\Microsoft Visual Studio 10.0\VC\redist\Debug_NonRedist\x86\Microsoft.VC100.DebugCRT\
goto EndSet

:SetTest
set TexFileNameSuffix=_test
goto EndSet

:EndSet
