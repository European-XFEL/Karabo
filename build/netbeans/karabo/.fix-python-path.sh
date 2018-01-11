#!/bin/bash

# Script for fixing up the python path on several python packages
#
# Author: <alessandro.silenzi@xfel.eu>
#

PACKAGEDIR=$1

PYTHON_VERSION=3.4

OLD_PREFIX=$(cat $PACKAGEDIR/extern/lib/python${PYTHON_VERSION}/config-${PYTHON_VERSION}m/Makefile | grep '^prefix=*' | sed '1 s/prefix=//')

PYTHON_FILES=(config-${PYTHON_VERSION}m/Makefile site-packages/sipconfig.py config-${PYTHON_VERSION}m/python-config.py
)

SED_PROGRAM='s%'$OLD_PREFIX'%'$PACKAGEDIR'%g'
count=0

while [ "x${PYTHON_FILES[count]}" != "x" ]
do
  file_name=${PYTHON_FILES[count]}
  [ -f $PACKAGEDIR/extern/lib/python$PYTHON_VERSION/$file_name ] && sed -i "$SED_PROGRAM" $PACKAGEDIR/extern/lib/python$PYTHON_VERSION/$file_name
  count=$(($count + 1))
done
