
@if %errorlevel% neq 0 goto Error

@echo ***********************************************************
@echo *                                                         *
@echo *                                                         *
@echo *                                                         *
@echo *        The build has been successfully finished.        *       
@echo *                                                         *
@echo *                                                         *
@echo *                                                         *
@echo ***********************************************************
@goto End


:Error
@if "%IsHelpMode%"=="false" (
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo !!!!                                                   !!!!
	@echo !!!!                                                   !!!!
	@echo !!!!        WARNING! The build has been failed!        !!!!
	@echo !!!!                                                   !!!!
	@echo !!!!                                                   !!!!
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	@echo !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
)
@if "%IsHelpMode%"=="true" goto FinalEnd


:End
@if "%IsNoClean%"=="false" goto Clean

:FinalEnd
@cd "%InitialDir%"
@exit /B 0


:Clean

attrib +R "%ExternalsDir%\*" /S
@if %errorlevel% neq 0 echo !!! Could not set read-only attributes for externals!

@if "%TempDir%"=="" (
	@echo TempDir is root!
	@exit /B 1
)
@if "%TempDir%"=="\" (
	@echo TempDir is root!
	@exit /B 1
)
@if "%TempDir%"=="/" (
	@echo TempDir is root!
	@exit /B 1
)
@if exist "%TempDir%" (
	rmdir /S /Q "%TempDir%"
	@if %errorlevel% neq 0 echo !!! Could not clear temp-directory! Error!
	@if exist "%TempDir%" echo !!! Could not clear temp-directory!
)

goto FinalEnd