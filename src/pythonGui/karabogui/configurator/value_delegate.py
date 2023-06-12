#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 12, 2017
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
