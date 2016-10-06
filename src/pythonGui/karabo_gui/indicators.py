#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on October 6, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
"""This module contains constants for unified states and alarm indicators."""

from PyQt4.QtGui import QColor, QIcon, QPixmap

from karabo.common.states import State


# State coloring
UNKNOWN_COLOR = (255, 170, 0)
KNOWN_NORMAL_COLOR = (200, 200, 200)
INIT_COLOR = (255, 240, 180)
DISABLED_COLOR = (255, 170, 255)
ERROR_COLOR = (255, 0, 0)
CHANGING_DECREASING_INCREASING_COLOR = (0, 170, 255)
STATIC_COLOR = (170, 255, 127)
ACTIVE_COLOR = (0, 170, 0)
PASSIVE_COLOR = (240, 240, 240)


# Map states to colors
STATE_COLORS = {
    State.UNKNOWN: UNKNOWN_COLOR,
    State.KNOWN : KNOWN_NORMAL_COLOR,
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


# Map states to colored icons
STATE_ICONS = {
    k: QIcon(QPixmap(100, 100).fill(QColor(*v))) for k,v in STATE_COLORS.items()
    }


def get_state_icon(value):
    """ Return a colored icon for the given ``value`` which describes a States
    as a string. A ``NoneType`` is returned in case the given ``value`` state
    could not be found."""
    state = State(value)
    if state.isDerivedFrom(State.UNKNOWN):
        return STATE_ICONS[State.UNKNOWN]
    elif state.isDerivedFrom(State.KNOWN):
        return STATE_ICONS[State.KNOWN]
    elif state.isDerivedFrom(State.NORMAL):
        return STATE_ICONS[State.NORMAL]
    elif state.isDerivedFrom(State.INIT):
        return STATE_ICONS[State.INIT]
    elif state.isDerivedFrom(State.DISABLED):
        return STATE_ICONS[State.DISABLED]
    elif state.isDerivedFrom(State.ERROR):
        return STATE_ICONS[State.ERROR]
    elif state.isDerivedFrom(State.CHANGING):
        return STATE_ICONS[State.CHANGING]
    elif state.isDerivedFrom(State.DECREASING):
        return STATE_ICONS[State.DECREASING]
    elif state.isDerivedFrom(State.INCREASING):
        return STATE_ICONS[State.INCREASING]
    elif state.isDerivedFrom(State.STATIC):
        return STATE_ICONS[State.STATIC]
    elif state.isDerivedFrom(State.ACTIVE):
        return STATE_ICONS[State.ACTIVE]
    elif state.isDerivedFrom(State.PASSIVE):
        return STATE_ICONS[State.PASSIVE]
