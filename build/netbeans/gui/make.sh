#!/bin/bash
CWD=$(pwd)
cd ../../../src/pythonGui
export PYTHONPATH=../../build/netbeans/karabo/dist/Debug/GNU-Linux-x86/lib
../../extern/GNU-Linux-x86/bin/python setup.py build
cd $CWD
