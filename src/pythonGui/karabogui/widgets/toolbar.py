#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on January 16, 2014
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
from qtpy.QtWidgets import QSizePolicy, QToolBar, QWidget


class ToolBar(QToolBar):
    def __init__(self, title="", parent=None):
        super().__init__(title, parent=parent)

        iconSize = QSize(32, 32)
        iconSize *= 0.6
        self.setIconSize(iconSize)
        self.setMovable(False)

    def add_expander(self):
        """Add an empty widget to the toolbar which absorbs horizontal space.
        """
        expander = QWidget()
        expander.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Fixed)
        self.addWidget(expander)
