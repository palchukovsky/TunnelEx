
set Root=

:GetKey
if "%~1" == "" goto GetKeyEnd
if "%~1" == "Root" goto GetRoot
echo Error: "%1" - unknown parameter.
goto Error

:GetNextKey
shift
goto GetKey

:GetRoot
shift
if "%~1" == "" goto Error
set Root=%~1
goto GetNextKey

:GetKeyEnd
if "%Root%" == "" goto Error
Verify > nul

set Compiler=%Root%Version\Compile.js
if exist "%Compiler%" del "%Compiler%"
if %errorlevel% neq 0 goto Error
SubWCRev "%Root%\" "%Compiler%.template" "%Compiler%" -f
if %errorlevel% neq 0 goto Error
goto Success

:Error
@echo Use root-key for project root, like: root="D:\Projects\TunnelEx\" (with slash at the end).
exit 1

:Success