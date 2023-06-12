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
from qtpy.QtCore import Qt, QTimer, Slot
from qtpy.QtGui import QColor, QPainter, QPen, QPixmap
from qtpy.QtWidgets import QWidget

from karabogui.sceneview.const import GRID_STEP

GRID_COLOR = (200, 200, 200, 100)
RESIZE_THROTTLE = 100
PEN_SIZE = 1


class GridView(QWidget):
    """An object representing the grid view for a Karabo GUI scene.
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self._pen = QPen()
        self._pen.setWidth(PEN_SIZE)
        self._pen.setColor(QColor(*GRID_COLOR))

        # The update timer is required to throttle resize events!
        self._update_timer = QTimer(self)
        self._update_timer.setInterval(RESIZE_THROTTLE)
        self._update_timer.timeout.connect(self._needs_update)

        # By default the grid is not visible!
        self._visible = False
        self._pixmap = QPixmap()

    def _build_pixmap(self):
        """Build a pixmap drawing a grid layout"""
        if not self._visible:
            self._pixmap = QPixmap()
            return

        # Create the pixmap layer and fill transparent!
        pixmap = QPixmap(self.size())
        pixmap.fill(Qt.transparent)

        # Set our pen
        painter = QPainter(pixmap)
        painter.setPen(self._pen)
        # And draw the horizontal and vertical lines of the grid
        width = self.width()
        height = self.height()
        for y in range(0, height, GRID_STEP):
            painter.drawLine(0, y, width, y)
        for x in range(0, width, GRID_STEP):
            painter.drawLine(x, 0, x, height)

        self._pixmap = pixmap

    def resizeEvent(self, event):
        """Throttle the generation of the grid on resize events!"""
        self._update_timer.start()
        super().resizeEvent(event)

    @Slot()
    def _needs_update(self):
        self._update_timer.stop()
        self.showGrid(self._visible)

    def paintEvent(self, event):
        with QPainter(self) as painter:
            painter.drawPixmap(0, 0, self._pixmap)

    # Public interface
    # -----------------------------------------------------------------------

    def showGrid(self, value):
        """Public method to toggle and recompute the grid view"""
        self._visible = value
        self._build_pixmap()
        self.update()
