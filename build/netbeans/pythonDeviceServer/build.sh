#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-pythondeviceserver <<End-of-file
#
# This file was automatically generated. Do not edit.
#
# Author: <burkhard.heisen@xfel.eu>
#
CWD=\$(pwd)
cd \$(dirname \$0)/../lib/pythonDeviceServer
export PYTHONPATH=../
export LD_LIBRARY_PATH=../../extern/lib
export DYLD_LIBRARY_PATH=../../extern/lib
export PATH=../../extern/bin
python device_server.py
cd \$CWD

End-of-file
chmod u+x karabo-pythondeviceserver
cd ../lib
cp -rf ../../../../../../src/pythonDeviceServer .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
