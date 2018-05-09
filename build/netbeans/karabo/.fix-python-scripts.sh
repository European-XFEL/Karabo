#!/bin/bash

# Script for fixing up the shebang lines of Python entry-point scripts of 
# Karabo entry points.
#
# Author: <john.wiggins@xfel.eu>
#

OS=$(uname -s)
if [ "$OS" = "Darwin" ]; then
  # Shebang lines don't need to change on OS X.
  exit 0
fi

PACKAGEDIR=$1

NEW_SHEBANG_LINE="#!/usr/bin/env python3"
SED_PROGRAM='1 s%^.*$%'$NEW_SHEBANG_LINE'%g'

PYTHON_ENTRY_POINTS=(convert-karabo-device convert-karabo-project
    ikarabo karabo karabo-cli karabo-gui karabo-middlelayerserver
    karabo-pythonserver karabo-start karabo-stop karabo-check
    karabo-xterm karabo-gterm karabo-add-deviceserver
    karabo-remove-deviceserver karabo-webserver karabo-kill
    pannel-runner karabo-scene2py
)

count=0
while [ "x${PYTHON_ENTRY_POINTS[count]}" != "x" ]
do
  script_name=${PYTHON_ENTRY_POINTS[count]}
  [ -f $PACKAGEDIR/extern/bin/$script_name ] && sed -i "$SED_PROGRAM" $PACKAGEDIR/extern/bin/$script_name
  count=$(($count + 1))
done
