set ProjectDir=%~1
Call SetApiDir.cmd
if %errorlevel%==0 (
	if exist "%TexCoreApiOutDir%" rmdir /Q /S "%TexCoreApiOutDir%"
)
@Verify > nul
