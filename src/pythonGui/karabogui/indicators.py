#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from PyQt5.QtCore import Qt
from PyQt5.QtGui import QColor, QIcon, QPainter, QPixmap

from karabo.common.api import State, ProxyStatus
from . import icons


# --------------------------------------------------------------------------
# Mapping states to colors


def get_state_color(value):
    """Return a state color for a given state string

    :param value: state string representation
    :type value: str
    """
    state = State(value)
    if state.isDerivedFrom(State.CHANGING):
        color = STATE_COLORS[State.CHANGING]
    elif state.isDerivedFrom(State.RUNNING):
        color = STATE_COLORS[State.RUNNING]
    elif state.isDerivedFrom(State.ACTIVE):
        color = STATE_COLORS[State.ACTIVE]
    elif state.isDerivedFrom(State.PASSIVE):
        color = STATE_COLORS[State.PASSIVE]
    elif state.isDerivedFrom(State.DISABLED):
        color = STATE_COLORS[State.DISABLED]
    elif state in [State.STATIC, State.NORMAL, State.ERROR, State.INIT]:
        color = STATE_COLORS[state]
    else:
        color = STATE_COLORS[State.UNKNOWN]

    return color


class _StateColors(Enum):
    UNKNOWN_COLOR = (255, 170, 0)
    KNOWN_NORMAL_COLOR = (200, 200, 200)
    INIT_COLOR = (230, 230, 170)
    DISABLED_COLOR = (255, 0, 255)
    ERROR_COLOR = (255, 0, 0)
    CHANGING_DECREASING_INCREASING_COLOR = (0, 170, 255)
    RUNNING_COLOR = (153, 204, 255)
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
    State.RUNNING: _StateColors.RUNNING_COLOR.value,
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
    elif state.isDerivedFrom(State.RUNNING):
        return _STATE_ICONS[State.RUNNING]
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
        painter.drawRect(0, 0, width - pen_width, height - pen_width)
        return QIcon(pix)


def get_state_icon_for_status(status):
    """Return a state icon which reflects the given `ProxyStatus`
    """
    unknown_statuses = (ProxyStatus.OFFLINE,
                        ProxyStatus.REQUESTED,
                        ProxyStatus.DEAD,
                        ProxyStatus.NOSERVER,
                        ProxyStatus.NOPLUGIN,
                        ProxyStatus.INCOMPATIBLE,
                        ProxyStatus.MISSING)

    if not isinstance(status, ProxyStatus):
        status = ProxyStatus(status)

    if status in unknown_statuses:
        return None

    state = State.ERROR if status is ProxyStatus.ERROR else State.ACTIVE
    # XXX: Maybe show more color options in the future
    return get_state_icon(state)


# --------------------------------------------------------------------------
# Mapping device status to icons

def get_device_status_icon(status, error=False):
    """A `QIcon` for the given `status` is returned.
    """
    status_icons = {
        ProxyStatus.OFFLINE: icons.deviceOffline,
        ProxyStatus.OK: icons.deviceAlive,
        ProxyStatus.ALIVE: icons.deviceAlive,
        ProxyStatus.REQUESTED: icons.device_requested,
        ProxyStatus.SCHEMA: icons.device_schema,
        ProxyStatus.DEAD: icons.device_dead,
        ProxyStatus.NOSERVER: icons.deviceOfflineNoServer,
        ProxyStatus.NOPLUGIN: icons.deviceOfflineNoPlugin,
        ProxyStatus.INCOMPATIBLE: icons.deviceIncompatible,
        ProxyStatus.MISSING: icons.propertyMissing,
        ProxyStatus.ERROR: icons.device_error
    }

    if not isinstance(status, ProxyStatus):
        status = ProxyStatus(status)

    if status == ProxyStatus.MONITORING and not error:
        return None
    elif status != ProxyStatus.OFFLINE and error:
        return status_icons.get(ProxyStatus.ERROR)

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
        ProxyStatus.ERROR: icons.deviceInstanceError,
        ProxyStatus.NOSERVER: icons.deviceOfflineNoServer,
        ProxyStatus.NOPLUGIN: icons.deviceOfflineNoPlugin,
        ProxyStatus.OFFLINE: icons.deviceOffline,
        ProxyStatus.REQUESTED: icons.deviceOffline,
        ProxyStatus.INCOMPATIBLE: icons.deviceIncompatible,
        ProxyStatus.MONITORING: icons.deviceMonitored,
    }

    if not isinstance(status, ProxyStatus):
        status = ProxyStatus(status)

    if status != ProxyStatus.OFFLINE and error:
        return status_icons.get(ProxyStatus.ERROR)

    return status_icons.get(status, icons.deviceInstance)


def get_project_server_status_icon(status):
    """An icon for the given server `status`.

    NOTE: These icons are for the Project Panel
    """
    status_icons = {
        ProxyStatus.ONLINE: icons.yes,
        ProxyStatus.OK: icons.yes,
        ProxyStatus.OFFLINE: icons.no,
    }

    if not isinstance(status, ProxyStatus):
        status = ProxyStatus(status)

    return status_icons.get(status)


# ---------------------------------
# Lamp coloring in the main window

PROC_FINE = 2
PROC_ALARM = 5
PROC_FINE_COLOR = (120, 255, 0, 200)
PROC_BETWEEN_COLOR = (255, 255, 102, 200)
PROC_ALARM_COLOR = (255, 0, 0, 200)


def get_processing_color(proc_delay):
    """Define the processing lamp coloring in the main window

    A three stage lamp is provided with basic colors: green, yellow, red
    """
    if proc_delay < PROC_FINE:
        return PROC_FINE_COLOR
    elif PROC_FINE <= proc_delay <= PROC_ALARM:
        return PROC_BETWEEN_COLOR
    elif proc_delay > PROC_ALARM:
        return PROC_ALARM_COLOR


# --------------------------------------------------------------------------
# Mapping topics to colors

def get_topic_color(topic=None):
    """Get the corresponding standard color belonging to karabo topic

    :param topic: The karabo topic string, e.g. SA1, FXE, SPB, SA2, etc.
    """
    color_map = {
        "SA1": (140, 170, 215, 128),
        "SA2": (215, 170, 150, 128),
        "SA3": (150, 185, 150, 128),
    }

    return color_map.get(topic, None)


# --------------------------------------------------------------------------
# Commonly used colors

OK_COLOR = (225, 242, 225, 128)
PROPERTY_READONLY_COLOR = (160, 160, 160, 255)
ALL_OK_COLOR = (224, 224, 224, 128)  # no alarm and fine
ERROR_COLOR_ALPHA = (255, 155, 155, 128)  # semitransparent
PROPERTY_ALARM_COLOR = (255, 125, 125, 128)
PROPERTY_WARN_COLOR = (255, 255, 125, 128)
PROPERTY_INTERLOCK_COLOR = (51, 51, 255, 128)
LOCKED_COLOR = (255, 145, 255, 128)


# --------------------------------------------------------------------------
# Mapping alarms to colors

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
