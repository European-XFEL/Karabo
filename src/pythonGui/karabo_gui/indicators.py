#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains constants for unified states and alarm indicators."""

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QIcon, QPainter, QPixmap

from . import icons
from karabo.common.states import State

# --------------------------------------------------------------------------
# Alarms

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

# --------------------------------------------------------------------------
# Mapping states to colors

UNKNOWN_COLOR = (255, 170, 0)
KNOWN_NORMAL_COLOR = (200, 200, 200)
INIT_COLOR = (230, 230, 170)
DISABLED_COLOR = (255, 0, 255)
ERROR_COLOR = (255, 0, 0)
CHANGING_DECREASING_INCREASING_COLOR = (0, 170, 255)
STATIC_COLOR = (170, 255, 127)
ACTIVE_COLOR = (0, 170, 0)
PASSIVE_COLOR = (204, 204, 255)

STATE_COLORS = {
    State.UNKNOWN: UNKNOWN_COLOR,
    State.KNOWN: KNOWN_NORMAL_COLOR,
    State.NORMAL: KNOWN_NORMAL_COLOR,
    State.INIT: INIT_COLOR,
    State.DISABLED: DISABLED_COLOR,
    State.ERROR: ERROR_COLOR,
    State.CHANGING: CHANGING_DECREASING_INCREASING_COLOR,
    State.DECREASING: CHANGING_DECREASING_INCREASING_COLOR,
    State.INCREASING: CHANGING_DECREASING_INCREASING_COLOR,
    State.STATIC: STATIC_COLOR,
    State.ACTIVE: ACTIVE_COLOR,
    State.PASSIVE: PASSIVE_COLOR
}


# --------------------------------------------------------------------------
# Mapping states to icons

# Use a lazily-initialized global. QPixmap needs a QApplication instance...
_STATE_ICONS = None


def get_state_icon(state):
    """ Return a colored icon for the given ``state``. A ``NoneType`` is
    returned in case the given ``state`` could not be found.
    """
    global _STATE_ICONS
    if _STATE_ICONS is None:
        _STATE_ICONS = {k: _create_state_icon(v)
                        for k, v in STATE_COLORS.items()}

    # Remark: order matters here - tree relation!
    if state.isDerivedFrom(State.PASSIVE):
        return _STATE_ICONS[State.PASSIVE]
    elif state.isDerivedFrom(State.ACTIVE):
        return _STATE_ICONS[State.ACTIVE]
    elif state.isDerivedFrom(State.DECREASING):
        return _STATE_ICONS[State.DECREASING]
    elif state.isDerivedFrom(State.INCREASING):
        return _STATE_ICONS[State.INCREASING]
    elif state.isDerivedFrom(State.STATIC):
        return _STATE_ICONS[State.STATIC]
    elif state.isDerivedFrom(State.CHANGING):
        return _STATE_ICONS[State.CHANGING]
    elif state.isDerivedFrom(State.DISABLED):
        return _STATE_ICONS[State.DISABLED]
    elif state.isDerivedFrom(State.ERROR):
        return _STATE_ICONS[State.ERROR]
    elif state.isDerivedFrom(State.NORMAL):
        return _STATE_ICONS[State.NORMAL]
    elif state.isDerivedFrom(State.KNOWN):
        return _STATE_ICONS[State.KNOWN]
    elif state.isDerivedFrom(State.UNKNOWN):
        return _STATE_ICONS[State.UNKNOWN]
    elif state.isDerivedFrom(State.INIT):
        return _STATE_ICONS[State.INIT]


def _create_state_icon(color):
    """ An icon from the given ``color`` tuple is returned."""
    width = 20
    height = 20
    pix = QPixmap(width, height)
    pix.fill(QColor(*color))
    with QPainter(pix) as painter:
        pen = painter.pen()
        pen_width = 1
        pen.setWidth(pen_width)
        pen.setColor(Qt.black)
        painter.setPen(pen)
        painter.drawRect(0, 0, width-pen_width, height-pen_width)
        return QIcon(pix)


# --------------------------------------------------------------------------
# Mapping device status to icons

STATUS_OFFLINE = 'offline'  # device could, but is not started
STATUS_ONLINE = 'online'  # the device is online but doesn't have a schema yet
STATUS_ALIVE = 'alive'  # everything is up-and-running
STATUS_MONITORING = 'monitoring'  # we are registered to monitor this device
STATUS_REQUESTED = 'requested'  # a schema is requested, but didnt arrive yet
STATUS_SCHEMA = 'schema'  # the device has a schema, but no value yet
STATUS_DEAD = 'dead'
STATUS_NOSERVER = 'noserver'  # device server not available
STATUS_NOPLUGIN = 'noplugin'  # class plugin not available
STATUS_INCOMPATIBLE = 'incompatible'  # device running, but of different type
STATUS_MISSING = 'missing'
STATUS_ERROR = 'error'


def get_device_status_icon(status, error):
    """A `QIcon` for the given `status` is returned.
    """
    status_icons = {
        STATUS_OFFLINE: icons.deviceOffline,
        STATUS_ALIVE: icons.deviceAlive,
        STATUS_REQUESTED: icons.device_requested,
        STATUS_SCHEMA: icons.device_schema,
        STATUS_DEAD: icons.device_dead,
        STATUS_NOSERVER: icons.deviceOfflineNoServer,
        STATUS_NOPLUGIN: icons.deviceOfflineNoPlugin,
        STATUS_INCOMPATIBLE: icons.deviceIncompatible,
        STATUS_MISSING: icons.propertyMissing,
        STATUS_ERROR: icons.device_error
    }

    if status == STATUS_MONITORING and not error:
        return None
    elif status != STATUS_OFFLINE and error:
        return status_icons.get('error')

    return status_icons.get(status)


def get_device_status_pixmap(status, error, extent=16):
    """A `QPixmap` for the given `status` is returned.

    `extent` sets the size of the pixmap. The pixmap might be smaller than
    requested, but never larger.
    """
    icon = get_device_status_icon(status, error)
    if icon is not None:
        return icon.pixmap(extent)
