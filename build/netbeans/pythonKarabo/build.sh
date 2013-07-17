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

karabo=\$(dirname \$0)/..
export PATH=\$karabo/extern/bin
export PYTHONPATH=\$karabo/extern/lib/karabo_python:\$karabo/lib
pythonLibPath=\$(python -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")
python \$pythonLibPath/karabo/device_server.py \$*

End-of-file
chmod u+x karabo-pythondeviceserver
cd ../lib
cp -rf ../../../../../../src/pythonKarabo .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
