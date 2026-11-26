@echo off
setlocal
rem Keep the host Windows SDK but use the private compiler, linker, STL and runtime libraries.
set "VCToolsInstallDir=%PLUTOBOOK_MSVC%\"
for %%i in ("%PLUTOBOOK_MSVC%") do set "VCToolsVersion=%%~nxi"
set "PHP_SDK_VS=%PLUTOBOOK_COMPILER%"
set "PHP_SDK_VS_NUM=%PLUTOBOOK_COMPILER:~2%"
set "PATH=%PLUTOBOOK_MSVC%\bin\Hostx64\x64;%PATH%"
set "INCLUDE=%PLUTOBOOK_MSVC%\include;%INCLUDE%"
set "LIB=%PLUTOBOOK_MSVC%\lib\x64;%LIB%"
set "LIBPATH=%PLUTOBOOK_MSVC%\lib\x64;%LIBPATH%"
call "%PLUTOBOOK_TASK%"
exit /b %errorlevel%
