
set BoostNameAndVersion=boost_1_40_0
set BoostDir=%ExternalsDir%\boost
set BoostSourcesDir=%TempDir%\%BoostNameAndVersion%
set BoostConfigurationType=

if "%IsDebug%"=="true" goto ConfDebug
:ConfEnd
if "%IsRelease%"=="true" set BoostConfigurationType=variant=release%BoostConfigurationType%

7z x -y -bd -o"%TempDir%" "%ExternalsDir%\%BoostNameAndVersion%.7z" > nul
@if %errorlevel% neq 0 goto Error

cd "%BoostSourcesDir%\tools\jam\src"
@if %errorlevel% neq 0 goto Error

call build.bat
@if %errorlevel% neq 0 goto Error
move /Y bin.ntx86\bjam.exe "%BoostSourcesDir%"
@if %errorlevel% neq 0 goto Error

cd "%BoostSourcesDir%"
@if %errorlevel% neq 0 goto Error

bjam ^
	-q ^
	-d0 ^
	--libdir=%LibDir% ^
	--includedir=%BoostDir% ^
	--with-regex ^
	--with-thread ^
	--with-test ^
	--with-date_time ^
	--with-filesystem ^
	link=static ^
	threading=multi ^
	runtime-link=shared ^
	link=static ^
	%BoostConfigurationType% ^
	install
@if %errorlevel% neq 0 goto Error

@goto Finish

:Error
@exit /B 1
:Finish
@exit /B 0

:ConfDebug
set BoostConfigurationType=variant=debug
if "%IsRelease%"=="true" set BoostConfigurationType= %BoostConfigurationType%
goto ConfEnd
