
set WxWidgetsVersion=2.8.10
set WxWidgetsNameAndVersion=wxWidgets-%WxWidgetsVersion%
set WxWidgetsDir=%ExternalsDir%\wxWidgets
set WxWidgetsSourcesDir=%WxWidgetsDir%\%WxWidgetsNameAndVersion%
set WxWidgetsBuildDir=%WxWidgetsSourcesDir%\build\msw

7z x -y -bd -o"%TempDir%" "%ExternalsDir%\%WxWidgetsNameAndVersion%.tar.bz2" > nul
@if %errorlevel% neq 0 goto Error

7z x -y -bd -o"%WxWidgetsDir%" "%TempDir%\%WxWidgetsNameAndVersion%.tar" "%WxWidgetsNameAndVersion%\art" "%WxWidgetsNameAndVersion%\include" "%WxWidgetsNameAndVersion%\src" > nul
@if %errorlevel% neq 0 goto Error

if not exist "%WxWidgetsBuildDir%" mkdir "%WxWidgetsBuildDir%"
xcopy  /Y /R "%WxWidgetsDir%\makefile.%WxWidgetsVersion%.vc" "%WxWidgetsBuildDir%"
@if %errorlevel% neq 0 goto Error
if exist "%WxWidgetsBuildDir%\makefile.vc" (
	del /Q /F "%WxWidgetsBuildDir%\makefile.vc"
)
@if %errorlevel% neq 0 goto Error
rename "%WxWidgetsBuildDir%\makefile.%WxWidgetsVersion%.vc" "makefile.vc"
@if %errorlevel% neq 0 goto Error

attrib -R "%WxWidgetsSourcesDir%\*" /S /D
@if %errorlevel% neq 0 goto Error

cd "%WxWidgetsBuildDir%"
@if %errorlevel% neq 0 goto Error

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
xcopy  /Y /R "%WxWidgetsDir%\config.%WxWidgetsVersion%.debug.vc" "%WxWidgetsBuildDir%"
@if %errorlevel% neq 0 goto Error
if exist "%WxWidgetsBuildDir%\config.vc" (
	del /Q /F "%WxWidgetsBuildDir%\config.vc"
)
@if %errorlevel% neq 0 goto Error
rename "%WxWidgetsBuildDir%\config.%WxWidgetsVersion%.debug.vc" "config.vc"
@if %errorlevel% neq 0 goto Error
nmake makefile.vc
@if %errorlevel% neq 0 goto Error
@goto BuildNext

:BuildRelease
xcopy  /Y /R "%WxWidgetsDir%\config.%WxWidgetsVersion%.release.vc" "%WxWidgetsBuildDir%"
@if %errorlevel% neq 0 goto Error
if exist "%WxWidgetsBuildDir%\config.vc" (
	del /Q /F "%WxWidgetsBuildDir%\config.vc"
)
@if %errorlevel% neq 0 goto Error
rename "%WxWidgetsBuildDir%\config.%WxWidgetsVersion%.release.vc" "config.vc"
@if %errorlevel% neq 0 goto Error
nmake makefile.vc
@if %errorlevel% neq 0 goto Error
@goto Finish
