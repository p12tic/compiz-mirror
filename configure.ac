AC_PREREQ(2.57)

AC_INIT([ccs-backend-gconf],esyscmd(. ./VERSION;echo -n $VERSION), [maniac@opencompositing.org])

AC_DEFINE_UNQUOTED(VERSION,"$VERSION",[Version])

#AC_CONFIG_AUX_DIR(config)

AM_INIT_AUTOMAKE([1.9 dist-bzip2])

#AC_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_LIBTOOL
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h sys/time.h unistd.h])

if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[[\ \	]]-Wall[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wpointer-arith[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wpointer-arith" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wstrict-prototypes[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wstrict-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wmissing-prototypes[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-prototypes" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wmissing-declarations[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-declarations" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-Wnested-externs[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -Wnested-externs" ;;
  esac

  case " $CFLAGS " in
  *[[\ \	]]-fno-strict-aliasing[[\ \	]]*) ;;
  *) CFLAGS="$CFLAGS -fno-strict-aliasing" ;;
  esac

  if test "x$enable_ansi" = "xyes"; then
    case " $CFLAGS " in
    *[[\ \	]]-ansi[[\ \	]]*) ;;
    *) CFLAGS="$CFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *[[\ \	]]-pedantic[[\ \	]]*) ;;
    *) CFLAGS="$CFLAGS -pedantic" ;;
    esac
  fi
fi

AC_C_BIGENDIAN

#LIBBERYLDECORATION_REQUIRES="xrender >= 0.8.4"
#PKG_CHECK_MODULES(LIBBERYLDECORATION, $LIBBERYLDECORATION_REQUIRES)
#AC_SUBST(LIBBERYLDECORATION_REQUIRES)

PKG_CHECK_MODULES(GLIB, "glib-2.0")
PKG_CHECK_MODULES(GCONF, "gconf-2.0")
PKG_CHECK_MODULES(CCS, "libcompizconfig")

AC_CONFIG_FILES([
Makefile
settings-backend/Makefile
])

AC_OUTPUT

