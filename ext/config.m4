PHP_ARG_WITH([plutobook], [whether to build the PlutoBook extension],
  [AS_HELP_STRING([--with-plutobook], [Build the PlutoBook HTML-to-PDF extension])], [yes])

if test "$PHP_PLUTOBOOK" != "no"; then
  PKG_CHECK_MODULES([PLUTOBOOK], [plutobook = 0.20.0])
  PHP_EVAL_INCLINE([$PLUTOBOOK_CFLAGS])
  PHP_EVAL_LIBLINE([$PLUTOBOOK_LIBS], [PLUTOBOOK_SHARED_LIBADD])
  PHP_SUBST([PLUTOBOOK_SHARED_LIBADD])
  PHP_NEW_EXTENSION([plutobook], [src/document.c src/options.c src/assets.c], [$ext_shared])
fi
