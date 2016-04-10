dnl Autoconf support for C++

AC_DEFUN([ID3_CXX_WARNINGS],[
  AC_ARG_ENABLE(cxx-warnings, 
    [  --enable-cxx-warnings=[no/minimum/yes]	Turn on compiler warnings.],,enable_cxx_warnings=minimum)

  AC_MSG_CHECKING(what warning flags to pass to the C++ compiler)
  warnCXXFLAGS=
  if test "x$GCC" != xyes; then
    enable_compile_warnings=no
  fi
  if test "x$enable_cxx_warnings" != "xno"; then
    if test "x$GCC" = "xyes"; then
      case " $CXXFLAGS " in
      *[\ \	]-Wall[\ \	]*) ;;
      *) warnCXXFLAGS="-Wall -Wno-unused -Wno-inline -Woverloaded-virtual -Wmissing-declarations" ;;
      esac

      ## -W is not all that useful.  And it cannot be controlled
      ## with individual -Wno-xxx flags, unlike -Wall
      if test "x$enable_cxx_warnings" = "xyes"; then
	warnCXXFLAGS="$warnCXXFLAGS -Wmissing-prototypes -Wmissing-declarations -Woverloaded-virtual"
      fi
    fi
  fi
  AC_MSG_RESULT($warnCXXFLAGS)

   AC_ARG_ENABLE(iso-cxx,
     [  --enable-iso-cxx          Try to warn if code is not ISO C++ ],,
     enable_iso_cxx=no)

   AC_MSG_CHECKING(what language compliance flags to pass to the C++ compiler)
   complCXXFLAGS=
   if test "x$enable_iso_cxx" != "xno"; then
     if test "x$GCC" = "xyes"; then
      case " $CXXFLAGS " in
      *[\ \	]-ansi[\ \	]*) ;;
      *) complCXXFLAGS="$complCXXFLAGS -ansi" ;;
      esac

      case " $CXXFLAGS " in
      *[\ \	]-pedantic[\ \	]*) ;;
      *) complCXXFLAGS="$complCXXFLAGS -pedantic" ;;
      esac
     fi
   fi
  AC_MSG_RESULT($complCXXFLAGS)
  if test "x$cxxflags_set" != "xyes"; then
    CXXFLAGS="$CXXFLAGS $warnCXXFLAGS $complCXXFLAGS"
    cxxflags_set=yes
    AC_SUBST(cxxflags_set)
  fi
])


# -----------------------------------------------------------------------
# This macro tests the C++ compiler for various portability problem.
# 1. Defines CXX_HAS_NO_BOOL if the compiler does not support the bool
#    data type
# 2. Defines CXX_HAS_BUGGY_FOR_LOOPS if the compiler has buggy
#    scoping for the for-loop
# Seperately we provide some config.h.bot code to be added to acconfig.h
# that implements work-arounds for these problems.
# -----------------------------------------------------------------------

dnl ACCONFIG TEMPLATE
dnl #undef CXX_HAS_BUGGY_FOR_LOOPS
dnl #undef CXX_HAS_NO_BOOL
dnl END ACCONFIG

AC_DEFUN(ID3_CXX_PORTABILITY,[

  AC_PROVIDE([$0])

  dnl
  dnl Check for common C++ portability problems
  dnl

  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS

  dnl Check whether we have bool
  AC_MSG_CHECKING(whether C++ has bool)
  AC_TRY_RUN([main() { bool b1=true; bool b2=false; }],
             [ AC_MSG_RESULT(yes) ],
             [ AC_MSG_RESULT(no)
               AC_DEFINE(CXX_HAS_NO_BOOL) ],
             [ AC_MSG_WARN(Don't cross-compile)]
            )

  dnl Test whether C++ has buggy for-loops
  AC_MSG_CHECKING(whether C++ has correct scoping in for-loops)
  AC_TRY_COMPILE([#include <iostream.h>], [
   for (int i=0;i<10;i++) { }
   for (int i=0;i<10;i++) { }
], [ AC_MSG_RESULT(yes) ],
   [ AC_MSG_RESULT(no)
     AC_DEFINE(CXX_HAS_BUGGY_FOR_LOOPS) ])

  dnl Done with the portability checks
  AC_LANG_RESTORE
])

dnl ACCONFIG BOTTOM
dnl 
dnl // This file defines portability work-arounds for various proprietory
dnl // C++ compilers
dnl 
dnl // Workaround for compilers with buggy for-loop scoping
dnl // That's quite a few compilers actually including recent versions of
dnl // Dec Alpha cxx, HP-UX CC and SGI CC.
dnl // The trivial "if" statement provides the correct scoping to the 
dnl // for loop
dnl 
dnl #ifdef CXX_HAS_BUGGY_FOR_LOOPS
dnl #undef for
dnl #define for if(1) for
dnl #endif
dnl 
dnl //
dnl // If the C++ compiler we use doesn't have bool, then
dnl // the following is a near-perfect work-around. 
dnl // You must make sure your code does not depend on "int" and "bool"
dnl // being two different types, in overloading for instance.
dnl //
dnl 
dnl #ifdef CXX_HAS_NO_BOOL
dnl #define bool int
dnl #define true 1
dnl #define false 0
dnl #endif
dnl    
dnl END ACCONFIG

