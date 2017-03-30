#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QColor, QIcon, QPainter, QPixmap

from karabo.common.api import State
from . import icons

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
NONE = 'none'

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


def get_alarm_icon(alarm_type):
    """A `QIcon` for the given `alarm_type` is returned.
    """
    alarm_icon = ALARM_ICONS.get(alarm_type)
    if alarm_icon is not None:
        return alarm_icon.icon


def get_alarm_pixmap(alarm_type, extent=16):
    """A `QPixmap` for the given `alarm_type` is returned.

    `extent` sets the size of the pixmap. The pixmap might be smaller than
    requested, but never larger.
    """
    if alarm_type is not None and alarm_type != NONE:
        icon = get_alarm_icon(alarm_type)
        if icon is not None:
            return icon.pixmap(extent)


# --------------------------------------------------------------------------
# Mapping states to colors

class _StateColors(Enum):
    UNKNOWN_COLOR = (255, 170, 0)
    KNOWN_NORMAL_COLOR = (200, 200, 200)
    INIT_COLOR = (230, 230, 170)
    DISABLED_COLOR = (255, 0, 255)
    ERROR_COLOR = (255, 0, 0)
    CHANGING_DECREASING_INCREASING_COLOR = (0, 170, 255)
    STATIC_COLOR = (0, 170, 0)
    ACTIVE_COLOR = (120, 255, 0)
    PASSIVE_COLOR = (204, 204, 255)


STATE_COLORS = {
    State.UNKNOWN: _StateColors.UNKNOWN_COLOR.value,
    State.KNOWN: _StateColors.KNOWN_NORMAL_COLOR.value,
    State.NORMAL: _StateColors.KNOWN_NORMAL_COLOR.value,
    State.INIT: _StateColors.INIT_COLOR.value,
    State.DISABLED: _StateColors.DISABLED_COLOR.value,
    State.ERROR: _StateColors.ERROR_COLOR.value,
    State.CHANGING: _StateColors.CHANGING_DECREASING_INCREASING_COLOR.value,
    State.DECREASING: _StateColors.CHANGING_DECREASING_INCREASING_COLOR.value,
    State.INCREASING: _StateColors.CHANGING_DECREASING_INCREASING_COLOR.value,
    State.STATIC: _StateColors.STATIC_COLOR.value,
    State.ACTIVE: _StateColors.ACTIVE_COLOR.value,
    State.PASSIVE: _StateColors.PASSIVE_COLOR.value
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


def get_state_icon_for_status(status_str):
    """Return a state icon which reflects the given ``status_str``
    """
    unknown_statuses = (DeviceStatus.STATUS_OFFLINE.value,
                        DeviceStatus.STATUS_REQUESTED.value,
                        DeviceStatus.STATUS_DEAD.value,
                        DeviceStatus.STATUS_NOSERVER.value,
                        DeviceStatus.STATUS_NOPLUGIN.value,
                        DeviceStatus.STATUS_INCOMPATIBLE.value,
                        DeviceStatus.STATUS_MISSING.value)
    if status_str in unknown_statuses:
        return None

    error = DeviceStatus.STATUS_ERROR.value
    state = State.ERROR if status_str == error else State.ACTIVE
    # XXX: Maybe show more color options in the future
    return get_state_icon(state)


# --------------------------------------------------------------------------
# Mapping device status to icons

class DeviceStatus(Enum):
    # device could, but is not started
    STATUS_OFFLINE = 'offline'
    # the device is online, but no detailed information retrieved yet
    STATUS_OK = 'ok'
    # the device is online but doesn't have a schema yet
    STATUS_ONLINE = 'online'
    # everything is up-and-running
    STATUS_ALIVE = 'alive'
    # we are registered to monitor this device
    STATUS_MONITORING = 'monitoring'
    # a schema is requested, but didnt arrive yet
    STATUS_REQUESTED = 'requested'
    # the device has a schema, but no value yet
    STATUS_SCHEMA = 'schema'
    # the device is dead
    STATUS_DEAD = 'dead'
    # device server not available
    STATUS_NOSERVER = 'noserver'
    # class plugin not available
    STATUS_NOPLUGIN = 'noplugin'
    # device running, but of different type
    STATUS_INCOMPATIBLE = 'incompatible'
    STATUS_MISSING = 'missing'
    STATUS_ERROR = 'error'


def get_device_status_icon(status, error=False):
    """A `QIcon` for the given `status` is returned.
    """
    status_icons = {
        DeviceStatus.STATUS_OFFLINE: icons.deviceOffline,
        DeviceStatus.STATUS_OK: icons.deviceAlive,
        DeviceStatus.STATUS_ALIVE: icons.deviceAlive,
        DeviceStatus.STATUS_REQUESTED: icons.device_requested,
        DeviceStatus.STATUS_SCHEMA: icons.device_schema,
        DeviceStatus.STATUS_DEAD: icons.device_dead,
        DeviceStatus.STATUS_NOSERVER: icons.deviceOfflineNoServer,
        DeviceStatus.STATUS_NOPLUGIN: icons.deviceOfflineNoPlugin,
        DeviceStatus.STATUS_INCOMPATIBLE: icons.deviceIncompatible,
        DeviceStatus.STATUS_MISSING: icons.propertyMissing,
        DeviceStatus.STATUS_ERROR: icons.device_error
    }

    if not isinstance(status, DeviceStatus):
        status = DeviceStatus(status)

    if status == DeviceStatus.STATUS_MONITORING and not error:
        return None
    elif status != DeviceStatus.STATUS_OFFLINE and error:
        return status_icons.get(DeviceStatus.STATUS_ERROR)

    return status_icons.get(status)


def get_device_status_pixmap(status, error, extent=16):
    """A `QPixmap` for the given `status` is returned.

    `extent` sets the size of the pixmap. The pixmap might be smaller than
    requested, but never larger.
    """
    icon = get_device_status_icon(status, error=error)
    if icon is not None:
        return icon.pixmap(extent)


def get_project_device_status_icon(status, error=False):
    """An icon for the given device `status`.

    NOTE: These icons are for the Project Panel
    """
    status_icons = {
        DeviceStatus.STATUS_ERROR: icons.deviceInstanceError,
        DeviceStatus.STATUS_NOSERVER: icons.deviceOfflineNoServer,
        DeviceStatus.STATUS_NOPLUGIN: icons.deviceOfflineNoPlugin,
        DeviceStatus.STATUS_OFFLINE: icons.deviceOffline,
        DeviceStatus.STATUS_INCOMPATIBLE: icons.deviceIncompatible,
    }

    if not isinstance(status, DeviceStatus):
        status = DeviceStatus(status)

    if status != DeviceStatus.STATUS_OFFLINE and error:
        return status_icons.get(DeviceStatus.STATUS_ERROR)

    return status_icons.get(status, icons.deviceInstance)


def get_project_server_status_icon(status):
    """An icon for the given server `status`.

    NOTE: These icons are for the Project Panel
    """
    status_icons = {
        DeviceStatus.STATUS_ONLINE: icons.yes,
        DeviceStatus.STATUS_OK: icons.yes,
        DeviceStatus.STATUS_OFFLINE: icons.no,
    }

    if not isinstance(status, DeviceStatus):
        status = DeviceStatus(status)

    return status_icons.get(status)
