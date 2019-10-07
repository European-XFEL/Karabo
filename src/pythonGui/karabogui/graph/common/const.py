import numpy as np

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
