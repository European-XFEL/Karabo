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
from qtpy.QtCore import Qt, Slot
from qtpy.QtGui import QPalette
from qtpy.QtWidgets import QLineEdit


class LineEditEditor(QLineEdit):
    """A colored `LineEdit` depending on acceptable input

    Note: This widget is included as promoted widgets in *.ui files.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self._normal_palette = self.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        self.textChanged.connect(self._check_background)

    @Slot(str)
    def _check_background(self, text):
        acceptable_input = self.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self.setPalette(palette)
