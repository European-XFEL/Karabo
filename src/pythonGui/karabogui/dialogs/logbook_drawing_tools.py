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
# or FITNESS FOR A PARTICULAR PURPOSE.\
from abc import abstractmethod
from collections import namedtuple

from qtpy import uic
from qtpy.QtCore import QPointF, Qt
from qtpy.QtGui import QBrush, QColor, QPen, QTransform
from qtpy.QtWidgets import QDialog, QGraphicsItem, QGraphicsPixmapItem
from traits.api import ABCHasStrictTraits, Instance

from karabogui import icons
from karabogui.dialogs.utils import get_dialog_ui

PEN = QPen(QBrush(QColor("red")), 2)


class TextDialog(QDialog):
    """Simple Text dialog to allow user to enter a text and select a font"""
    def __init__(self, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui('simple_text_dialog.ui'), self)

    @property
    def text(self):
        return self.text_line_edit.text()


class BaseDrawingTool(ABCHasStrictTraits):
    """Base class for the drawing tools in a QGraphicsScene"""

    # QGraphics Item to be drawn on the scene
    graphics_item = Instance(QGraphicsItem)

    # Starting point for the graphics item.
    start_pos = Instance(QPointF)

    @abstractmethod
    def mouse_down(self, scene, event):
        """Method to call on mouse press on the GraphicsScene"""

    @abstractmethod
    def mouse_move(self, scene, event):
        """Method to call on mouse move on the GraphicsScene"""

    @abstractmethod
    def mouse_up(self, scene, event):
        """Method to call on mouse move on the GraphicsScene."""


class LineTool(BaseDrawingTool):

    def mouse_down(self, scene, event):
        self.start_pos = event.scenePos()
        self.graphics_item = scene.addLine(
            self.start_pos.x(), self.start_pos.y(),
            self.start_pos.x(), self.start_pos.y(), PEN)

    def mouse_move(self, scene, event):
        pos = event.scenePos()
        if self.start_pos and self.graphics_item:
            self.graphics_item.setLine(
                self.start_pos.x(), self.start_pos.y(), pos.x(), pos.y())

    def mouse_up(self, scene, event):
        pass


class RectTool(BaseDrawingTool):

    def mouse_down(self, scene, event):
        pos = event.scenePos()
        self.start_pos = pos
        self.graphics_item = scene.addRect(
            pos.x(), pos.y(), 0, 0, PEN, QBrush())

    def mouse_move(self, scene, event):
        pos = event.scenePos()
        if self.start_pos and self.graphics_item:
            width = pos.x() - self.start_pos.x()
            height = pos.y() - self.start_pos.y()
            self.graphics_item.setRect(self.start_pos.x(), self.start_pos.y(),
                                       width, height)

    def mouse_up(self, scene, event):
        pass


class TextTool(BaseDrawingTool):
    """Allow to open a dialog to define the text to the QGraphicsScene"""

    def mouse_down(self, scene, event):
        pass

    def mouse_move(self, scene, event):
        pass

    def mouse_up(self, scene, event):
        text_dialog = TextDialog(parent=scene.parent())
        text_dialog.setModal(False)
        if text_dialog.exec() == QDialog.Accepted:
            text = text_dialog.text.strip()
            if text:
                self.graphics_item = scene.addText(text)
                self.graphics_item.setPos(event.scenePos())


class EraserTool(BaseDrawingTool):

    def mouse_down(self, scene, event):
        if event.button() == Qt.LeftButton:
            item = scene.itemAt(event.scenePos(), QTransform())
            if item and not isinstance(item, QGraphicsPixmapItem):
                scene.removeItem(item)

    def mouse_move(self, scene, event):
        pass

    def mouse_up(self, scene, event):
        pass


TOOLS_FACTORY = namedtuple("TOOL_FACTORY", ["name", "icon", "drawing_tool"])


def get_tools():
    return [
        TOOLS_FACTORY(name="Line", icon=icons.line, drawing_tool=LineTool),
        TOOLS_FACTORY(name="Rect", icon=icons.rect, drawing_tool=RectTool),
        TOOLS_FACTORY(name="Text", icon=icons.text, drawing_tool=TextTool),
        TOOLS_FACTORY(
            name="Eraser", icon=icons.eraser, drawing_tool=EraserTool)
             ]
