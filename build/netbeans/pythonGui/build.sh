#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
# clean previous dist folder - in case of file/folders deletion/creation in oryginal source folder
rm -rf dist
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-gui <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#
SCRIPT_DIR=\$(dirname \`[[ \$0 = /* ]] && echo "\$0" || echo "\$PWD/\${0#./}"\`)
KARABO_DIR=\$SCRIPT_DIR/..
OS=\$(uname -s)
export PATH=\$KARABO_DIR/extern/bin:\$PATH
export PYTHONPATH=\$KARABO_DIR/extern/lib/karabo_python
if [ "\$OS" = "Darwin" ]; then
    export PATH=/opt/local/bin:\$PATH
    export PYTHONPATH=\$KARABO_DIR/lib:\$PYTHONPATH
    export DYLD_LIBRARY_PATH=\$KARABO_DIR/lib:\$KARABO_DIR/extern/lib:\$DYLD_LIBRARY_PATH
fi

cd \$KARABO_DIR/lib/pythonGui
python3 karabo-gui.py "\$@"
cd -
End-of-file
chmod a+x karabo-gui
cd ../lib
cp -rf ../../../../../../src/pythonGui .
rm -f `find . -type f -name *.pyc`
cd $CWD
