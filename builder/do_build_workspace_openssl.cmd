
@rem OpenSslVersion and OpenSslDir see in the set_build_environment.cmd

set OpenSslNameAndVersion=openssl-%OpenSslVersion%
set OpenSslArchiveName=openssl-%OpenSslVersion%.tar

7z x -y -bd -o"%TempDir%" "%ExternalsDir%\%OpenSslArchiveName%.gz" > nul
@if %errorlevel% neq 0 goto Error

7z x -y -bd -o"%TempDir%" "%TempDir%\%OpenSslArchiveName%" > nul
@if %errorlevel% neq 0 goto Error

cd "%TempDir%\%OpenSslNameAndVersion%"
@if %errorlevel% neq 0 goto Error

patch -p1 -i "%BuilderDir%\OpenSSL.patch"
@if %errorlevel% neq 0 goto Error

perl Configure VC-WIN32 --prefix=__install__
@if %errorlevel% neq 0 goto Error

call ms\do_nasm
@if %errorlevel% neq 0 goto Error

@if "%IsNoBuild%"=="true" goto BuildFinish

nmake -f ms\ntdll.mak install
@if %errorlevel% neq 0 goto Error

cd __install__
@if %errorlevel% neq 0 goto Error

if not exist "%OpenSslDir%" mkdir "%OpenSslDir%"
@if %errorlevel% neq 0 goto Error

if exist "%OpenSslDir%\%OpenSslVersion%" rmdir /S /Q "%OpenSslDir%\%OpenSslVersion%"
@if %errorlevel% neq 0 goto Error
mkdir "%OpenSslDir%\%OpenSslVersion%"
@if %errorlevel% neq 0 goto Error

@if %errorlevel% neq 0 goto Error
xcopy ".\include\*" "%OpenSslDir%\%OpenSslVersion%" /Y /E /R /I
@if %errorlevel% neq 0 goto Error
xcopy ".\bin\*.exe" "%ToolsDir%" /Y /I
@if %errorlevel% neq 0 goto Error
xcopy ".\bin\*.dll" "%BinDir%" /Y /I
@if %errorlevel% neq 0 goto Error
xcopy ".\lib\*.lib" "%LibDir%" /Y /I
@if %errorlevel% neq 0 goto Error 

@goto BuildFinish 

:Error
@exit /B 1
:BuildFinish
@exit /B 0
