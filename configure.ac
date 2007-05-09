#
#	(C)2007 Dustin Spicuzza
#
#	http://autoconf-archive.cryp.to/macros-by-category.html
#

AC_INIT([gpslog],[0.1],[dustin@virtualroadside.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# checks for programs
AC_PROG_CC
CFLAGS="$CFLAGS -std=c99"

AC_HEADER_STDC

AC_CHECK_HEADERS([mysql/mysql.h],[],[AC_MSG_ERROR("mysql client libraries are not installed!")])
AC_CHECK_HEADERS([gps.h],[],[AC_MSG_ERROR("gpsd is not installed!")])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
	Makefile
	src/Makefile
])

MYSQL_LIBS="-lmysqlclient"
AC_SUBST(MYSQL_LIBS)

LIBGPS_LIBS="-llibgps"
AC_SUBST(LIBGPS_LIBS)

AC_OUTPUT

