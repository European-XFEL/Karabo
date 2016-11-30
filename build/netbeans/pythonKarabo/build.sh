#!/bin/bash
BUILD_OPTION=$2
OS=$(uname -s)
CWD=$(pwd)
DIST=dist/$OS

if [ "$OS" = "Darwin" ]; then
    PYTHON=/opt/local/bin/python
    KARABO=$($PYTHON -c 'import os,sys;print(os.path.realpath(sys.argv[1]))' "$1")
    PIP=/opt/local/bin/pip
    PIP_EXTRA_ARGS="--user"
else
    KARABO=$(readlink -f "$1")
    PYTHON=$KARABO/extern/bin/python
    PIP=$KARABO/extern/bin/pip
    PIP_EXTRA_ARGS=
fi


# clean previous dist folder - in case of file/folders deletion/creation in original source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin

cat > karabo-cli <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#
SCRIPT_DIR=\$(dirname \`[[ \$0 = /* ]] && echo "\$0" || echo "\$PWD/\${0#./}"\`)
KARABO=\$SCRIPT_DIR/..
OS=\$(uname -s)
export PATH=\$KARABO/extern/bin:\$PATH
export PYTHONPATH=\$KARABO/extern/lib/karabo_python
if [ "\$OS" = "Darwin" ]; then
    export PATH=/opt/local/bin:\$PATH
    export PYTHONPATH=\$KARABO/lib:\$PYTHONPATH
    export DYLD_LIBRARY_PATH=\$KARABO/lib:\$KARABO/extern/lib:\$DYLD_LIBRARY_PATH
fi

\$KARABO/extern/bin/ideviceclient "\$@"
End-of-file

cat > karabo-pythonserver <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#
SCRIPT_DIR=\$(dirname \`[[ \$0 = /* ]] && echo "\$0" || echo "\$PWD/\${0#./}"\`)
KARABO=\$SCRIPT_DIR/..
OS=\$(uname -s)
export PATH=\$KARABO/extern/bin:\$PATH
export PYTHONPATH=\$KARABO/extern/lib/karabo_python
if [ "\$OS" = "Darwin" ]; then
    export PATH=/opt/local/bin:\$PATH
    export PYTHONPATH=\$KARABO/lib:\$PYTHONPATH
    export DYLD_LIBRARY_PATH=\$KARABO/lib:\$KARABO/extern/lib:\$DYLD_LIBRARY_PATH
fi

\$KARABO/extern/bin/karabo-pythonserver "\$@"
End-of-file

chmod a+x karabo-cli
chmod a+x karabo-pythonserver

cd ../../../../../../src/pythonKarabo
rm -rf dist/ build/
if [ "$BUILD_OPTION" == "wheel" ]; then
    $PYTHON setup.py bdist_wheel
    $PIP --disable-pip-version-check install -U $PIP_EXTRA_ARGS dist/*.whl
else
    $PIP --disable-pip-version-check install $PIP_EXTRA_ARGS -e .
fi
cd $CWD
