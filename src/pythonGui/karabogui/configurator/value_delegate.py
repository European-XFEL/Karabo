#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 12, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import QSize
from PyQt4.QtGui import QStyledItemDelegate

# XXX: For now....
# from .utils import FIXED_ROW_HEIGHT
FIXED_ROW_HEIGHT = 30


class ValueDelegate(QStyledItemDelegate):
    def sizeHint(self, option, index):
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)
