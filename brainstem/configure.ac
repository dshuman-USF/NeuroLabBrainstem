AC_PREREQ([2.67])
AC_INIT([brainstem],[1.1.13],[dshuman@usf.edu])
AM_INIT_AUTOMAKE()
AC_CONFIG_SRCDIR(brainstem.cpp)
AC_CONFIG_HEADERS([config.h])
${CPPFLAGS=""}
${CXXFLAGS=""}
${CFLAGS=""}
AC_PROG_CXX
AC_HEADER_STDC
AC_PROG_LN_S

PKG_PROG_PKG_CONFIG

# Check for Qt libraries
PKG_CHECK_MODULES(QT, [Qt5Core, Qt5Gui, Qt5Widgets Qt5OpenGL Qt5PrintSupport], [], [AC_MSG_ERROR([Qt libraries are required.])])

# Retrieve Qt compilation and linker flags
CPPFLAGS="`$PKG_CONFIG --cflags-only-I Qt5Core Qt5Gui Qt5Widgets Qt5OpenGL Qt5PrintSupport` $CPPFLAGS"
LDFLAGS="`$PKG_CONFIG --libs-only-L Qt5Core Qt5Gui Qt5Widgets Qt5OpenGL Qt5PrintSupport` $LDFLAGS"
LIBS="`$PKG_CONFIG --libs-only-l Qt5Core Qt5Gui Qt5Widgets Qt5OpenGL Qt5PrintSupport`  $LIBS"

if ! `$PKG_CONFIG --atleast-version=5.7.0 Qt5Core`; then
	AC_MSG_ERROR([Qt 5.7.0 or greater is required.])
fi

AC_CHECK_PROGS(MOC, [moc])
AC_CHECK_PROGS(UIC, [uic])
AC_CHECK_PROGS(RCC, [rcc])

if test -z "$MOC" || test -z "$UIC" || test -z "$RCC"; then 
	AC_MSG_ERROR([Qt utility programs moc, uic, and rcc are required.])
fi


AC_CHECK_PROGS([MXE_QMAKE],[x86_64-w64-mingw32.static-qmake-qt5])
if test -z "$MXE_QMAKE"; then
	AC_MSG_WARN([The MXE cross development environment is required to build the MS Windows version of brainstem (not fatal).  Consult the HOWTO_BUILD_FOR_WIN document included in this package.])
else
	AC_MSG_NOTICE([The MXE cross development environment is available.])
fi


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
