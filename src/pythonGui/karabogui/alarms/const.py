#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on May 10, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import OrderedDict, namedtuple

from PyQt4.QtGui import QColor

from karabogui import icons


WARN_COLOR = (255, 255, 0)  # yellow
ALARM_COLOR = (255, 0, 0)  # red
NORM_COLOR = (100, 149, 237)  # fancy blue
INTERLOCK_COLOR = (51, 51, 255)  # blue

# --------------------------------------------------------------------------
# All available alarm entries

ALARM_ID = 'id'
TIME_OF_FIRST_OCCURENCE = 'timeOfFirstOccurrence'
TIME_OF_OCCURENCE = 'timeOfOccurrence'
DEVICE_ID = 'deviceId'
PROPERTY = 'property'
ALARM_TYPE = 'type'
DESCRIPTION = 'description'
NEEDS_ACKNOWLEDGING = 'needsAcknowledging'
ACKNOWLEDGEABLE = 'acknowledgeable'
ACKNOWLEDGE = 'acknowledge'  # puts together needsAcknowledging/acknowledgeable
SHOW_DEVICE = 'showDevice'

ALARM_DATA = OrderedDict()
ALARM_DATA[ALARM_ID] = 'ID'
ALARM_DATA[TIME_OF_FIRST_OCCURENCE] = 'Time of First Occurence'
ALARM_DATA[TIME_OF_OCCURENCE] = 'Time of Occurence'
ALARM_DATA[DEVICE_ID] = 'Device ID'
ALARM_DATA[PROPERTY] = 'Property'
ALARM_DATA[ALARM_TYPE] = 'Type'
ALARM_DATA[DESCRIPTION] = 'Description'
ALARM_DATA[ACKNOWLEDGE] = 'Acknowledge'
ALARM_DATA[SHOW_DEVICE] = 'Show Device'

AlarmEntry = namedtuple('AlarmEntry', [key for key in ALARM_DATA.keys()])

# --------------------------------------------------------------------------
# All available alarm update types

INIT_UPDATE_TYPE = 'init'
ADD_UPDATE_TYPE = 'add'
REMOVE_UPDATE_TYPE = 'remove'
UPDATE_UPDATE_TYPE = 'update'
ACKNOWLEGDABLE_UPDATE_TYPE = 'acknowledgeable'
DEVICE_KILLED_UPDATE_TYPE = 'deviceKilled'
REFUSE_ACKNOWLEDGEMENT_UPDATE_TYPE = 'refuseAcknowledgement'

# Tuples for convenience
ADD_ALARM_TYPES = (ADD_UPDATE_TYPE, INIT_UPDATE_TYPE, UPDATE_UPDATE_TYPE)
UPDATE_ALARM_TYPES = (
    INIT_UPDATE_TYPE, UPDATE_UPDATE_TYPE, ADD_UPDATE_TYPE,
    ACKNOWLEGDABLE_UPDATE_TYPE, REFUSE_ACKNOWLEDGEMENT_UPDATE_TYPE,
    DEVICE_KILLED_UPDATE_TYPE)
REMOVE_ALARM_TYPES = (ACKNOWLEGDABLE_UPDATE_TYPE, REMOVE_UPDATE_TYPE)

WARN_GLOBAL = 'warn'
WARN_LOW = 'warnLow'
WARN_HIGH = 'warnHigh'
WARN_VARIANCE_LOW = 'warnVarianceLow'
WARN_VARIANCE_HIGH = 'warnVarianceHigh'
ALARM_GLOBAL = 'alarm'
ALARM_LOW = 'alarmLow'
ALARM_HIGH = 'alarmHigh'
ALARM_VARIANCE_LOW = 'alarmVarianceLow'
ALARM_VARIANCE_HIGH = 'alarmVarianceHigh'
INTERLOCK = 'interlock'
ALARM_NONE = 'none'

# --------------------------------------------------------------------------
# Mapping alarm types to colors

ALARM_COLORS = {
    WARN_GLOBAL: QColor(*WARN_COLOR),
    WARN_LOW: QColor(*WARN_COLOR),
    WARN_HIGH: QColor(*WARN_COLOR),
    WARN_VARIANCE_LOW: QColor(*WARN_COLOR),
    WARN_VARIANCE_HIGH: QColor(*WARN_COLOR),
    ALARM_GLOBAL: QColor(*ALARM_COLOR),
    ALARM_LOW: QColor(*ALARM_COLOR),
    ALARM_HIGH: QColor(*ALARM_COLOR),
    ALARM_VARIANCE_LOW: QColor(*ALARM_COLOR),
    ALARM_VARIANCE_HIGH: QColor(*ALARM_COLOR),
    INTERLOCK: QColor(*INTERLOCK_COLOR),
}


def get_alarm_key_index(key):
    """ Return ``index`` position in ``ALARM_DATA`` OrderedDict for the given
        ``key``.
        If the ``key`` is not found, ``None`` is returned."""
    return list(ALARM_DATA.keys()).index(key)


def get_alarm_icon(alarm_type):
    """A `QIcon` for the given `alarm_type` is returned.
    """
    # NOTE: Declare the ALARM_ICONS dict here, because the icons module might
    # not yet be initialized the first time we are imported!
    ALARM_ICONS = {
        WARN_GLOBAL: icons.warnGlobal,
        WARN_LOW: icons.warnLow,
        WARN_HIGH: icons.warnHigh,
        WARN_VARIANCE_LOW: icons.warnVarianceLow,
        WARN_VARIANCE_HIGH: icons.warnVarianceHigh,
        ALARM_GLOBAL: icons.alarmGlobal,
        ALARM_LOW: icons.alarmLow,
        ALARM_HIGH: icons.alarmHigh,
        ALARM_VARIANCE_LOW: icons.alarmVarianceLow,
        ALARM_VARIANCE_HIGH: icons.alarmVarianceHigh,
        INTERLOCK: icons.interlock,
    }

    alarm_icon = ALARM_ICONS.get(alarm_type)
    if alarm_icon is not None:
        return alarm_icon


def get_alarm_pixmap(alarm_type, extent=16):
    """A `QPixmap` for the given `alarm_type` is returned.

    `extent` sets the size of the pixmap. The pixmap might be smaller than
    requested, but never larger.
    """
    if alarm_type is not None and alarm_type != ALARM_NONE:
        icon = get_alarm_icon(alarm_type)
        if icon is not None:
            return icon.pixmap(extent)
