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
from qtpy.QtCore import QObject, QPointF, QRectF, Qt, Slot
from qtpy.QtGui import QColor, QPen
from qtpy.QtWidgets import QGraphicsObject, QGraphicsRectItem

from karabogui.graph.common.api import CoordsLegend, float_to_string
from karabogui.graph.common.const import TF_SCALING, TF_TRANSLATION
from karabogui.graph.image.legends.picker import PickerLegend


class PickerController(QObject):

    def __init__(self, plotItem, parent=None):
        super().__init__(parent=parent)

        self.plotItem = plotItem
        self._selected_pixel = None

        # Indicator rectangle for hovered pixel.
        # Only appears when image is hovered
        self._indicator_rect = IndicatorRectangle()
        self._indicator_rect.setZValue(10)
        self.plotItem.vb.addItem(self._indicator_rect,
                                 ignoreBounds=True)

        # Selection rectange for clicked pixel. Appears when image is clicked.
        # Persistent when re-selecting the Picker tool button.
        self._selection_rect = IndicatorRectangle()
        self._selection_rect.setZValue(10)
        self.plotItem.vb.addItem(self._selection_rect,
                                 ignoreBounds=True)

        # Coords legend for hovered pixel coords. Located
        # at upper right corner.
        self._coords_legend = CoordsLegend()
        self._coords_legend.setParentItem(self.plotItem.vb)
        self._coords_legend.anchor(itemPos=(1, 0),
                                   parentPos=(1, 0),
                                   offset=(-5, 5))

        # Picker legend for selected pixel coordinates.
        # Located at lower right corner.
        self._picker_legend = PickerLegend()
        self._picker_legend.setParentItem(self.plotItem.vb)
        self._picker_legend.anchor(itemPos=(1, 1),
                                   parentPos=(1, 1),
                                   offset=(-5, -5))

        # Connect image plot and item signals
        imageItem = plotItem.imageItem
        imageItem.sigImageChanged.connect(self.update)
        imageItem.clicked.connect(self.select_pixel)
        imageItem.hovered.connect(self.hover_pixel)
        plotItem.imageTransformed.connect(self._update_geometry)

    def activate(self, active):
        """Called upon clicking Picker toolbutton"""
        self._indicator_rect.setVisible(False)
        self._coords_legend.setVisible(False)

        # If there is previously selected pixel, show selection rect and
        # picker legend.
        if self._selected_pixel is not None:
            self._selection_rect.setVisible(active)
            self._picker_legend.setVisible(active)
            if active:
                self._update_geometry()
                self.update()

    @Slot(float, float)
    def select_pixel(self, x, y):
        if not self._selection_rect.isVisible():
            self._selection_rect.show()
            self._picker_legend.show()

        self._selection_rect.setPos(x, y)
        self.update()

    @Slot()
    def update(self):
        if not self._selection_rect.isVisible():
            return

        if not self._picker_legend.isVisible():
            self._picker_legend.show()

        x, y = np.floor(self._selection_rect.absolute_position).astype(int)
        self._selected_pixel = (x, y)

        value = self.plotItem.image[y, x]
        color = self.plotItem.imageItem.get_color(x, y)
        pos = self._selection_rect.pos()

        self._picker_legend.set_value(pos.x(), pos.y(), value, color)

    @Slot(object, object)
    def hover_pixel(self, x, y):
        if x is None and y is None:
            if self._indicator_rect.isVisible():
                self._indicator_rect.hide()
                self._coords_legend.hide()
        else:
            if not self._indicator_rect.isVisible():
                self._indicator_rect.show()
                self._coords_legend.show()
            self._indicator_rect.setPos(x, y)

        if x is not None:
            x = float_to_string(x)
        if y is not None:
            y = float_to_string(y)
        self._coords_legend.set_value(x, y)

    def _update_geometry(self):
        """Adjusts picker items geometry with the current image transform"""
        self._indicator_rect.update_geometry_from_transform(
            self.plotItem.axes_transform[TF_SCALING],
            self.plotItem.axes_transform[TF_TRANSLATION])
        self._selection_rect.update_geometry_from_transform(
            self.plotItem.axes_transform[TF_SCALING],
            self.plotItem.axes_transform[TF_TRANSLATION])

    def destroy(self):
        imageItem = self.plotItem.imageItem

        imageItem.sigImageChanged.disconnect(self.update)
        imageItem.clicked.disconnect(self.select_pixel)
        imageItem.hovered.disconnect(self.hover_pixel)
        self.plotItem.imageTransformed.disconnect(self._update_geometry)

        for item in [self._indicator_rect, self._selection_rect,
                     self._coords_legend, self._picker_legend]:
            self.plotItem.vb.removeItem(item)

            # Only instances of QObject have deleteLater(). It is said that
            # when an QGraphicsItem went out of scope, it is automatically
            # deleted. But we can use sip.delete() just to make sure? We can
            # also use multiple inheritance such as:
            # IndicatorRectangle(QGraphicsRectItem, QObject)
            if isinstance(item, QGraphicsObject):
                item.deleteLater()

        self.deleteLater()


class IndicatorRectangle(QGraphicsRectItem):
    """A graphics item that is used to indicate pixel position when the image
       is hovered or clicked."""

    def __init__(self):
        QGraphicsRectItem.__init__(self, 0, 0, 1, 1)

        pen = QPen(QColor(Qt.white))
        pen.setCosmetic(True)
        self.setPen(pen)
        self.hide()

        self._scaling = np.array([1, 1])
        self._translation = np.array([0, 0])

    def hoverEnterEvent(self, event):
        self.savedPen = self.pen()
        self.setPen(QPen(QColor(0, 0, 0)))
        super().hoverEnterEvent(event)

    def hoverLeaveEvent(self, event):
        self.setPen(self.savedPen)
        event.ignore()
        super().hoverLeaveEvent(event)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            event.accept()
        else:
            event.ignore()
        super().mousePressEvent(event)

    def mouseMoveEvent(self, event):
        pass

    def update_geometry_from_transform(self, scaling, translation):
        x, y = (self.absolute_position * scaling) + translation
        w, h = self._absolute_size * scaling
        self._scaling = scaling
        self._translation = translation

        self.prepareGeometryChange()
        self.setRect(QRectF(0, 0, w, h))
        self.setPos(QPointF(x, y))

    @property
    def absolute_position(self):
        pos = self.pos()
        scaled = np.array([pos.x(), pos.y()]) - self._translation
        return scaled / self._scaling

    @property
    def _absolute_size(self):
        rect = self.rect()
        return np.array([rect.width(), rect.height()]) / self._scaling
