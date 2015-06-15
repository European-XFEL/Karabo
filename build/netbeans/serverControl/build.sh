#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
# clean previous dist folder - in case of file/folders deletion/creation in oryginal source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-server-control <<End-of-file
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
    PYKARABO=\$KARABO/lib
else
    # Is a site-package in the shipped bundled python environment
    PYKARABO=\$(python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())")
fi

python3 \$PYKARABO/karabo/karaboServerControl.py "\$@"
End-of-file
chmod a+x karabo-server-control
cd ../lib
cp -rf ../../../../../../src/serverControl .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
