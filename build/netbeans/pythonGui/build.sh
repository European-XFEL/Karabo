#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
mkdir -p $DIST/bin
cd $DIST/bin
cat > karabo-gui <<End-of-file
#!/bin/bash
#
# This file was automatically generated. Do not edit.
#
SCRIPT_DIR=\$(dirname \`[[ \$0 = /* ]] && echo "\$0" || echo "\$PWD/\${0#./}"\`)
KARABO=\$SCRIPT_DIR/..
OS=\$(uname -s)
PATH=\$KARABO/extern/bin:\$PATH
PYTHONPATH=\$KARABO/extern/lib/karabo_python
if [ "\$OS" = "Darwin" ]; then
    PYTHONPATH=\$KARABO/lib:\$PYTHONPATH
    DYLD_LIBRARY_PATH=\$KARABO/lib:\$KARABO/extern/lib:\$DYLD_LIBRARY_PATH
fi

cd \$KARABO/lib/pythonGui  
python karabo-gui.py "\$@"
cd -
End-of-file
chmod u+x karabo-gui
cd ../lib
cp -rf ../../../../../../src/pythonGui .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
