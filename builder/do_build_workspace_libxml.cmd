
set LibxmlVersion=2.7.2
set LibxmlNameAndVersion=libxml2-%LibxmlVersion%
set LibxmlArchiveName=libxml2-sources-%LibxmlVersion%.tar
set LibxmlDir=%ExternalsDir%\libxml

7z x -y -bd -o"%TempDir%" "%ExternalsDir%\%LibxmlArchiveName%.gz" > nul
@if %errorlevel% neq 0 goto Error

7z x -y -bd -o"%TempDir%" "%TempDir%\%LibxmlArchiveName%" > nul
@if %errorlevel% neq 0 goto Error

xcopy "%LibxmlDir%\Makefile.%LibxmlVersion%.msvc" "%TempDir%\%LibxmlNameAndVersion%\win32\Makefile.msvc" /Y
@if %errorlevel% neq 0 goto Error

attrib -R "%LibxmlDir%\*" /S /D
@if %errorlevel% neq 0 goto Error

cd %TempDir%\%LibxmlNameAndVersion%\win32
@if %errorlevel% neq 0 goto Error

@set LibXmlComonConfigure=cscript configure.js ^
	ftp=no ^
	http=no ^
	html=no ^
	c14n=no ^
	catalog=no ^
	docb=no ^
	xpath=yes ^
	xptr=no ^
	xinclude=no ^
	iconv=no ^
	iso8859x=no ^
	zlib=no ^
	xml_debug=no ^
	mem_debug=no ^
	run_debug=no ^
	regexps=yes ^
	modules=no ^
	tree=yes ^
	reader=no ^
	writer=yes ^
	walker=no ^
	pattern=yes ^
	push=yes ^
	valid=no ^
	sax1=no ^
	legacy=no ^
	output=yes ^
	schemas=yes ^
	schematron=no ^
	python=no ^
	static=no ^
	dynruntime=no ^
	vcmanifest=yes ^
	bindir=. ^
	libdir="%LibDir%." ^
	prefix="%LibxmlDir%\%LibxmlNameAndVersion%" ^
	incdir="%LibxmlDir%\%LibxmlNameAndVersion%" ^
	sodir="%BinDir%."

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
%LibXmlComonConfigure% cruntime="/MDd" debug=yes
@if %errorlevel% neq 0 goto Error
nmake /NOLOGO /F Makefile.msvc install
@if %errorlevel% neq 0 goto Error
@goto BuildNext

:BuildRelease
%LibXmlComonConfigure% cruntime="/MD" debug=no
@if %errorlevel% neq 0 goto Error
nmake /NOLOGO /F Makefile.msvc install
@if %errorlevel% neq 0 goto Error
@goto BuildFinish
