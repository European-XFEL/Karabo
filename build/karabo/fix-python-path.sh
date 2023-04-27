#!/bin/bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

# Script for fixing up the python path on several python packages
#
# Author: <alessandro.silenzi@xfel.eu>
#

PACKAGEDIR=$1

MAKEFILE=$($PACKAGEDIR/extern/bin/python -c "import sysconfig; print(sysconfig.get_makefile_filename())")

OLD_PREFIX=$(cat $MAKEFILE | grep '^prefix=*' | sed '1 s/prefix=//')

PYTHON_FILES=($MAKEFILE site-packages/sipconfig.py $(dirname $MAKEFILE)/python-config.py)

SED_PROGRAM='s%'$OLD_PREFIX'%'$PACKAGEDIR/extern'%g'
count=0

while [ "x${PYTHON_FILES[count]}" != "x" ]
do
  file_name=${PYTHON_FILES[count]}
  [ -f $PACKAGEDIR/extern/lib/python$PYTHON_VERSION/$file_name ] && sed -i "$SED_PROGRAM" $PACKAGEDIR/extern/lib/python$PYTHON_VERSION/$file_name
  count=$(($count + 1))
done
