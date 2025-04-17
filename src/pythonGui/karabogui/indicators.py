#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
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
from pathlib import Path

from qtpy.QtGui import QIcon

import karabogui.access as krb_access
from karabo.common.api import InstanceStatus, State
from karabogui.binding.api import ProxyStatus

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
# Mapping device status to icons


def get_instance_info_icon(status):
    """Return an instance info icon which reflects the given `DeviceStatus`
    """
    if not isinstance(status, InstanceStatus):
        status = InstanceStatus(status)

    if status is InstanceStatus.OK:
        return icons.statusOk
    elif status is InstanceStatus.ERROR:
        return icons.statusError
    elif status is InstanceStatus.UNKNOWN:
        return icons.statusUnknown
    # No icon!
    return None


def get_device_status_icon(status):
    """A `QIcon` for the given `status` is returned."""
    status_icons = {
        ProxyStatus.OFFLINE: icons.deviceOffline,
        ProxyStatus.ALIVE: icons.deviceAlive,
        ProxyStatus.REQUESTED: icons.device_requested,
        ProxyStatus.ONLINEREQUESTED: icons.device_requested,
        ProxyStatus.SCHEMA: icons.device_schema,
        ProxyStatus.NOSERVER: icons.deviceOfflineNoServer,
        ProxyStatus.NOPLUGIN: icons.deviceOfflineNoPlugin,
        ProxyStatus.INCOMPATIBLE: icons.deviceIncompatible,
        ProxyStatus.MISSING: icons.propertyMissing,
    }

    if not isinstance(status, ProxyStatus):
        status = ProxyStatus(status)

    return status_icons.get(status)


def get_device_status_pixmap(status, extent=16):
    """A `QPixmap` for the given `status` is returned.

    `extent` sets the size of the pixmap. The pixmap might be smaller than
    requested, but never larger.
    """
    icon = get_device_status_icon(status)
    if icon is not None:
        return icon.pixmap(extent)


def get_project_device_status_icon(status, error=False):
    """An icon for the given device `status`.

    NOTE: These icons are for the Project Panel
    """
    status_icons = {
        ProxyStatus.NOSERVER: icons.deviceOfflineNoServer,
        ProxyStatus.NOPLUGIN: icons.deviceOfflineNoPlugin,
        ProxyStatus.OFFLINE: icons.deviceOffline,
        ProxyStatus.REQUESTED: icons.deviceOffline,
        ProxyStatus.INCOMPATIBLE: icons.deviceIncompatible,
        ProxyStatus.MONITORING: icons.deviceMonitored,
    }

    if not isinstance(status, ProxyStatus):
        status = ProxyStatus(status)

    return status_icons.get(status, icons.deviceInstance)


def get_project_server_status_icon(status):
    """An icon for the given server `status`.

    NOTE: These icons are for the Project Panel
    """
    status_icons = {
        ProxyStatus.ONLINE: icons.yes,
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
UNKNOWN_COLOR_ALPHA = (255, 200, 150, 128)
PROPERTY_ALARM_COLOR = (255, 125, 125, 128)
PROPERTY_WARN_COLOR = (255, 255, 125, 128)
PROPERTY_INTERLOCK_COLOR = (51, 51, 255, 128)
LOCKED_COLOR = (255, 145, 255, 128)

# --------------------------------------------------------------------------
# Mapping alarms to colors

PROPERTY_ALARM_COLOR_MAP = {
    None: ALL_OK_COLOR,
    "none": None,
    "alarm": PROPERTY_ALARM_COLOR,
    "alarmLow": PROPERTY_ALARM_COLOR,
    "alarmHigh": PROPERTY_ALARM_COLOR,
    "alarmVarianceLow": PROPERTY_ALARM_COLOR,
    "alarmVarianceHigh": PROPERTY_ALARM_COLOR,
    "warn": PROPERTY_WARN_COLOR,
    "warnLow": PROPERTY_WARN_COLOR,
    "warnHigh": PROPERTY_WARN_COLOR,
    "warnVarianceLow": PROPERTY_WARN_COLOR,
    "warnVarianceHigh": PROPERTY_WARN_COLOR,
    "interlock": PROPERTY_INTERLOCK_COLOR}

WARN_GLOBAL = "warn"
WARN_LOW = "warnLow"
WARN_HIGH = "warnHigh"
WARN_VARIANCE_LOW = "warnVarianceLow"
WARN_VARIANCE_HIGH = "warnVarianceHigh"
ALARM_GLOBAL = "alarm"
ALARM_LOW = "alarmLow"
ALARM_HIGH = "alarmHigh"
ALARM_VARIANCE_LOW = "alarmVarianceLow"
ALARM_VARIANCE_HIGH = "alarmVarianceHigh"
INTERLOCK = "interlock"
ALARM_NONE = "none"

ICON_PATH = Path(icons.__file__).parent


def get_alarm_svg(alarm_type):
    """The svg icon for the given `alarm_type` is returned.
    """
    svg_none = str(ICON_PATH.joinpath("alarm_none.svg"))
    svg_warn = str(ICON_PATH.joinpath("warning.svg"))
    svg_alarm = str(ICON_PATH.joinpath("critical.svg"))
    svg_interlock = str(ICON_PATH.joinpath("interlock.svg"))

    ALARM_SVG = {
        ALARM_NONE: svg_none,
        WARN_GLOBAL: svg_warn,
        WARN_LOW: svg_warn,
        WARN_HIGH: svg_warn,
        WARN_VARIANCE_LOW: svg_warn,
        WARN_VARIANCE_HIGH: svg_warn,
        ALARM_GLOBAL: svg_alarm,
        ALARM_LOW: svg_alarm,
        ALARM_HIGH: svg_alarm,
        ALARM_VARIANCE_LOW: svg_alarm,
        ALARM_VARIANCE_HIGH: svg_alarm,
        INTERLOCK: svg_interlock,
    }

    alarm_svg = ALARM_SVG.get(alarm_type)
    if alarm_svg is not None:
        return alarm_svg


def get_user_session_button_data() -> (QIcon, str):
    """Provide the icon and tooltip for the temporary session button
    depending on the state of the temporary session."""
    if krb_access.SESSION_END_NOTICE:
        tooltip = "Need to reauthenticate."
        return icons.switchCritical, tooltip

    user = krb_access.TEMPORARY_SESSION_USER
    level = krb_access.GLOBAL_ACCESS_LEVEL.name
    if user is None:
        tooltip = "Start a temporary session"
        return icons.switchNormal, tooltip
    if krb_access.TEMPORARY_SESSION_WARNING:
        tooltip = (f"Logged in as '{user}' with access level '{level}'. "
                   "Temporary session is about to end.")
        return icons.switchWarning, tooltip
    tooltip = (f"Logged in as '{user}' with access level '{level}'. "
               "End the temporary session.")
    return icons.switchActive, tooltip
