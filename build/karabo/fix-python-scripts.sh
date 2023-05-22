#!/bin/bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

# Script for fixing up the shebang lines of Python entry-point scripts of
# Karabo entry points.
#
# Author: <john.wiggins@xfel.eu>
#

OS=$(uname -s)

PACKAGEDIR=$1

NEW_SHEBANG_LINE="#!/usr/bin/env python3"
CAPTURE="#![^#]*python.*"
SED_PROGRAM='1 s%^'$CAPTURE'$%'$NEW_SHEBANG_LINE'%'

for script_name in $PACKAGEDIR/extern/bin/*
do
  if grep -s -e '^'$CAPTURE'$' $script_name > /dev/null
  then
    sed -i "$SED_PROGRAM" $script_name
  fi
done
