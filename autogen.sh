#!/bin/sh
#aclocal
#autoconf
#autoheader

echo "Regenerating autotools files"
autoreconf --force --install || exit 1

automake --add-missing

libtoolize -f -c

echo "Setting up Intltool"
intltoolize --copy --force --automake || exit 1
