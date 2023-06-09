# This file is part of Karabo.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# Karabo is free software: you can redistribute it and/or modify it under
# the terms of the MPL-2 Mozilla Public License.
#
# You should have received a copy of the MPL-2 Public License along with
# Karabo. If not, see <https://www.mozilla.org/en-US/MPL/2.0/>.
#
# Karabo is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
usage()
{
  cat <<EOF
Usage: $0 [options]
Options:
  --help       print this message
  --prefix=dir install directory
EOF
  exit 1
}

echo_exit()
{
  echo $1
  exit 1
}


interactive=TRUE
install_prefix_dir=""
for a in $@; do
  if echo $a | grep "^--prefix=" > /dev/null 2> /dev/null; then
     install_prefix_dir=`echo $a | sed "s/^--prefix=//"`
     install_prefix_dir=${install_prefix_dir/#\~/$HOME}
     if [ ! -d ${install_prefix_dir} ]; then
       mkdir -p  ${install_prefix_dir} ||  echo_exit "Cannot create directory ${install_prefix_dir}"
     fi
     install_prefix_dir=`cd "${install_prefix_dir}"; pwd`
  fi
  if echo $a | grep "^--help" > /dev/null 2> /dev/null; then
    usage
  fi
done

if [ "x${install_prefix_dir}x" != "xx" ]; then
  interactive=FALSE
fi
echo
echo " #####################################################################"
echo "                                INSTALLATION"
echo   
echo "                       Karabo plugin: $PLUGINNAME"
echo "       version: $VERSION, karaboFramework version: $KARABOVERSION"
echo                                            
echo " #####################################################################"
echo
installDir=$HOME/packages
if [ -z $KARABO ]; then
    echo "\$KARABO is not defined. Make sure you have sourced the activate script for the Karabo Framework which you would like to use."
    exit 1
else
    LOCALKARABOVERSION=$(cat $KARABO/VERSION)
    if [ "$LOCALKARABOVERSION" == "$KARABOVERSION" ]; then
       installDir=$KARABO/plugins
    else
       echo "Plugin was compiled with different karaboFramework version"
       echo "than installed one: $KARABOVERSION vs. $LOCALKARABOVERSION"
       echo " "
    fi
fi

if [ "x${install_prefix_dir}x" != "xx" ]
then
  installDir="${install_prefix_dir}"
fi

if [ "x${interactive}x" = "xTRUEx" ]
then
  read -e -p " Installation path [$installDir]: " dir
  dir=${dir/#\~/$HOME}
  if [ "x${dir}x" != "xx" ]; then
     if [ ! -d ${dir} ]; then
       mkdir -p  ${dir} ||  echo_exit "Cannot create directory $dir"
     fi
     installDir=`cd "${dir}"; pwd`
  else
     if [ ! -d ${installDir} ]; then
       mkdir -p  ${installDir} ||  echo_exit "Cannot create directory $installDir"
     fi
  fi
fi

echo -n " Extracting files, please wait..."
# searches for the line number where finish the script and start the tar.gz
SKIP=`awk '/^__TARFILE_FOLLOWS__/ { print NR + 1; exit 0; }' $0`
tail -n +$SKIP $0 | (cd  $installDir && tar xzf -) || echo_exit "Problem unpacking the file $0"
echo  " unpacking finished successfully"
echo
echo " Package was successfully installed to: $installDir"
echo
echo
exit 0
# NOTE: Don't place any newline characters after the last line below.
__TARFILE_FOLLOWS__
