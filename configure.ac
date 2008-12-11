AC_INIT([fastcgi++], [ACTUALVERSION], [Eddie Carle <eddie@mailforce.net>], [fastcgi++])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_HEADER(include/fastcgi++/config.h)
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LIBTOOL

BOOST_REQUIRE([1.35.0])
BOOST_BIND
BOOST_DATE_TIME
BOOST_FUNCTION
BOOST_SMART_PTR
BOOST_THREADS

## Linux keeps its endian determination in endian.h
AC_CHECK_HEADER(endian.h,
				[AC_DEFINE(HAVE_ENDIAN_H, 1, [Using "endian.h"])],
				[])
## FreeBSD keeps its "endianess" in machine/endian.h
AC_CHECK_HEADER(machine/endian.h,
				[AC_DEFINE(HAVE_MACHINE_ENDIAN_H, 1,
					 [Using "machine/endian.h"])],
				[])

AC_OUTPUT([Makefile \
                   src/Makefile \
                   include/Makefile \
						 examples/Makefile])
