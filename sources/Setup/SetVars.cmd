
Call "%SolutionDir%Version\SetVersion.cmd"

set UpgradeCode=CAF68BDD-A264-4849-B888-15A30775E77D
set ProductName=%TunnelExName%
set ProductFullName=%ProductName%
set CompressionLevel=none
set MsFrameworkDir=%ProgramFiles%\Microsoft Visual Studio 8\VC\redist\x86\Microsoft.VC80.CRT\
set TexFileNameSuffix=
set TexExternalsFileNameSuffix=
set MsFileNameSuffix=
set MsManifestNameSuffix=

if "%ConfigurationName%" neq "Release" (
	set ProductFullName=%ProductFullName% %ConfigurationName%
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
set MsFrameworkDir=%ProgramFiles%\Microsoft Visual Studio 8\VC\redist\Debug_NonRedist\x86\Microsoft.VC80.DebugCRT\
set TunnelExRevisionState=%TunnelExRevisionState%D
goto EndSet

:SetTest
set TexFileNameSuffix=_test
set TunnelExRevisionState=%TunnelExRevisionState%T
goto EndSet

:EndSet

if "%TunnelExRevisionState%" neq "" (
	set ProductFullName=%ProductFullName% %TunnelExRevisionState%
)
