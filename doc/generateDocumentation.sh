#!/bin/bash

# Script for automated building and packaging of the entire karaboFramework
#
# Author: <irina.kozlova@xfel.eu>
#

# Make sure the script runs in the correct directory
scriptDir=$(dirname `[[ $0 = /* ]] && echo "$0" || echo "$PWD/${0#./}"`)
cd ${scriptDir}
if [ $? -ne 0 ]; then
    echo " Could not change directory to ${scriptDir}"
    exit 1;
fi

if [ $(which doxygen) ]; then
    
    doxygen Doxyfile

else

    echo 
    echo "You need to install doxygen to generate this documentation."
    echo
fi

exit