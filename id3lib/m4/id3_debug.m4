AC_DEFUN([ID3_DEBUG],[
  AC_ARG_ENABLE(debug, [  --enable-debug=[no/minimum/yes] turn on debugging [default=$debug_default]],,enable_debug=$debug_default)

  if test "x$enable_debug" = "xyes"; then
    test "$cflags_set" = set || CFLAGS="$CFLAGS -g"
    AC_DEFINE(ID3_ENABLE_DEBUG)
  else
    if test "x$enable_debug" = "xno"; then
      AC_DEFINE(ID3_DISABLE_ASSERT)
      AC_DEFINE(ID3_DISABLE_CHECKS)
    fi
  fi
])

dnl ACCONFIG TEMPLATE
dnl #undef ID3_ENABLE_DEBUG
dnl #undef ID3_DISABLE_ASSERT
dnl #undef ID3_DISABLE_CHECKS
dnl END ACCONFIG
dnl ACCONFIG BOTTOM
dnl #if defined (ID3_ENABLE_DEBUG) && defined (HAVE_LIBCW_SYS_H) && defined (__cplusplus)
dnl 
dnl #define DEBUG
dnl 
dnl #include <libcw/sys.h>
dnl #include <libcw/debug.h>
dnl 
dnl #define ID3D_INIT_DOUT()    Debug( libcw_do.on() )
dnl #define ID3D_INIT_WARNING() Debug( dc::warning.on() )
dnl #define ID3D_INIT_NOTICE()  Debug( dc::notice.on() )
dnl #define ID3D_NOTICE(x)      Dout( dc::notice, x )
dnl #define ID3D_WARNING(x)     Dout( dc::warning, x )
dnl 
dnl #else
dnl 
dnl #  define ID3D_INIT_DOUT()
dnl #  define ID3D_INIT_WARNING()
dnl #  define ID3D_INIT_NOTICE()
dnl #  define ID3D_NOTICE(x)
dnl #  define ID3D_WARNING(x)
dnl 
dnl #endif /* defined (ID3_ENABLE_DEBUG) && defined (HAVE_LIBCW_SYS_H) */
dnl    
dnl END ACCONFIG

