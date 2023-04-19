#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 12, 2017
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from qtpy.QtCore import QSize
from qtpy.QtWidgets import QStyledItemDelegate

from .utils import FIXED_ROW_HEIGHT


class ValueDelegate(QStyledItemDelegate):
    """A delegate for graceful size calculation for the value column

    This value delegate will prevent flickering of the column size
    """
    def sizeHint(self, option, index):
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)
