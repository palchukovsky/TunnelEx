
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
devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Debug" /project Test
devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Debug" /project Setup
devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Test" /project Test
devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Test" /project Setup
@if %errorlevel% neq 0 goto Error
@goto BuildNext

:BuildRelease
if "%IsFinalBuild%"=="true" (
	if "%IsDebug%" NEQ "true" (
		devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Test" /project Test
	)
	cd "%BinDir%"
	TexTest_test.exe
	@if %errorlevel% neq 0 goto Error
)
devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Release" /project Test
if "%IsFinalBuild%"=="true" (
	cd "%BinDir%"
	TexTest.exe
	@if %errorlevel% neq 0 goto Error
)
devenv.com "%SolutionDir%\sources\TunnelEx.sln" /Build "Release" /project Setup
@if %errorlevel% neq 0 goto Error
@goto Finish
