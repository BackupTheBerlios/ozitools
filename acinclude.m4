# Copyright © 2008 Dean Povey <povey@wedgetail.com>
AC_DEFUN([AC_CHECK_PERL_MODULES],[dnl
ac_perl_modules="$1"
# Make sure we have perl
if test -z "$PERL"; then
AC_PATH_PROG(PERL,perl)
fi

if test "x$PERL" != x; then
  ac_perl_modules_failed=0
  for ac_perl_module in $ac_perl_modules; do
    AC_MSG_CHECKING(for perl module $ac_perl_module)

    # Would be nice to log result here, but can't rely on autoconf internals
    $PERL "-M$ac_perl_module" -e exit > /dev/null 2>&1
    if test $? -ne 0; then
      AC_MSG_RESULT(no);
      ac_perl_modules_failed=1
   else
      AC_MSG_RESULT(ok);
    fi
  done

  # Run optional shell commands
  if test "$ac_perl_modules_failed" = 0; then
    :
    $2
  else
    :
    $3
  fi
else
  AC_MSG_WARN(could not find perl)
fi])dnl

dnl ---------------------------------------------------------------------------
dnl Macros for GDAL detection.
dnl ---------------------------------------------------------------------------

dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_GDAL
dnl
dnl adds support for --gdal-prefix, --gdal-exec-prefix, --with-gdaldir
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_GDAL],
[
    AC_ARG_WITH(gdaldir,
                [  --with-gdaldir=PATH       Use uninstalled version of GDAL in PATH],
                gdal_config_name="$withval/gdal-config")
    AC_ARG_WITH(gdal-config,
                [  --with-gdal-config=CONFIG gdal-config script to use (optional)],
                gdal_config_name="$withval" )
    AC_ARG_WITH(gdal-prefix,
                [  --with-gdal-prefix=PREFIX Prefix where GDAL is installed (optional)],
                gdal_config_prefix="$withval", gdal_config_prefix="")
])

dnl Helper macro for checking if GDAL version is at least $1.$2.$3, set's
dnl gdal_ver_ok=yes if it is:
AC_DEFUN([_GDAL_PRIVATE_CHECK_VERSION],
[
    gdal_ver_ok=""
    if test "x$GDAL_VERSION" != x ; then
      if test $gdal_config_major_version -gt $1; then
        gdal_ver_ok=yes
      else
        if test $gdal_config_major_version -eq $1; then
           if test $gdal_config_minor_version -gt $2; then
              gdal_ver_ok=yes
           else
              if test $gdal_config_minor_version -eq $2; then
                 if test $gdal_config_micro_version -ge $3; then
                    gdal_ver_ok=yes
                 fi
              fi
           fi
        fi
      fi
    fi
])

dnl ---------------------------------------------------------------------------
dnl AM_PATH_GDALCONFIG(VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl                  [, ADDITIONAL-GDAL-CONFIG-FLAGS]]])
dnl
dnl Test for GDAL, and define GDAL_C*FLAGS, GDAL_LIBS and GDAL_LIBS_STATIC
dnl (the latter is for static linking against GDAL). Set GDAL_CONFIG_NAME
dnl environment variable to override the default name of the gdal-config script
dnl to use. Set GDAL_CONFIG_PATH to specify the full path to gdal-config - in this
dnl case the macro won't even waste time on tests for its existence.
dnl
dnl Optional ADDITIONAL-GDAL-CONFIG-FLAGS argument is appended to gdal-config
dnl invocation command in present. It can be used to fine-tune lookup of
dnl best GDAL build available.
dnl
dnl Example use:
dnl   AM_PATH_GDALCONFIG([1.6.0], [gdal=1], [gdal=0])
dnl ---------------------------------------------------------------------------

