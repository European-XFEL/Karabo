#!/bin/bash
CWD=$(pwd)
DIST=dist/$(uname -s)
# clean previous dist folder - in case of file/folders deletion/creation in oryginal source folder
rm -rf dist
mkdir -p $DIST/lib
cd $DIST/lib
cp -rf ../../../../../../src/pythonTools .
rm -f `find . -type f -name *.pyc`
cd $CWD
