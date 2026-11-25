PHP_ARG_WITH([plutobook], [whether to build the PlutoBook extension],
  [AS_HELP_STRING([--with-plutobook], [Build the PlutoBook HTML-to-PDF extension])], [yes])

if test "$PHP_PLUTOBOOK" != "no"; then
  PKG_CHECK_MODULES([PLUTOBOOK], [plutobook = 0.20.0])
  PHP_EVAL_INCLINE([$PLUTOBOOK_CFLAGS])
  PHP_EVAL_LIBLINE([$PLUTOBOOK_LIBS], [PLUTOBOOK_SHARED_LIBADD])

  dnl The binding downloads remote assets through libcurl itself. PHP's curl extension
  dnl and PlutoBook's optional curl support are not used.
  PKG_CHECK_MODULES([CURL], [libcurl >= 7.68.0], [],
    [AC_MSG_ERROR([libcurl 7.68.0 or newer with HTTP and HTTPS support is required.
Install the libcurl development package (for example libcurl4-openssl-dev or
libcurl-devel), or point PKG_CONFIG_PATH at its libcurl.pc.])])
  AC_MSG_CHECKING([whether libcurl supports HTTP and HTTPS])
  plutobook_curl_protocols=`$PKG_CONFIG --variable=supported_protocols libcurl 2>/dev/null`
  case " $plutobook_curl_protocols " in
    *" HTTP "*)
      case " $plutobook_curl_protocols " in
        *" HTTPS "*) AC_MSG_RESULT([yes]) ;;
        *) AC_MSG_ERROR([libcurl was built without HTTPS (TLS) support: $plutobook_curl_protocols]) ;;
      esac
      ;;
    "  ") AC_MSG_RESULT([unknown, libcurl.pc does not list its protocols]) ;;
    *) AC_MSG_ERROR([libcurl was built without HTTP support: $plutobook_curl_protocols]) ;;
  esac
  PHP_EVAL_INCLINE([$CURL_CFLAGS])
  PHP_EVAL_LIBLINE([$CURL_LIBS], [PLUTOBOOK_SHARED_LIBADD])

  PHP_SUBST([PLUTOBOOK_SHARED_LIBADD])
  PHP_NEW_EXTENSION([plutobook], [src/document.c src/options.c src/assets.c src/network.c], [$ext_shared])
fi
