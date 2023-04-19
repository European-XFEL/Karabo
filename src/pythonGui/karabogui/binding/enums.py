#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on July 6, 2022
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
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
