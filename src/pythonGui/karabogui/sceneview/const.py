#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QBoxLayout

# A handy limit for things measured in pixels
SCREEN_MAX_VALUE = 100000

SCENE_BORDER_WIDTH = 10

GRID_STEP = 10

# Karabo colors
HOVER_COLOR = (75, 138, 164)
SELECTION_COLOR = (11, 18, 61)


# For convenience association of strings to Qt cursors
QT_CURSORS = {
    'arrow': Qt.ArrowCursor,
    'blank': Qt.BlankCursor,
    'busy': Qt.BusyCursor,
    'cross': Qt.CrossCursor,
    'closed-hand': Qt.ClosedHandCursor,
    'open-hand': Qt.OpenHandCursor,
    'pointing-hand': Qt.PointingHandCursor,
    'resize-diagonal-trbl': Qt.SizeBDiagCursor,
    'resize-diagonal-tlbr': Qt.SizeFDiagCursor,
    'resize-horizontal': Qt.SizeHorCursor,
    'resize-vertical': Qt.SizeVerCursor,
}

# For convenience association of strings to Qt enums
QT_PEN_CAP_STYLE_FROM_STR = {
    'butt': Qt.FlatCap,
    'square': Qt.SquareCap,
    'round': Qt.RoundCap,
}

# For convenience association reverse mapping of Qt enums to string
QT_PEN_CAP_STYLE_TO_STR = {v: k for k, v in QT_PEN_CAP_STYLE_FROM_STR.items()}

# For convenience association of strings to Qt enums
QT_PEN_JOIN_STYLE_FROM_STR = {
    'miter': Qt.MiterJoin,
    'svgmiter': Qt.SvgMiterJoin,
    'round': Qt.RoundJoin,
    'bevel': Qt.BevelJoin,
}

# For convenience association reverse mapping of Qt enums to string
QT_PEN_JOIN_STYLE_TO_STR = {v: k
                            for k, v in QT_PEN_JOIN_STYLE_FROM_STR.items()}

# For convenience association of ints to Qt enums
QT_BOX_LAYOUT_DIRECTION = (
    QBoxLayout.LeftToRight,
    QBoxLayout.RightToLeft,
    QBoxLayout.TopToBottom,
    QBoxLayout.BottomToTop
)
