
set ProjectDir=%~1

Call SetApiDir.cmd

set attempts=0

:BuildApi

set attempts+=1

if not exist "%TexCoreApiOutDir%" mkdir "%TexCoreApiOutDir%"

echo N | comp "Api.h" "%TexCoreApiOutDir%Api.h" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Api.h" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Connection.hpp" "%TexCoreApiOutDir%Connection.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Connection.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Singleton.hpp" "%TexCoreApiOutDir%Singleton.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Singleton.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Collection.hpp" "%TexCoreApiOutDir%Collection.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Collection.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Server.hpp" "%TexCoreApiOutDir%Server.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Server.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Rule.hpp" "%TexCoreApiOutDir%Rule.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Rule.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Endpoint.hpp" "%TexCoreApiOutDir%Endpoint.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Endpoint.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "EndpointAddress.hpp" "%TexCoreApiOutDir%EndpointAddress.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "EndpointAddress.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Service.hpp" "%TexCoreApiOutDir%Service.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Service.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Exceptions.hpp" "%TexCoreApiOutDir%Exceptions.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Exceptions.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "String.hpp" "%TexCoreApiOutDir%String.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "String.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "UniquePtr.hpp" "%TexCoreApiOutDir%UniquePtr.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "UniquePtr.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "SharedPtr.hpp" "%TexCoreApiOutDir%SharedPtr.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "SharedPtr.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Log.hpp" "%TexCoreApiOutDir%Log.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Log.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Locking.hpp" "%TexCoreApiOutDir%Locking.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Locking.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Filter.hpp" "%TexCoreApiOutDir%Filter.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Filter.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Listener.hpp" "%TexCoreApiOutDir%Listener.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Listener.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Acceptor.hpp" "%TexCoreApiOutDir%Acceptor.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Acceptor.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Instance.hpp" "%TexCoreApiOutDir%Instance.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Instance.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "IoHandle.hpp" "%TexCoreApiOutDir%IoHandle.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "IoHandle.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "ConnectionSignal.hpp" "%TexCoreApiOutDir%ConnectionSignal.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "ConnectionSignal.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "MessageBlock.hpp" "%TexCoreApiOutDir%MessageBlock.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "MessageBlock.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "DataTransferCommand.hpp" "%TexCoreApiOutDir%DataTransferCommand.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "DataTransferCommand.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Error.hpp" "%TexCoreApiOutDir%Error.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Error.hpp" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "Time.h" "%TexCoreApiOutDir%Time.h" | find "Files compare OK"
if errorlevel 1 echo copy /Y "Time.h" "%TexCoreApiOutDir%"
@if errorlevel 1 goto BuildApiError

echo N | comp "SslCertificatesStorage.hpp" "%TexCoreApiOutDir%SslCertificatesStorage.hpp" | find "Files compare OK"
if errorlevel 1 echo copy /Y "SslCertificatesStorage.hpp" "%TexCoreApiOutDir%"
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
