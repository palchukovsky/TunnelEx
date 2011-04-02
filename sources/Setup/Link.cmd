
set IntDir=%~s1
set ConfigurationName=%~2
set OutDir=%~s3
set SolutionDir=%~4
set ProjectDir=%~s5

Call SetVars.cmd

set OutFile=%OutDir%%ProductName% [%TexBuildIdentity%].msi

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
