
@Call builder\set_build_environment.cmd %*
@if %errorlevel%==0 Call "%SolutionDir%\do_build_workspace.cmd"
@Call "%BuilderDir%\do_build_finish.cmd"
