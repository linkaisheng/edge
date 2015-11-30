dnl $Id$
dnl config.m4 for extension edge

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(edge, for edge support,
dnl Make sure that the comment is aligned:
dnl [  --with-edge             Include edge support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(edge, whether to enable edge support,
Make sure that the comment is aligned:
[  --enable-edge           Enable edge support])

AC_ARG_ENABLE(edge-cli,
[  --enable-edge-cli     Enable edge-cli mode default=no],
[EDGE_CLI=$enableval],
[EDGE_CLI="no"])  

if test "$PHP_EDGE" != "no"; then
  if test "$EDGE_CLI" = "yes"; then
      AC_DEFINE(EDGE_CLI,1,[define to 1 if you want to change the POST/GET by php script])
  else 
      AC_DEFINE(EDGE_CLI,0,[define to 0 if you want to change the POST/GET by php script])
  fi
  dnl Write more examples of tests here...

  dnl # --with-edge -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/edge.h"  # you most likely want to change this
  dnl if test -r $PHP_EDGE/$SEARCH_FOR; then # path given as parameter
  dnl   EDGE_DIR=$PHP_EDGE
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for edge files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       EDGE_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$EDGE_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the edge distribution])
  dnl fi

  dnl # --with-edge -> add include path
  dnl PHP_ADD_INCLUDE($EDGE_DIR/include)

  dnl # --with-edge -> check for lib and symbol presence
  dnl LIBNAME=edge # you may want to change this
  dnl LIBSYMBOL=edge # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $EDGE_DIR/$PHP_LIBDIR, EDGE_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_EDGELIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong edge lib version or lib not found])
  dnl ],[
  dnl   -L$EDGE_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(EDGE_SHARED_LIBADD)

  PHP_NEW_EXTENSION(edge, edge.c edge_config.c edge_core.c edge_router.c edge_loader.c edge_controller.c, $ext_shared)
fi
