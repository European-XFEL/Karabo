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
import numpy as np

from karabo.common.alarm_conditions import AlarmCondition
from karabo.common.states import State

# Toolbar
# --------------------------------------------------------------------------

BUTTON_SIZE = 32
ICON_SIZE = 20

# Global Constants
# --------------------------------------------------------------------------

AXIS_ITEMS = ["top", "bottom", "left", "right"]
AXIS_X = ["top", "bottom"]
AXIS_Y = ["left", "right"]

WIDGET_MIN_HEIGHT = 200  # pixels
WIDGET_MIN_WIDTH = 300

WIDGET_HEIGHT_HINT = 250  # pixels
WIDGET_WIDTH_HINT = 450

# Plot Constants
# --------------------------------------------------------------------------

DEFAULT_BAR_WIDTH = 0.1
DEFAULT_PEN_WIDTH = 1.0

SYMBOL_SIZE = 5
DEFAULT_SYMBOL = 'o'
EMPTY_SYMBOL_OPTIONS = {'symbol': None, 'symbolSize': None,
                        'symbolPen': None, 'symbolBrush': None}
EMPTY_BAR = np.zeros(shape=(10,), dtype=np.int64)

CHECK_ACTIONS = ['x_grid', 'y_grid', 'x_log', 'y_log',
                 'x_invert', 'y_invert']
RANGE_ACTIONS = ['axes', 'x_range', 'y_range']
VIEW_ACTIONS = ['view']
ACTION_ITEMS = CHECK_ACTIONS + RANGE_ACTIONS + VIEW_ACTIONS

# Items required for the state trendline!
ALL_STATES = [
    State.NORMAL,
    State.UNKNOWN,
    State.INIT,
    State.ERROR,
    State.INTERLOCKED,
    State.DISABLED,
    State.CHANGING,
    State.MOVING,
    State.OFF,
    State.ON,
    State.STOPPED,
    State.RUNNING,
    State.ACQUIRING,
    State.PROCESSING,
    State.HOMING,
    State.ACTIVE,
    State.PASSIVE,
    State.OPENED,
    State.CLOSED,
    State.PAUSED,
    State.OPENING,
    State.CLOSING,
    State.STARTED,
    State.INSERTED,
    State.MONITORING,
    State.IGNORING,
    State.INSERTING,
    State.STARTING,
    State.STOPPING,
    State.SEARCHING,
    State.STATIC,
    State.RAMPING_DOWN,
    State.RAMPING_UP,
    State.EXTRACTING,
    State.EXTRACTED,
    State.COOLING,
    State.COOLED,
    State.COLD,
    State.HEATING,
    State.HEATED,
    State.WARM,
    State.INTERLOCK_OK,
    State.INTERLOCK_BROKEN,
    State.PRESSURIZED,
    State.EVACUATED,
    State.EMPTYING,
    State.FILLING,
    State.DISENGAGING,
    State.DISENGAGED,
    State.ENGAGING,
    State.ENGAGED,
    State.SWITCHING_OFF,
    State.SWITCHING_ON,
    State.SWITCHING,
    State.ROTATING,
    State.ROTATING_CNTCLK,
    State.ROTATING_CLK,
    State.MOVING_LEFT,
    State.MOVING_DOWN,
    State.MOVING_BACK,
    State.MOVING_RIGHT,
    State.MOVING_UP,
    State.MOVING_FORWARD,
    State.UNLOCKED,
    State.LOCKED,
    State.INCREASING,
    State.DECREASING,
    State.KNOWN,
]

STATE_INTEGER_MAP = {state.value: i for i, state in enumerate(ALL_STATES)}
INTEGER_STATE_MAP = {i: state.value for i, state in enumerate(ALL_STATES)}

ALARM_INTEGER_MAP = {alarm.value: i for i, alarm in
                     enumerate(AlarmCondition.__members__.values())}
INTEGER_ALARM_MAP = {i: alarm.value for i, alarm in
                     enumerate(AlarmCondition.__members__.values())}


def get_alarm_string(value):
    """Get an alarm condition from a mapped `index`"""
    return INTEGER_ALARM_MAP.get(round(value), "")


def get_state_string(value):
    """Get an state value from a mapped `index`"""
    return INTEGER_STATE_MAP.get(round(value), "")


# Image Constants
# --------------------------------------------------------------------------

TF_SCALING = "scaling"
TF_TRANSLATION = "translation"
TF_FLIPPED = "flipped"
TF_ROTATION = "rotation"

ROTATION_FACTOR = {
    0: (1, 1),
    90: (1, -1),
    180: (-1, -1),
    270: (-1, 1)
}

DEFAULT_SCALE_X = 1.0
DEFAULT_SCALE_Y = 1.0
DEFAULT_OFFSET_X = 0.0
DEFAULT_OFFSET_Y = 0.0

X_AXIS_HEIGHT = 46
Y_AXIS_WIDTH = 75

DEFAULT_UNITS_X = "pixels"
DEFAULT_LABEL_X = "X-axis"
DEFAULT_UNITS_Y = "pixels"
DEFAULT_LABEL_Y = "Y-axis"
