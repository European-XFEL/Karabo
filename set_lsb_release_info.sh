#!/bin/bash

# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

# Sets the environment variables LSB_RELEASE_DIST and
# LSB_RELEASE_VERSION with the system distribution and
# version information, respectively.
# Tries to retrieve to information from "/etc/os-release"
# and if that is not available, uses "lsb_release" as a
# fallback. Both Ubuntu and RHEL based distributions like
# AlmaLinux have "/ect/os-release" with the information
# needed.
#
export LSB_RELEASE_DIST=UNKNOWN
export LSB_RELEASE_VERSION="0"

if [ -f "/etc/os-release" ]; then
    LSB_RELEASE_DIST=$(cat "/etc/os-release" | awk -F= '{if($1 == "NAME") print $2}' | tr -d \" | awk '{print $1}')
    export LSB_RELEASE_DIST
    LSB_RELEASE_VERSION=$(cat "/etc/os-release" | awk -F= '{if($1 == "VERSION_ID") print $2}' | tr -d \")
    export LSB_RELEASE_VERSION
else
    eval which lsb_release &> /dev/null
    if [ $? = 0 ]; then
        # lsb_release is available; use it to set the variables
        LSB_RELEASE_DIST=$(lsb_release -is)
        export LSB_RELEASE_DIST
        LSB_RELEASE_VERSION=$(lsb_release -rs)
        export LSB_RELEASE_VERSION
    fi
fi
