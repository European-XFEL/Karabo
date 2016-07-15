#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################


"""This module contains a class which defines a certain number of constants for
   the application.
"""

from xml.etree import ElementTree
from PyQt4.QtCore import Qt


ns_svg = "{http://www.w3.org/2000/svg}"
ns_karabo = "{http://karabo.eu/scene}"
ElementTree.register_namespace("svg", ns_svg[1:-1])
ElementTree.register_namespace("krb", ns_karabo[1:-1])
ElementTree.register_namespace("xlink", "http://www.w3.org/1999/xlink")

# Commonly used colors
OK_COLOR = (225, 242, 225, 128)
ERROR_COLOR = (255, 155, 155, 128)

# Scene dimensions
SCENE_MIN_WIDTH = 1024
SCENE_MIN_HEIGHT = 768

# Parameter item properties
INTERNAL_KEY           = Qt.UserRole
VALUE_TYPE             = Qt.UserRole + 1
DEFAULT_VALUE          = Qt.UserRole + 2
CURRENT_INSTANCE_VALUE = Qt.UserRole + 3
CURRENT_EDITABLE_VALUE = Qt.UserRole + 4
ALIAS                  = Qt.UserRole + 5
TAGS                   = Qt.UserRole + 6
DESCRIPTION            = Qt.UserRole + 7
TIMESTAMP              = Qt.UserRole + 8
REQUIRED_ACCESS_LEVEL  = Qt.UserRole + 9
IS_CHOICE_ELEMENT      = Qt.UserRole + 10
IS_LIST_ELEMENT        = Qt.UserRole + 11
UPDATE_NEEDED          = Qt.UserRole + 12
ACCESS_TYPE            = Qt.UserRole + 13
CLASS_ALIAS            = Qt.UserRole + 14
ALLOWED_STATE          = Qt.UserRole + 15
UNIT_SYMBOL            = Qt.UserRole + 16
METRIC_PREFIX_SYMBOL   = Qt.UserRole + 17
ENUMERATION            = Qt.UserRole + 18

STATE_OFFLINE = 'offline'  # device could, but is not started
STATE_ONLINE = 'online'  # the device is online but doesn't have a schema yet
STATE_ALIVE = 'alive'  # everything is up-and-running
STATE_MONITORING = 'monitoring'  # we are registered to monitor this device
STATE_REQUESTED = 'requested'  # a schema is requested, but didnt arrive yet
STATE_SCHEMA = 'schema'  # the device has a schema, but no value yet
STATE_DEAD = 'dead'
STATE_NOSERVER = 'noserver'  # device server not available
STATE_NOPLUGIN = 'noplugin'  # class plugin not available
STATE_INCOMPATIBLE = 'incompatible'  # device running, but of different type
STATE_MISSING = 'missing'
STATE_ERROR = 'error'
