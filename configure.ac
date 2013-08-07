AC_INIT([QPicoscope],[0.1],[vincent.hervieux@free.fr])
# place to put some extra build scripts installed
AC_CONFIG_AUX_DIR([build-aux])
# local m4 scripts
AC_CONFIG_MACRO_DIR([build-aux])
# set all warnings ON:
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
# Check for programs
AC_PROG_CC
# Look for a C++ compiler.
AC_LANG([C++])
AC_PROG_CXX
AC_CHECK_PROGS([QMAKE], [qmake-qt4 qmake], [:])
if test "$QMAKE" = :; then
    AC_MSG_ERROR([This package needs qmake.])
fi
#check for headers
AC_CHECK_HEADERS([pthread.h])

# Check for libraries -lm -lps2000 -lps2000a -lps3000 -lqwt-qt4 -lQtGui -lQtCore -lpthread
libps2000_ok=yes
libps2000a_ok=yes
libps3000_ok=yes
AC_CHECK_LIB([ps2000], [ps2000_open_unit],,[libps2000_ok=no])
AC_CHECK_LIB([ps2000a], [ps2000aOpenUnit],,[libps2000a_ok=no])
AC_CHECK_LIB([ps3000], [ps3000_open_unit],,[libps3000_ok=no])
if test "x$libps2000_ok" = "xno"; then
    if test "x$libps2000a_ok" = "xno"; then
        if test "x$libps3000_ok" = "xno"; then
        AC_MSG_ERROR([Need at least one of the following: libps2000, libps2000a or libps3000 from Picotech.\
It can be found either in lib directory or on Picotech's website.])
        fi
    fi
fi
#AC_CHECK_LIB([qwt-qt4], [_init],,AC_MSG_ERROR([This package needs libqwt-qt4]))
#AC_CHECK_LIB([pthread], [pthread_create],,AC_MSG_ERROR([This package needs POSIX libpthread.]))


qt_ok=yes
AC_PATH_PROGS([QMAKE], 
        [qmake-qt4 qmake-4.4 qmake-mac qmake],
        [not-found])

    if test "x$QMAKE" = "xnot-found" ; then
        AC_MSG_WARN([could not find qmake for QT4 ])
        qt_ok=no
    else
        AC_SUBST([QMAKE], [$ac_cv_path_QMAKE])
        AC_FIND_QWT(5.0.0,4,
            [qwt-qt4 qwt], 
            [/usr/include/qwt5 /usr/include/qwt-qt4 /opt/local/include /opt/local/include/qwt /usr/include/qwt],
            [/opt/local/lib],
            [qt_ok=no])
    fi

    if test "x$qt_ok" = "xno"; then
        AC_MSG_ERROR([QT or QWT could not be found. See previous errors.])
    fi

# AutoTroll with Qt.
AT_WITH_QT

# Output files
AC_CONFIG_HEADERS(qpicoscope-config.h)
AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
