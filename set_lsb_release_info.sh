#!/bin/bash

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
    . /etc/os-release
    LSB_RELEASE_DIST=${NAME%% *}
    if [ "$LSB_RELEASE_DIST" = "Red" ]; then
       # Change the dist name value to match what the deployment script expects
       LSB_RELEASE_DIST="RedHat"
    fi
    export LSB_RELEASE_DIST
    LSB_RELEASE_VERSION=$VERSION_ID
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
