#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 12, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import QSize
from PyQt5.QtWidgets import QStyledItemDelegate

from .utils import FIXED_ROW_HEIGHT


class ValueDelegate(QStyledItemDelegate):
    def sizeHint(self, option, index):
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)
