import numpy as np
from PyQt4.QtCore import pyqtSlot, QObject, QPointF, QRectF, Qt
from PyQt4.QtGui import (
    QColor, QGraphicsObject, QGraphicsRectItem, QPen)

from karabogui.graph.common.api import CoordsLegend
from karabogui.graph.common.const import SCALING, TRANSLATION
from karabogui.graph.image.legends.picker import PickerLegend


class PickerController(QObject):

    def __init__(self, plotItem):
        super(PickerController, self).__init__()

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
        plotItem.imageLevelsChanged.connect(self.update)

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

    @pyqtSlot(float, float)
    def select_pixel(self, x, y):
        if not self._selection_rect.isVisible():
            self._selection_rect.show()
            self._picker_legend.show()

        self._selection_rect.setPos(x, y)
        self.update()

    @pyqtSlot()
    def update(self):
        if not self._selection_rect.isVisible():
            return

        if not self._picker_legend.isVisible():
            self._picker_legend.show()

        image = self.plotItem.image
        image_y, image_x = image.shape

        x, y = np.floor(self._selection_rect.absolute_position).astype(int)
        self._selected_pixel = (x, y)
        value = image[y, x]

        # Get color of selected pixel
        qimage = self.plotItem.imageItem.get_qimage()

        # Check if qimage is downsampled
        qimage_size = qimage.size()
        qimage_x, qimage_y = qimage_size.width(), qimage_size.height()
        if image_x != qimage_x:
            x = np.ceil(x / image_x * qimage_x)
        if image_y != qimage_y:
            y = np.ceil(y / image_y * qimage_y)

        color = QColor(qimage.pixel(x, y))
        pos = self._selection_rect.pos()

        self._picker_legend.set_value(pos.x(), pos.y(), value, color)

    @pyqtSlot(object, object)
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
        self._coords_legend.set_value(x, y)

    def _update_geometry(self):
        """Adjusts picker items geometry with the current image transform"""
        self._indicator_rect.update_geometry_from_transform(
            self.plotItem.axes_transform[SCALING],
            self.plotItem.axes_transform[TRANSLATION])
        self._selection_rect.update_geometry_from_transform(
            self.plotItem.axes_transform[SCALING],
            self.plotItem.axes_transform[TRANSLATION])

    def destroy(self):
        imageItem = self.plotItem.imageItem

        imageItem.sigImageChanged.disconnect(self.update)
        imageItem.clicked.disconnect(self.select_pixel)
        imageItem.hovered.disconnect(self.hover_pixel)
        self.plotItem.imageTransformed.disconnect(self._update_geometry)
        self.plotItem.imageLevelsChanged.disconnect(self.update)

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
        super(IndicatorRectangle, self).hoverEnterEvent(event)

    def hoverLeaveEvent(self, event):
        self.setPen(self.savedPen)
        event.ignore()
        super(IndicatorRectangle, self).hoverLeaveEvent(event)

    def mousePressEvent(self, event):
        if event.button() == Qt.LeftButton:
            event.accept()
        else:
            event.ignore()
        super(IndicatorRectangle, self).mousePressEvent(event)

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
