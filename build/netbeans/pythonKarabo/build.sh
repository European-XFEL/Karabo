#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
mkdir -p $DIST/lib
cd $DIST/lib
cp -rf ../../../../../../src/pythonKarabo .
rm -f `find . -type f -name *.pyc`
rm -rf `find . -type d -name .svn`
cd $CWD
