
set ProjectDir=%~1

Call SetApiDir.cmd

set attempts=0

:BuildApi

set attempts+=1

Call ClearApi.cmd

if not exist "%TexCoreApiOutDir%" mkdir "%TexCoreApiOutDir%"

copy /Y Api.h "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Connection.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Singleton.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Collection.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Server.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Rule.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Endpoint.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y EndpointAddress.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Service.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Exceptions.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y String.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y UniquePtr.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y SharedPtr.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Log.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Locking.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Filter.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Listener.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Acceptor.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Instance.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y IoHandle.h "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y ConnectionSignal.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y MessageBlock.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y DataTransferCommand.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Error.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y Time.h "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

copy /Y SslCertificatesStorage.hpp "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

@if "%TexCoreApiOutDir%"=="\" (
	@echo TexCoreApiOutDir is root!
	@exit /B 1
)
attrib +R "%TexCoreApiOutDir%*" /D /S
@if errorlevel 1 goto BuildApiError

goto BuildApiExit

:BuildApiError
if attempts LSS 3 goto BuildApi
exit /B 1

:BuildApiExit
