#!/bin/bash

INSTALL_PREFIX=$1
CWD=$(pwd)
DIR=`dirname $0`
cd ${DIR}

###
#
# Any dependency must install headers to:
#
#    $INSTALL_PREFIX/include
#
# and libraries to:
#
#    $INSTALL_PREFIX/lib
#
# Optionally binaries can be installed to:
#
#    $INSTALL_PREFIX/bin
#
# And documentation to:
#
#    $INSTALL_PREFIX/doc
#
###

SNMP_DIR=net-snmp-5.7

if [ ! -d $SNMP_DIR ]; then
    echo "Building $SNMP_DIR..."
    echo "Unpacking files, please wait..."
    tar -xzf ${SNMP_DIR}.tar.gz
fi
  cd $SNMP_DIR
  ./configure --prefix=${DIR}/${SNMP_DIR}/snmp --with-default-snmp-version=2 --with-sys-contact="European XFEL GmbH" --with-sys-location="Unknown" --with-logfile="/var/log/snmpd.log" --with-persistent-directory="/var/net-snmp" --without-perl-modules --disable-embedded-perl  2>&1 | tee configure.log
  
  make 2>&1 | tee make.log
  make install
  cp -rf snmp/include ${INSTALL_PREFIX}
  cp -rf snmp/lib ${INSTALL_PREFIX}

cd $CWD
