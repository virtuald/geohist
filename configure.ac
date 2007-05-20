#
#	(C)2007 Dustin Spicuzza
#

AC_INIT([geoHist],[0.1.1],[dustin@virtualroadside.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC
CFLAGS="$CFLAGS -std=c99"

# options
AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],[enable debugging output]))

if test x$enable_debug = xyes; then
	CFLAGS="$CFLAGS -DDEBUG"
fi

# libraries
AC_CHECK_LIB([mysqlclient],[mysql_stmt_bind_param])
MYSQL_LIBS="-lmysqlclient"
AC_SUBST(MYSQL_LIBS)

AC_CHECK_LIB([gps],[gps_query])
LIBGPS_LIBS="-llibgps"
AC_SUBST(LIBGPS_LIBS)


# headers
AC_HEADER_STDC

AC_CHECK_HEADERS([mysql/mysql.h],[],[AC_MSG_ERROR("mysql client libraries are not installed!")])
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h linux/limits.h])

AC_CHECK_HEADERS([gps.h],[],[AC_MSG_ERROR("gpsd is not installed!")],
	[#ifdef HAVE_STDINT_H
	#include <stdint.h>
	#endif
	#ifdef HAVE_LINUX_LIMITS_H
	#include <linux/limits.h>
	#endif
	])

# types
AC_C_CONST
AC_TYPE_PID_T

# library functions
AC_FUNC_FORK
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([memset])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])


AC_OUTPUT

