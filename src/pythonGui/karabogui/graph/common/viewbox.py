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
import numpy as np
from pyqtgraph import ViewBox
from qtpy.QtCore import QPoint, Qt, Signal
from qtpy.QtWidgets import QMenu

from .enums import MouseTool

ZOOM_IN = 1
ZOOM_OUT = -1


class KaraboViewBox(ViewBox):
    middleButtonClicked = Signal()

    def __init__(self, parent=None):
        super().__init__(parent, enableMenu=False)
        self.mouse_tool = MouseTool.Pointer
        self.setBackgroundColor("w")

        # Build a menu!
        self.menu = QMenu(parent)
        self.menu.addAction("View all", self.enableAutoRange)
        self.autorange_enabled = True

    # ---------------------------------------------------------------------
    # mouse events

    def mouseClickEvent(self, event):
        if event.button() == Qt.MiddleButton:
            if self.autorange_enabled:
                self.enableAutoRange()
            self.middleButtonClicked.emit()

        if self.mouse_tool is MouseTool.Zoom:
            if event.button() == Qt.LeftButton:
                self._click_zoom_mode(event, ZOOM_IN)
            elif event.button() == Qt.RightButton:
                self._click_zoom_mode(event, ZOOM_OUT)
        else:
            super().mouseClickEvent(event)

    def mouseDragEvent(self, event, axis=None):
        button = event.buttons()

        # Restore original MouseMode (and cursor) when dragEvent is finished
        if event.isFinish():
            super().mouseDragEvent(event, axis)
            self.set_mouse_tool(self.mouse_tool)
            return

        # Check on how the drag will commence
        if self.mouse_tool is MouseTool.Pointer and button == Qt.LeftButton:
            event.ignore()
            return
        elif self.mouse_tool is MouseTool.Move or button == Qt.MiddleButton:
            if event.isStart():
                # Enable panning regardless of MouseTool
                self.state["mouseMode"] = ViewBox.PanMode
                self.setCursor(Qt.ClosedHandCursor)
        elif button == Qt.RightButton:
            if event.isStart():
                # Change cursor on right button as well
                self.setCursor(Qt.ClosedHandCursor)

        super().mouseDragEvent(event, axis)

    def wheelEvent(self, event, axis=None):
        """Ignore mouse scroll since it also catches scene scroll"""
        event.ignore()

    # ---------------------------------------------------------------------
    # Public methods

    def set_mouse_tool(self, mode):
        if mode is MouseTool.Pointer:
            mouseMode = ViewBox.PanMode
            cursor = Qt.ArrowCursor
        elif mode is MouseTool.Zoom:
            mouseMode = ViewBox.RectMode
            cursor = Qt.CrossCursor
        elif mode is MouseTool.Move:
            mouseMode = ViewBox.PanMode
            cursor = Qt.OpenHandCursor
        else:
            raise ValueError(f"Invalid mouseTool: {mode}")

        # Using self.state to assign vb mouse mode to avoid emitting
        # sigStateChanged
        self.state["mouseMode"] = mouseMode
        self.setCursor(cursor)
        self.mouse_tool = mode

    def add_action(self, action, separator=True):
        if self.menu is None:
            return

        if separator:
            self.menu.addSeparator()
        self.menu.addAction(action)

    # ---------------------------------------------------------------------
    # Backward compatibility

    set_mouse_mode = set_mouse_tool

    @property
    def mouse_mode(self):
        return self.mouse_tool

    @mouse_mode.setter
    def mouse_mode(self, value):
        self.mouse_tool = value

    # ---------------------------------------------------------------------
    # Reimplemented `PyQtGraph` methods

    def removeItem(self, item):
        if item in self.allChildItems():
            super().removeItem(item)

    def menuEnabled(self):
        return self.menu is not None

    def raiseContextMenu(self, event):
        if self.menu is None:
            return
        pos = event.screenPos()
        self.menu.popup(QPoint(int(pos.x()), int(pos.y())))

    # ---------------------------------------------------------------------
    # private methods

    def _click_zoom_mode(self, event, direction):
        # Lifted from the PyQtGraph wheelEvent(). wheelScaleFactor can be
        # increased for larger scale/zoom

        scene_rect = self.sceneBoundingRect()
        pos = event.pos()
        if not scene_rect.contains(pos):
            event.ignore()
            return

        mask = np.array([1, 1])
        s = ((mask * 0.02) + 1) ** \
            (120 * direction * self.state["wheelScaleFactor"])

        center = self.mapToView(pos)
        self._resetTarget()
        self.scaleBy(s, center)
        self.sigRangeChangedManually.emit(self.state["mouseEnabled"])
        event.accept()
