dnl $Id$
dnl config.m4 for extension iocron

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

IOCRON_VERSION="0.1.0"

dnl If your extension references something external, use with:

PHP_ARG_WITH(iocron, for iocron support,
dnl Make sure that the comment is aligned:
[  --with-iocron             Include iocron support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(iocron, whether to enable iocron support,
dnl Make sure that the comment is aligned:
[  --enable-iocron           Enable iocron support])

PHP_ARG_ENABLE(iocron-debug, debug mode,
dnl Make sure that the comment is aligned:
[  --enable-iocron-debug           iocron debug mode])

if test "$PHP_IOCRON_DEBUG" != "no"; then

  AC_MSG_RESULT(IOCron debug mode)
  PHP_DEFINE([PHP_IOCRON_DEBUG], [1])
else
  PHP_DEFINE([PHP_IOCRON_DEBUG], [0])
fi

if test "$PHP_IOCRON" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-iocron -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/iocron.h"  # you most likely want to change this
  dnl if test -r $PHP_IOCRON/$SEARCH_FOR; then # path given as parameter
  dnl   IOCRON_DIR=$PHP_IOCRON
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for iocron files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       IOCRON_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$IOCRON_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the iocron distribution])
  dnl fi

  dnl # --with-iocron -> add include path
  dnl PHP_ADD_INCLUDE($IOCRON_DIR/include)

  dnl # --with-iocron -> check for lib and symbol presence
  dnl LIBNAME=iocron # you may want to change this
  dnl LIBSYMBOL=iocron # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $IOCRON_DIR/$PHP_LIBDIR, IOCRON_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_IOCRONLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong iocron lib version or lib not found])
  dnl ],[
  dnl   -L$IOCRON_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  dnl PHP_SUBST(IOCRON_SHARED_LIBADD)

  AC_MSG_RESULT(yes)
  PHP_DEFINE([PHP_IOCRON_VERSION], [\"$IOCRON_VERSION\"])
  ifdef([PHP_ADD_EXTENSION_DEP], [PHP_ADD_EXTENSION_DEP(pdo_mysql, pdo, mysqlnd)])
	
  PHP_NEW_EXTENSION(iocron, iocron.c iocron_pdo_helper.c,$ext_shared)
  SAPI_IOCRON_EXECUTER_PATH=sapi/iocron_executer/iocron_executer
  
  IOCRON_SERVICE_PATH=/etc/init.d
  dnl IOCRON_SHARED_DEPENDENCIES="$IOCRON_SHARED_DEPENDENCIES compile-iocron-executer"
	
  dnl PHP_OUTPUT($srcdir/iocron_executer)
  PHP_ADD_MAKEFILE_FRAGMENT([Makefile.frag])
  
  PHP_SELECT_SAPI(iocron_executer, program, iocron_executer.c,, '$(SAPI_IOCRON_EXECUTER_PATH)')
  
  case $host_alias in
  *darwin*)
    BUILD_IOCRON_EXECUTER="\$(CC) \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(NATIVE_RPATHS) \$(PHP_GLOBAL_OBJS:.lo=.o) \$(PHP_BINARY_OBJS:.lo=.o) \$(PHP_IOCRON_EXECUTER_OBJS:.lo=.o) \$(PHP_FRAMEWORKS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_IOCRON_EXECUTER_PATH)"
    ;;
  *cygwin*)
    SAPI_IOCRON_EXECUTER_PATH=sapi/iocron_executer/iocron_executer.exe
    BUILD_IOCRON_EXECUTER="\$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(PHP_IOCRON_EXECUTER_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_IOCRON_EXECUTER_PATH)"
    ;;
  *)
    BUILD_IOCRON_EXECUTER="\$(LIBTOOL) --mode=link \$(CC) -export-dynamic \$(CFLAGS_CLEAN) \$(EXTRA_CFLAGS) \$(EXTRA_LDFLAGS_PROGRAM) \$(LDFLAGS) \$(PHP_RPATHS) \$(PHP_GLOBAL_OBJS) \$(PHP_BINARY_OBJS) \$(PHP_IOCRON_EXECUTER_OBJS) \$(EXTRA_LIBS) \$(ZEND_EXTRA_LIBS) -o \$(SAPI_IOCRON_EXECUTER_PATH)"
    ;;
  esac
  
  PHP_SUBST(IOCRON_SERVICE_PATH)
  PHP_SUBST(IOCRON_SHARED_DEPENDENCIES)
  PHP_SUBST(SAPI_IOCRON_EXECUTER_PATH)
  PHP_SUBST(BUILD_IOCRON_EXECUTER)
  dnl AC_CONFIG_FILES([$srcdir/iocron_executer])
  
  PHP_OUTPUT(ext/iocron/iocron_executer.1)
  
else
  AC_MSG_RESULT(no)
fi
