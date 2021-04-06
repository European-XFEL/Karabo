#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 12, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import QSize
from qtpy.QtWidgets import QStyledItemDelegate

from .utils import FIXED_ROW_HEIGHT


class ValueDelegate(QStyledItemDelegate):
    def sizeHint(self, option, index):
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)
