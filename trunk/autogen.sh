#! /bin/sh

aclocal -I build-aux\
&& autoheader \
&& automake --add-missing \
&& autoconf -I build-aux
