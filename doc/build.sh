#!/bin/bash
KARABO=$(readlink -f "$1")
VERSION=$2

export PATH=$KARABO/extern/bin:$PATH

cp conf.py .clean_conf.py
sed -i "s/release =.*/release = '${VERSION##*/}'/" conf.py
make clean
make html
make doxygen

mv -f .clean_conf.py conf.py
