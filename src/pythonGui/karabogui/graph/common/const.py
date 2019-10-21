import numpy as np

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
RANGE_ACTIONS = ['ranges', 'axes']
ACTION_ITEMS = CHECK_ACTIONS + RANGE_ACTIONS

# Items required for the state trendline!
ALL_STATES = list(State.__members__.values())
STATE_INTEGER_MAP = {state.name: i for i, state in enumerate(ALL_STATES)}
INTEGER_STATE_MAP = {i: state.name for i, state in enumerate(ALL_STATES)}

# XXX: Allow UNKNOWN padding of a single item
MIN_STATE_INT = min(STATE_INTEGER_MAP.values()) - 1
MAX_STATE_INT = max(STATE_INTEGER_MAP.values()) + 1

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
Y_AXIS_WIDTH = 61
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
