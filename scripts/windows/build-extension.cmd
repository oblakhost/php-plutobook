@echo off
setlocal
cd /d "%EXT_SOURCE%" || exit /b 1
call "%PHP_DEVEL%\phpize.bat" || exit /b 1
set "STATIC_CURL="
if "%PLUTOBOOK_LINKAGE%"=="static" set "STATIC_CURL=--with-plutobook-static-curl"
call configure.bat --with-php-build="%DEPS%" --with-plutobook=shared %STATIC_CURL% || exit /b 1
nmake /NOLOGO || exit /b 1
if "%PLUTOBOOK_LINKAGE%"=="static" dumpbin /nologo /dependents "x64\%PHP_BUILD_DIR%\php_plutobook.dll" > linkage.txt
exit /b %errorlevel%
