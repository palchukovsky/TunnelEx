
set IntDir=%~s1
set ConfigurationName=%~2
set OutDir=%~s3
set SolutionDir=%~4
set ProjectDir=%~s5

Call SetVars.cmd

if %TIME:~0,1%_==_ goto skipSpace
set Stamp=%TIME:~0,2%.%TIME:~3,2%
goto endSp
:skipSpace
set Stamp=0%TIME:~1,1%.%TIME:~3,2%
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

set BuildIdentity=%ProductName% (%ConfigurationName%) %TunnelExVersionFull%%TunnelExRevisionState% %Stamp% %USERDOMAIN%
set OutFile=%OutDir%%BuildIdentity%.msi

light.exe ^
	-ct 10 ^
	-nologo ^
	-pedantic ^
	-ext WixUtilExtension ^
	-ext WixUIExtension ^
	-cultures:en-us ^
	-b %IntDir%  ^
	-out "%OutFile%" ^
	%IntDir%Product.wxs.wixobj ^
	%IntDir%UI.wxs.wixobj ^
	%IntDir%Actions.wxs.wixobj ^
	%IntDir%Components.wxs.wixobj ^
	-loc %ProjectDir%localization/en-us.wxl
