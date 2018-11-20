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
CAPTURE="#![^#]*python.*"
SED_PROGRAM='1 s%^'$CAPTURE'$%'$NEW_SHEBANG_LINE'%'

for script_name in $PACKAGEDIR/extern/bin/*
do
  sed -i "$SED_PROGRAM" $script_name
done
