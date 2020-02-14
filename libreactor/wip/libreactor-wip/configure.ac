AC_INIT([libreactor], [0.2.0], [fredrik.widlund@gmail.com])
AC_CONFIG_AUX_DIR([autotools])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define subdir-objects])

: ${CFLAGS="-Wall -Wextra -Wpedantic"}

CFLAGS="$CFLAGS -I$(pwd)/lib/{libdynamic,libreactor_core}/src"
LDFLAGS="$LDFLAGS -L$(pwd)/lib/{libdynamic,libreactor_core}"

export CFLAGS LDFLAGS

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([lib/libdynamic lib/libreactor_core])
#AC_CONFIG_SUBDIRS([lib/libdynamic lib/libreactor lib/libreactor-http lib/libreactor-rest])
AC_OUTPUT
