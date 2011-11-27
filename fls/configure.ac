AC_PREREQ([2.68])
AC_INIT([fls], [0.2], [h.v.raven@googlemail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
ACLOCAL_AMFLAGS=([-I.])
AC_PROG_LIBTOOL

AC_LANG([C++])

# Checks for programs.
AC_PROG_CXX

# Checks for header files.
AC_CHECK_HEADERS([pwd.h grp.h sys/types.h sys/stat.h])

# Checks for library functions
BOOST_REQUIRE([1.46.0]) # lower version run propably too, but not tested
BOOST_FILESYSTEM
BOOST_PROGRAM_OPTIONS
BOOST_THREADS

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
