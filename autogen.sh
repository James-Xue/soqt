#! /bin/sh
# **************************************************************************
# Regenerate all files which are constructed by the autoconf, automake
# and libtool tool-chain.
# Note: only developers should need to use this script.

# Authors:
#   Lars J. Aas <larsa@sim.no>
#   Morten Eriksen <mortene@sim.no>

directory=`echo "$0" | sed -e 's/[^\/]*$//g'`;
cd $directory
if test ! -f ./autogen.sh; then
  echo "unexpected problem with your shell - bailing out"
  exit 1
fi

AUTOCONF_VER=2.49b   # Autoconf from CVS
AUTOMAKE_VER=1.4a    # Automake from CVS
LIBTOOL_VER=1.3.5

GUI=Qt
PROJECT=So$GUI
MACRODIR=cfg/m4

SUBPROJECTS="$MACRODIR src/Inventor/Qt/common"
SUBPROJECTNAMES="SoQtMacros SoQtCommon"

AUTOMAKE_ADD=
if test "$1" = "--clean"; then
  rm -f config.h.in \
        configure \
        stamp-h*
  find . -name Makefile.in -print | xargs rm
  exit 0
elif test "$1" = "--add"; then
  AUTOMAKE_ADD=""
fi

echo "Checking the installed configuration tools..."

if test -z "`autoconf --version | grep \" $AUTOCONF_VER\" 2> /dev/null`"; then
  cat <<EOF

  You must have autoconf version $AUTOCONF_VER installed to
  generate configure information and Makefiles for $PROJECT.
  You can find it at:

    ftp://alpha.gnu.org/gnu/autoconf/autoconf-2.49a.tar.gz

EOF
    DIE=true
fi

if test -z "`automake --version | grep \" $AUTOMAKE_VER\" 2> /dev/null`"; then
    echo
    echo "You must have automake version $AUTOMAKE_VER installed to"
    echo "generate configure information and Makefiles for $PROJECT."
    echo ""
    echo "The Automake version we are using is a development version"
    echo "\"frozen\" from the CVS repository. You can get it here:"
    echo ""
    echo "   ftp://ftp.sim.no/pub/coin/automake-1.4a-coin.tar.gz"
    echo ""
    DIE=true
fi

if test -z "`libtool --version | egrep \"$LIBTOOL_VER\" 2> /dev/null`"; then
    echo
    echo "You must have libtool version $LIBTOOL_VER installed to"
    echo "generate configure information and Makefiles for $PROJECT."
    echo ""
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool/libtool-1.3.5.tar.gz"
    echo ""
    DIE=true
fi

set $SUBPROJECTNAMES
num=1
for project in $SUBPROJECTS; do
  test -d $project || {
    echo
    echo "Could not find subdirectory '$project'."
    echo "It was probably added after you initially fetched $PROJECT."
    echo "To add the missing module, run 'cvs co $1' from the $PROJECT"
    echo "base directory."
    echo ""
    echo "To do a completely fresh cvs checkout of the whole $PROJECT module,"
    echo "(if all else fails), remove $PROJECT and run:"
    echo "  cvs -z3 -d :pserver:cvs@cvs.sim.no:/export/cvsroot co -P $PROJECT"
    echo ""
    DIE=true
  }
  num=`expr $num + 1`
  shift
done

# abnormal exit?
${DIE=false} && echo "" && exit 1

# generate aclocal.m4
echo "Running aclocal..."
aclocal -I $MACRODIR

# generate config.h.in
echo "Running autoheader..."
autoheader

# generate Makefile.in templates
echo "Running automake..."
echo "[ignore any \"directory should not contain '/'\" warning]"
automake


fixmsg=0
for bugfix in `find . -name Makefile.in.diff`; do
  if test $fixmsg -eq 0; then
    echo "[correcting automake problems]"
    fixmsg=1
  fi
  patch -p0 < $bugfix
done

# generate configure
echo "Running autoconf..."
autoconf

if test -f configure.diff; then
  echo "[correcting autoconf problems]";
  patch -p0 < configure.diff;
fi

echo "Done."

# **************************************************************************
