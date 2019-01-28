#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on November 7, 2011
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from xml.etree import ElementTree

from PyQt4.QtCore import Qt

ElementTree.register_namespace("xlink", "http://www.w3.org/1999/xlink")

# Commonly used colors
OK_COLOR = (225, 242, 225, 128)
PROPERTY_READONLY_COLOR = (160, 160, 160, 255)
ALL_OK_COLOR = (214, 214, 255, 64)  # no alarm and fine
ERROR_COLOR_ALPHA = (255, 155, 155, 128)  # semitransparent
PROPERTY_ALARM_COLOR = (255, 125, 125, 128)
PROPERTY_WARN_COLOR = (255, 255, 125, 128)
PROPERTY_INTERLOCK_COLOR = (51, 51, 255, 128)
LOCKED_COLOR = (255, 145, 255, 128)


PROPERTY_ALARM_COLOR_MAP = {
    None: ALL_OK_COLOR,
    'none': None,
    'alarm': PROPERTY_ALARM_COLOR,
    'alarmLow': PROPERTY_ALARM_COLOR,
    'alarmHigh': PROPERTY_ALARM_COLOR,
    'alarmVarianceLow': PROPERTY_ALARM_COLOR,
    'alarmVarianceHigh': PROPERTY_ALARM_COLOR,
    'warn': PROPERTY_WARN_COLOR,
    'warnLow': PROPERTY_WARN_COLOR,
    'warnHigh': PROPERTY_WARN_COLOR,
    'warnVarianceLow': PROPERTY_WARN_COLOR,
    'warnVarianceHigh': PROPERTY_WARN_COLOR,
    'interlock': PROPERTY_INTERLOCK_COLOR}

# Parameter item properties
INTERNAL_KEY = Qt.UserRole
VALUE_TYPE = Qt.UserRole + 1
DEFAULT_VALUE = Qt.UserRole + 2
CURRENT_INSTANCE_VALUE = Qt.UserRole + 3
CURRENT_EDITABLE_VALUE = Qt.UserRole + 4
ALIAS = Qt.UserRole + 5
TAGS = Qt.UserRole + 6
DESCRIPTION = Qt.UserRole + 7
TIMESTAMP = Qt.UserRole + 8
REQUIRED_ACCESS_LEVEL = Qt.UserRole + 9
IS_CHOICE_ELEMENT = Qt.UserRole + 10
IS_LIST_ELEMENT = Qt.UserRole + 11
UPDATE_NEEDED = Qt.UserRole + 12
ACCESS_TYPE = Qt.UserRole + 13
CLASS_ALIAS = Qt.UserRole + 14
ALLOWED_STATE = Qt.UserRole + 15
UNIT_SYMBOL = Qt.UserRole + 16
METRIC_PREFIX_SYMBOL = Qt.UserRole + 17
ENUMERATION = Qt.UserRole + 18

# WIDGET PROPERTIES
WIDGET_MIN_WIDTH = 80
WIDGET_MIN_HEIGHT = 20

# PLOT PROPERTIES
MAXNUMPOINTS = 1000
MAX_NUMBER_LIMIT = 1e30
