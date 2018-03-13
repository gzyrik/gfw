#!/bin/sh

REALDIR=`readlink $0`
if [ -z "$REALDIR" ]; then
    PROGDIR=`dirname $0`
else
    PROGDIR=`dirname $REALDIR`
fi
PROGDIR=`cd $PROGDIR && pwd -P`
$PROGDIR/luajit $@
