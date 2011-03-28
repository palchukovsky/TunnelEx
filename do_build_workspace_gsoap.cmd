
set GsoapDir=%ExternalsDir%\gSOAP
cd "%GsoapDir%"
rar x -o+ gsoap.rar > nul
@if %errorlevel% neq 0 goto Error

if not exist "%OutputDir%\bin\tools" mkdir "%OutputDir%\bin\tools"
@if errorlevel neq 0 goto Error

attrib -R "%GsoapDir%\gsoap-2.7.10\*" /D /S
@if %errorlevel% neq 0 goto Error

move /Y "%GsoapDir%\gsoap-2.7.10\gsoap\bin\win32\*.exe" "%OutputDir%\bin\tools"
@if %errorlevel% neq 0 goto Error

rmdir /Q /S "%GsoapDir%\gsoap-2.7.10\gsoap\bin\win32"
@Verify > nul

xcopy /E /Y /Q /R "%GsoapDir%\gsoap-2.7.10\gsoap\*" "%GsoapDir%"
@if %errorlevel% neq 0 goto Error

rmdir /Q /S "%GsoapDir%\gsoap-2.7.10"
rmdir /Q /S "%GsoapDir%\bin"
@Verify > nul
