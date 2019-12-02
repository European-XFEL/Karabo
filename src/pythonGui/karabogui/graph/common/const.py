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

# Plot Constants
# --------------------------------------------------------------------------

MIN_DOWNSAMPLE = 6000
MAX_DOWNSAMPLE = 30000

DEFAULT_BAR_WIDTH = 0.1
DEFAULT_PEN_WIDTH = 1.0

SYMBOL_SIZE = 7
DEFAULT_SYMBOL = 'o'
EMPTY_SYMBOL_OPTIONS = {'symbol': None, 'symbolSize': None,
                        'symbolPen': None, 'symbolBrush': None}
EMPTY_BAR = np.zeros(shape=(10,), dtype=np.int64)

CHECK_ACTIONS = ['x_grid', 'y_grid', 'x_log', 'y_log',
                 'x_invert', 'y_invert']
RANGE_ACTIONS = ['axes', 'x_range', 'y_range']
ACTION_ITEMS = CHECK_ACTIONS + RANGE_ACTIONS

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

MIN_STATE_INT = min(STATE_INTEGER_MAP.values())
MAX_STATE_INT = max(STATE_INTEGER_MAP.values())

ALARM_INTEGER_MAP = {alarm.value: i for i, alarm in
                     enumerate(AlarmCondition.__members__.values())}
INTEGER_ALARM_MAP = {i: alarm.value for i, alarm in
                     enumerate(AlarmCondition.__members__.values())}

MIN_ALARM_INT = min(ALARM_INTEGER_MAP.values())
MAX_ALARM_INT = max(ALARM_INTEGER_MAP.values())

# Image Constants
# --------------------------------------------------------------------------

LABEL = "label"
UNITS = "units"
PREFIX = "prefix"
VISIBLE = "visible"
SCALING = "scaling"
TRANSLATION = "translation"
IS_FLIPPED = "is_flipped"
IS_INVERTED = "is_inverted"
ROTATION = "rotation"

ROTATION_FACTOR = {
    0: (1, 1),
    90: (1, -1),
    180: (-1, -1),
    270: (-1, 1)
}

X_AXIS_HEIGHT = 46
Y_AXIS_WIDTH = 75
LABEL_HEIGHT = 20

# default values

VIRIDIS = "viridis"

DEFAULT_SCALE_X = 1
DEFAULT_SCALE_Y = 1
DEFAULT_OFFSET_X = 0
DEFAULT_OFFSET_Y = 0

DEFAULT_UNITS_X = "pixels"
DEFAULT_LABEL_X = "X-axis"
DEFAULT_UNITS_Y = "pixels"
DEFAULT_LABEL_Y = "Y-axis"
