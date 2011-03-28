
@if "%BuildWhat%" neq "all" goto %BuildWhat%

:OpenSSL
@echo ***********************************************************
@echo *                                                         *
@echo * Building OpenSSL...                                     *
@echo *                                                         *
@echo ***********************************************************
Call "%BuilderDir%\do_build_workspace_openssl.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


:ACE
@echo ***********************************************************
@echo *                                                         *
@echo * Building ACE framework...                               *
@echo *                                                         *
@echo ***********************************************************
Call "%BuilderDir%\do_build_workspace_ace.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


:wxWidgets
@echo ***********************************************************
@echo *                                                         *
@echo * Building wxWidgets library...                           *
@echo *                                                         *
@echo ***********************************************************
Call "%BuilderDir%\do_build_workspace_wxwidgets.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


:Boost
@echo ***********************************************************
@echo *                                                         *
@echo * Building Boost C++ libraries...                         *
@echo *                                                         *
@echo ***********************************************************
Call "%BuilderDir%\do_build_workspace_boost.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


:Libxml
@echo ***********************************************************
@echo *                                                         *
@echo * Building Libxml...                                      *
@echo *                                                         *
@echo ***********************************************************
Call "%BuilderDir%\do_build_workspace_libxml.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


:gSOAP
@echo ***********************************************************
@echo *                                                         *
@echo * Building gSOAP...                                       *
@echo *                                                         *
@echo ***********************************************************
Call "%SolutionDir%\do_build_workspace_gsoap.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


:MiniUPnP
@echo ***********************************************************
@echo *                                                         *
@echo * Building MiniUPnP...                                    *
@echo *                                                         *
@echo ***********************************************************
Call "%BuilderDir%\do_build_workspace_miniupnp.cmd"
@if %errorlevel% neq 0 goto Error
@if "%BuildWhat%" neq "all" goto End


@rem #############################################################
@rem ##
@rem ## End
@rem ##
@rem #############################################################


:Success
goto End

:Error

:End
