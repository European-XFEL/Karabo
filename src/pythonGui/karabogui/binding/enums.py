#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on July 6, 2022
# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
#############################################################################

from enum import Enum


class ProxyStatus(Enum):
    # device could, but is not started
    OFFLINE = "offline"
    # the device is online but doesn't have a schema yet
    ONLINE = "online"
    # online device waiting for its schema
    ONLINEREQUESTED = "onlinerequested"
    # everything is up-and-running
    ALIVE = "alive"
    # we are registered to monitor this device
    MONITORING = "monitoring"
    # a schema is requested, but didnt arrive yet
    REQUESTED = "requested"
    # the device has a schema, but no value yet
    SCHEMA = "schema"
    # device server not available
    NOSERVER = "noserver"
    # class plugin not available
    NOPLUGIN = "noplugin"
    # device running, but of different type
    INCOMPATIBLE = "incompatible"
    MISSING = "missing"


# The device is online in these statuses
ONLINE_STATUSES = (
    ProxyStatus.ONLINE,
    ProxyStatus.ALIVE,
    ProxyStatus.ONLINEREQUESTED,
    ProxyStatus.MONITORING,
    ProxyStatus.SCHEMA,
)
SCHEMA_STATUSES = (
    ProxyStatus.ALIVE,
    ProxyStatus.MONITORING,
    ProxyStatus.SCHEMA,
)
# The device is not able to receive configuration
NO_CONFIG_STATUSES = (
    ProxyStatus.NOPLUGIN,
    ProxyStatus.NOSERVER,
    ProxyStatus.REQUESTED,
)
NO_CLASS_STATUSES = (
    ProxyStatus.NOPLUGIN,
    ProxyStatus.NOSERVER
)

# The device can perform an online to offline configuration
ONLINE_CONFIG_STATUSES = (
    ProxyStatus.MONITORING,
    ProxyStatus.ALIVE,
)