dnl
dnl Get the cflags and libraries from the gdal-config script
dnl
AC_DEFUN([AM_PATH_GDALCONFIG],
[
  dnl do we have gdal-config name...
  if test x${GDAL_CONFIG_NAME+set} != xset ; then
     GDAL_CONFIG_NAME=gdal-config
  fi

  if test "x$gdal_config_name" != x ; then
     GDAL_CONFIG_NAME="$gdal_config_name"
  fi

  dnl deal with optional prefixes
  if test x$gdal_config_prefix != x ; then
     gdal_config_args="$gdal_config_args --prefix=$gdal_config_prefix"
     GDAL_LOOKUP_PATH="$GDAL_LOOKUP_PATH:$gdal_config_prefix/bin"
  fi

  dnl don't search the PATH if GDAL_CONFIG_NAME is absolute filename
  if test -x "$GDAL_CONFIG_NAME" ; then
     AC_MSG_CHECKING(for gdal-config)
     GDAL_CONFIG_PATH="$GDAL_CONFIG_NAME"
     AC_MSG_RESULT($GDAL_CONFIG_PATH)
  else
     AC_PATH_PROG(GDAL_CONFIG_PATH, $GDAL_CONFIG_NAME, no, "$GDAL_LOOKUP_PATH:$PATH")
  fi

  if test "$GDAL_CONFIG_PATH" != "no" ; then
    GDAL_VERSION=""

    min_gdal_version=ifelse([$1], ,1.5.3,$1)
    if test -z "$5" ; then
      AC_MSG_CHECKING([for GDAL version >= $min_gdal_version])
    else
      AC_MSG_CHECKING([for GDAL version >= $min_gdal_version ($5)])
    fi

    GDAL_CONFIG_WITH_ARGS="$GDAL_CONFIG_PATH $gdal_config_args $5 $4"

    GDAL_VERSION=`$GDAL_CONFIG_WITH_ARGS --version 2>/dev/null`
    gdal_config_major_version=`echo $GDAL_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\1/'`
    gdal_config_minor_version=`echo $GDAL_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\2/'`
    gdal_config_micro_version=`echo $GDAL_VERSION | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\).*/\3/'`

    gdal_requested_major_version=`echo $min_gdal_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gdal_requested_minor_version=`echo $min_gdal_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gdal_requested_micro_version=`echo $min_gdal_version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`

    _GDAL_PRIVATE_CHECK_VERSION([$gdal_requested_major_version],
                              [$gdal_requested_minor_version],
                              [$gdal_requested_micro_version])

    if test -n "$gdal_ver_ok"; then

      AC_MSG_RESULT(yes (version $GDAL_VERSION))
      GDAL_LIBS=`$GDAL_CONFIG_WITH_ARGS --libs`

      dnl is this even still appropriate?  --static is a real option now
      dnl and GDAL_CONFIG_WITH_ARGS is likely to contain it if that is
      dnl what the user actually wants, making this redundant at best.
      dnl For now keep it in case anyone actually used it in the past.
      dnl AC_MSG_CHECKING([for GDAL static library])
      dnl GDAL_LIBS_STATIC=`$GDAL_CONFIG_WITH_ARGS --static --libs 2>/dev/null`
      dnl if test "x$GDAL_LIBS_STATIC" = "x"; then
      dnl   AC_MSG_RESULT(no)
      dnl else
      dnl   AC_MSG_RESULT(yes)
      dnl fi

         GDAL_CFLAGS=`$GDAL_CONFIG_WITH_ARGS --cflags`
         GDAL_CPPFLAGS=$GDAL_CFLAGS
         GDAL_CXXFLAGS=$GDAL_CFLAGS

         GDAL_CFLAGS_ONLY=$GDAL_CFLAGS
         GDAL_CXXFLAGS_ONLY=$GDAL_CFLAGS

      ifelse([$2], , :, [$2])

    else

       if test "x$GDAL_VERSION" = x; then
          dnl no gdal-config at all
          AC_MSG_RESULT(no)
       else
          AC_MSG_RESULT(no (version $GDAL_VERSION is not new enough))
       fi

       GDAL_CFLAGS=""
       GDAL_CPPFLAGS=""
       GDAL_CXXFLAGS=""
       GDAL_LIBS=""
       GDAL_LIBS_STATIC=""
       ifelse([$3], , :, [$3])

    fi
  else

    GDAL_CFLAGS=""
    GDAL_CPPFLAGS=""
    GDAL_CXXFLAGS=""
    GDAL_LIBS=""
    GDAL_LIBS_STATIC=""
    GDAL_RESCOMP=""

    ifelse([$3], , :, [$3])

  fi

  AC_SUBST(GDAL_CPPFLAGS)
  AC_SUBST(GDAL_CFLAGS)
  AC_SUBST(GDAL_CXXFLAGS)
  AC_SUBST(GDAL_CFLAGS_ONLY)
  AC_SUBST(GDAL_CXXFLAGS_ONLY)
  AC_SUBST(GDAL_LIBS)
  AC_SUBST(GDAL_LIBS_STATIC)
  AC_SUBST(GDAL_VERSION)
])

