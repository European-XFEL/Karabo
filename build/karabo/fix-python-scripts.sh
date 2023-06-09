#!/bin/bash
# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.

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
