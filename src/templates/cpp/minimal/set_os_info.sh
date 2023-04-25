#!/bin/bash
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.

#
# Sets the environment variables OS_DIST and
# OS_MAJOR_VERSION with the system distribution and
# version information, respectively.
# Tries to retrieve to information from "/etc/os-release"
# and if that is not available, uses "lsb_release" as a
# fallback. Both Ubuntu and RHEL based distributions like
# AlmaLinux have "/ect/os-release" with the information
# needed.
#

if [ -f "/etc/os-release" ]; then
    OS_DIST=$(cat "/etc/os-release" | awk -F= '{if($1 == "NAME") print $2}' | tr -d \" | awk '{print $1}')
    export OS_DIST
    OS_MAJOR_VERSION=$(cat "/etc/os-release" | awk -F= '{if($1 == "VERSION_ID") print $2}' | tr -d \" |  sed -r "s/^([0-9]+).*/\1/")
    export OS_MAJOR_VERSION
else
  eval which lsb_release &> /dev/null
  if [ $? = 0 ]; then
    # lsb_release is available; use it to set the variables
    OS_DIST=$(lsb_release -is)
    export OS_DIST
    OS_MAJOR_VERSION=$(lsb_release -rs | sed -r "s/^([0-9]+).*/\1/")
    export OS_MAJOR_VERSION
  fi
fi