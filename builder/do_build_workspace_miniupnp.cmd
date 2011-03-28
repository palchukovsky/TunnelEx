
set MiniUpnpVersion=1.4
set MiniUpnpBuild=20100418
set MiniUpnpBinArchiveName=upnpc-exe-win32-%MiniUpnpBuild%
set MiniUpnpSourceArchiveName=miniupnpc-%MiniUpnpVersion%.%MiniUpnpBuild%
set MiniUpnpDir=%ExternalsDir%\miniupnp

7z x -y -bd -o"%MiniUpnpDir%" "%ExternalsDir%\%MiniUpnpBinArchiveName%.zip" > nul
@if %errorlevel% neq 0 goto Error

move /Y "%MiniUpnpDir%\*.exe" "%ToolsDir%"
@if %errorlevel% neq 0 goto Error

move /Y "%MiniUpnpDir%\*.lib" "%LibDir%"
@if %errorlevel% neq 0 goto Error

move /Y "%MiniUpnpDir%\*.def" "%LibDir%"
@if %errorlevel% neq 0 goto Error

move /Y "%MiniUpnpDir%\*.dll" "%BinDir%"
@if %errorlevel% neq 0 goto Error

7z x -y -bd -o"%TempDir%" "%ExternalsDir%\%MiniUpnpSourceArchiveName%.tar.gz" > nul
@if %errorlevel% neq 0 goto Error

7z e -y -bd -o"%MiniUpnpDir%" "%TempDir%\%MiniUpnpSourceArchiveName%.tar" "%MiniUpnpSourceArchiveName%\*.h" > nul
@if %errorlevel% neq 0 goto Error

@goto Finish 

:Error
@exit /B 1
:Finish
@exit /B 0
