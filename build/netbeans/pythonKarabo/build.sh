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

# $(dirname $0) <==> $KARABO/bin
karabo=\$(dirname \$0)/..
export PYTHONPATH=\$karabo/lib
export LD_LIBRARY_PATH=\$karabo/extern/lib
export DYLD_LIBRARY_PATH=\$karabo/extern/lib
export PATH=\$karabo/extern/bin
python \$karabo/lib/pythonKarabo/device_server.py

End-of-file
chmod u+x karabo-pythondeviceserver
cd ../lib
cp -rf ../../../../../../src/pythonKarabo .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
