#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on June 7, 2016
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QBoxLayout

# For convenience association of strings to Qt enums
QT_PEN_CAP_STYLE = {
    'butt': Qt.FlatCap,
    'square': Qt.SquareCap,
    'round': Qt.RoundCap,
}

# For convenience association of strings to Qt enums
QT_PEN_JOIN_STYLE = {
    'miter': Qt.SvgMiterJoin,
    'round': Qt.RoundJoin,
    'bevel': Qt.BevelJoin,
}

# For convenience association of ints to Qt enums
QT_BOX_LAYOUT_DIRECTION = {
    0: QBoxLayout.LeftToRight,
    1: QBoxLayout.RightToLeft,
    2: QBoxLayout.TopToBottom,
    3: QBoxLayout.BottomToTop,
}
