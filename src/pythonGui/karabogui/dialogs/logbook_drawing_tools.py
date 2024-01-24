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
from qtpy.QtCore import QPointF, Qt, Slot
from qtpy.QtGui import QBrush, QColor, QFont, QIcon, QPen, QPixmap, QTransform
from qtpy.QtWidgets import (
    QColorDialog, QDialog, QGraphicsItem, QGraphicsPixmapItem)
from traits.api import ABCHasStrictTraits, Instance

from karabogui import icons
from karabogui.const import IS_MAC_SYSTEM
from karabogui.dialogs.font_dialog import FontDialog
from karabogui.dialogs.utils import get_dialog_ui
from karabogui.fonts import get_alias_from_font, get_qfont

BUTTON_SIZE = 10


class TextDialog(QDialog):
    """Simple Text dialog to allow user to enter the text and also to select
    the font and color for the text."""
    def __init__(self, font, color, parent=None):
        super().__init__(parent=parent)
        uic.loadUi(get_dialog_ui('simple_text_dialog.ui'), self)
        self.text_font = font
        self.text_color = color
        self.set_text_color_button()
        self.set_text_font_button()
        self.setModal(False)

    @property
    def text(self):
        return self.text_line_edit.text()

    @Slot()
    def on_pbFont_clicked(self):
        dialog = FontDialog(self.text_font, parent=self)
        if dialog.exec() == QDialog.Accepted:
            self.text_font = dialog.qfont
            self.set_text_font_button()

    @Slot()
    def on_pbTextColor_clicked(self):
        color = QColorDialog.getColor(initial=self.text_color, parent=self)
        if color.isValid():
            self.text_color = color
            self.set_text_color_button()

    def set_text_color_button(self):
        pixmap = QPixmap(24, 16)
        pixmap.fill(self.text_color)
        self.pbTextColor.setIcon(QIcon(pixmap))

    def set_text_font_button(self):
        """
        Update the  text and font family of the button. The font size
        should not be changed in order to keep the button size fixed.
        """
        qfont = QFont(self.text_font)
        qfont.setPointSize(BUTTON_SIZE)
        self.pbFont.setFont(qfont)
        family = get_alias_from_font(self.text_font.family())
        button_text = f"{family}, {self.text_font.pointSize()}pt"
        self.pbFont.setText(button_text)


class BaseDrawingTool(ABCHasStrictTraits):
    """Base class for the drawing tools in a QGraphicsScene"""

    # QGraphics Item to be drawn on the scene
    graphics_item = Instance(QGraphicsItem)

    # Starting point for the graphics item.
    start_pos = Instance(QPointF)

    # Annotation Color
    pen_color = Instance(QColor)

    @abstractmethod
    def mouse_down(self, scene, event):
        """Method to call on mouse press on the GraphicsScene"""

    @abstractmethod
    def mouse_move(self, scene, event):
        """Method to call on mouse move on the GraphicsScene"""

    @abstractmethod
    def mouse_up(self, scene, event):
        """Method to call on mouse move on the GraphicsScene."""

    def set_pen_color(self, color):
        self.pen_color = color


class LineTool(BaseDrawingTool):

    def mouse_down(self, scene, event):
        self.start_pos = event.scenePos()
        pen = QPen(QBrush(QColor(self.pen_color)), 2)
        self.graphics_item = scene.addLine(
            self.start_pos.x(), self.start_pos.y(),
            self.start_pos.x(), self.start_pos.y(), pen)

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
        pen = QPen(QBrush(QColor(self.pen_color)), 2)
        self.start_pos = pos
        self.graphics_item = scene.addRect(
            pos.x(), pos.y(), 0, 0, pen, QBrush())

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

    color = Instance(QColor)
    font = Instance(QFont)

    def mouse_down(self, scene, event):
        pass

    def mouse_move(self, scene, event):
        pass

    def mouse_up(self, scene, event):
        if self.font is None:
            self.font = get_qfont()
        if self.color is None:
            self.color = QColor("black")
        parent = scene.parent()
        text_dialog = TextDialog(
            font=self.font,  color=self.color, parent=parent)
        if text_dialog.exec() == QDialog.Accepted:
            text = text_dialog.text.strip()
            font = text_dialog.text_font
            color = text_dialog.text_color
            if text:
                self.graphics_item = scene.addSimpleText(text, font)
                brush = QBrush(color)
                self.graphics_item.setBrush(brush)
                self.graphics_item.setPos(event.scenePos())
            self.color = color
            self.font = font
        # To avoid LogBookPreview dialog hiding behind the main window
        if IS_MAC_SYSTEM:
            parent.raise_()


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


class CropTool(BaseDrawingTool):

    def mouse_down(self, scene, event):
        pos = event.scenePos()
        pen = QPen(QBrush(QColor(0, 0, 255, 127)), 2)
        self.start_pos = pos
        self.graphics_item = scene.addRect(
            pos.x(), pos.y(), 0, 0, pen, QBrush())

    def mouse_move(self, scene, event):
        pos = event.scenePos()
        if self.start_pos and self.graphics_item:
            width = pos.x() - self.start_pos.x()
            height = pos.y() - self.start_pos.y()
            self.graphics_item.setRect(self.start_pos.x(), self.start_pos.y(),
                                       width, height)
            self.graphics_item.setBrush(QColor(0, 0, 255, 127))

    def mouse_up(self, scene, event):
        scene.removeItem(self.graphics_item)
        rect = self.graphics_item.rect()
        scene.crop(rect)
        # Remove the remnants of annotations.
        for item in scene.items():
            if item and not isinstance(item, QGraphicsPixmapItem):
                scene.removeItem(item)


TOOLS_FACTORY = namedtuple("TOOL_FACTORY", ["tooltip", "icon", "drawing_tool"])


def get_tools():
    return [
        TOOLS_FACTORY(tooltip="Draw Line", icon=icons.line,
                      drawing_tool=LineTool),
        TOOLS_FACTORY(tooltip="Draw Rectangle", icon=icons.rect,
                      drawing_tool=RectTool),
        TOOLS_FACTORY(tooltip="Add Text to the Image", icon=icons.text,
                      drawing_tool=TextTool),
        TOOLS_FACTORY(tooltip="Delete item", icon=icons.eraser,
                      drawing_tool=EraserTool),
        TOOLS_FACTORY(tooltip="Crop image", icon=icons.crop,
                      drawing_tool=CropTool),
             ]
