AC_PREREQ([2.69])
AC_INIT([generate], [0.1], [derp])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([foreign subdir-objects])
LT_INIT([disable-shared])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_CPP

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h libintl.h stdint.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MMAP
AC_FUNC_REALLOC
AC_CHECK_FUNCS([dup2 memchr memset mkdir munmap pow setenv setlocale strcasecmp strchr strdup strrchr strstr strtol])

#(cd ..; . ./setup.sh)

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(SQLITE, sqlite3)
PKG_CHECK_MODULES(GIT, libgit2)

AC_CONFIG_FILES([Makefile])
AC_CONFIG_SUBDIRS([htmlish])
AC_OUTPUT
