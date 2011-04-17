
set InputFile=%~1
set SolutionDir=%~2
set ConfigurationName=%~3
set OutDir=%~4
set IntDir=%~5
set ProjectDir=%~6
set BinDir=%SolutionDir%..\output\bin\

Call SetVars.cmd

if not exist "%OutDir%" mkdir "%OutDir%"
if not exist "%IntDir%" mkdir "%IntDir%"

@rem Candle.cmd uses to convert long pathes to 8.3 fromat.
Call Candle.cmd ^
	"%SolutionDir%" ^
	"%BinDir%" ^
	"%MsFrameworkDir%" ^
	"%IntDir%%InputFile%.wixobj" ^
	"%ProjectDir%%InputFile%"