@echo off
setlocal
cd /d "%PHP_SOURCE%" || exit /b 1
call buildconf.bat || exit /b 1
call configure.bat --disable-all --enable-cli --enable-zlib --with-openssl --with-php-build="%DEPS%" %PHP_THREAD_OPTION% --disable-debug --with-mp=4 || exit /b 1
nmake /NOLOGO || exit /b 1
nmake /NOLOGO build-devel
exit /b %errorlevel%
