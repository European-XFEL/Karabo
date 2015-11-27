#!/bin/bash

# Script for fixing up the shebang lines of Python entry-point scripts
#
# Author: <john.wiggins@xfel.eu>
#

OS=$(uname -s)
if [ "$OS" = "Darwin" ]; then
  # Shebang lines don't need to change on OS X.
  exit 0
fi


PACKAGEDIR=$1

# TODO: Add an environment setup script to Karabo and use "/usr/bin/env python3" here.
NEW_SHEBANG_LINE="#!/usr/bin/env python3"
SED_PROGRAM='1 s%^.*$%'$NEW_SHEBANG_LINE'%g'

PYTHON_ENTRY_POINTS=(2to3 2to3-3.4 convert-karabo-device-project cygdb cython
    easy_install easy_install-3.4 f2py3 guidata-tests guiqwt-tests idle3
    idle3.4 ideviceclient ikarabo ipcluster ipcluster3 ipcontroller
    ipcontroller3 ipengine ipengine3 iptest iptest3 ipython ipython3
    karabo_device_server karabo_gui karaboServerControl.py nosetests
    nosetests-3.4 pip pip3 pip3.4 pnuke prsync pscp pslurp pssh
    pssh-askpass pydoc3 pydoc3.4 pygmentize pyvenv pyvenv-3.4 rst2html.py
    rst2latex.py rst2man.py rst2odt_prepstyles.py rst2odt.py rst2pseudoxml.py
    rst2s5.py rst2xetex.py rst2xml.py rstpep2html.py sift sphinx-apidoc
    sphinx-autogen sphinx-build sphinx-quickstart wheel
)

count=0
while [ "x${PYTHON_ENTRY_POINTS[count]}" != "x" ]
do
  script_name=${PYTHON_ENTRY_POINTS[count]}
  [ -f $PACKAGEDIR/extern/bin/$script_name ] && sed -i "$SED_PROGRAM" $PACKAGEDIR/extern/bin/$script_name
  count=$(($count + 1))
done
