
dnl Process this file with autoconf to produce a configure script.
AC_INIT(throttle, 1.1, james@klicman.org)
AM_INIT_AUTOMAKE(throttle, 1.1)
AM_CONFIG_HEADER(config.h)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

dnl Checks for libraries.

dnl Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h stdlib.h sys/time.h unistd.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([gettimeofday])
AC_SEARCH_LIBS(nanosleep, rt posix4, AC_DEFINE(HAVE_NANOSLEEP, 1,
    [Define to 1 if you have nanosleep]))

AC_MSG_CHECKING([whether usleep can sleep for a second or more])
usleep_check=yes
AC_RUN_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
#include <errno.h>]],
    [[if (!(usleep(1000000) != 0 && errno == EINVAL)) return 1;]])],
    [AC_DEFINE([HAVE_SUBSEC_USLEEP], 1,
               [Define to 1 if usleep with a value not smaller than 1000000 is an error.])
     usleep_check=no])
AC_MSG_RESULT([$usleep_check])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
