@echo off
setlocal
rem Force the native Visual C++ generator tools even when Strawberry Perl is on PATH.
set "PATH=%VCINSTALLDIR%..\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VCINSTALLDIR%..\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "CURL_BUILD=-DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=OFF -DCURL_USE_OPENSSL=ON -DCURL_USE_SCHANNEL=OFF"
if not "%PLUTOBOOK_LINKAGE%"=="static" goto curl
cmake -G Ninja -S "%DEPS%\zlib-src" -B "%DEPS%\zlib-build" -DCMAKE_C_COMPILER=cl -DCMAKE_BUILD_TYPE=Release ^
 -DCMAKE_INSTALL_PREFIX="%DEPS%" -DZLIB_BUILD_SHARED=OFF -DZLIB_BUILD_STATIC=ON -DZLIB_BUILD_TESTING=OFF || exit /b 1
cmake --build "%DEPS%\zlib-build" --parallel 4 || exit /b 1
cmake --install "%DEPS%\zlib-build" || exit /b 1
copy /y "%DEPS%\lib\zs.lib" "%DEPS%\lib\zlib_a.lib" >nul || exit /b 1
set "CURL_BUILD=-DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DSTATIC_LIB_SUFFIX=_a -DCURL_USE_OPENSSL=OFF -DCURL_USE_SCHANNEL=ON"
:curl
cmake -G Ninja -S "%DEPS%\curl-src" -B "%DEPS%\curl-build" -DCMAKE_C_COMPILER=cl -DCMAKE_BUILD_TYPE=Release ^
 -DCMAKE_INSTALL_PREFIX="%DEPS%" %CURL_BUILD% -DBUILD_CURL_EXE=OFF ^
 -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF -DBUILD_LIBCURL_DOCS=OFF -DBUILD_MISC_DOCS=OFF ^
 -DCURL_DISABLE_DICT=ON -DCURL_DISABLE_FILE=ON -DCURL_DISABLE_FTP=ON -DCURL_DISABLE_GOPHER=ON ^
 -DCURL_DISABLE_IMAP=ON -DCURL_DISABLE_IPFS=ON -DCURL_DISABLE_LDAP=ON -DCURL_DISABLE_MQTT=ON ^
 -DCURL_DISABLE_POP3=ON -DCURL_DISABLE_RTSP=ON -DCURL_DISABLE_SMTP=ON ^
 -DCURL_DISABLE_TELNET=ON -DCURL_DISABLE_TFTP=ON -DCURL_DISABLE_WEBSOCKETS=ON -DOPENSSL_ROOT_DIR="%DEPS%" ^
 -DCURL_ZLIB=ON -DZLIB_LIBRARY="%DEPS%\lib\zlib_a.lib" -DZLIB_INCLUDE_DIR="%DEPS%\include" ^
 -DCURL_USE_LIBPSL=OFF -DCURL_BROTLI=OFF -DCURL_ZSTD=OFF -DUSE_NGHTTP2=OFF -DUSE_LIBIDN2=OFF || exit /b 1
cmake --build "%DEPS%\curl-build" --parallel 4 || exit /b 1
cmake --install "%DEPS%\curl-build" || exit /b 1
lib /nologo /machine:x64 /def:"%DEPS%\plutobook.def" /out:"%DEPS%\lib\plutobook.lib" || exit /b 1
