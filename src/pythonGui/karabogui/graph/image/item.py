import numpy as np
from PyQt4.QtCore import pyqtSignal, Qt, QPointF
from PyQt4.QtGui import QTransform
from pyqtgraph import ImageItem

from karabogui.graph.common.enums import MouseMode
from .utils import map_rect_to_transform


class KaraboImageItem(ImageItem):
    clicked = pyqtSignal(float, float)
    hovered = pyqtSignal(object, object)

    def __init__(self, image=np.zeros((50, 50), dtype=int)):
        super(KaraboImageItem, self).__init__(image)

        self._origin = np.array([0, 0])
        self.auto_levels = True
        self._rect = None

    # ---------------------------------------------------------------------
    # Public methods

    def get_qimage(self):
        if self.qimage is None:
            self.render()
        return self.qimage

    def set_transform(self, scaling=(1, 1), translation=(0, 0), rotation=0):
        transform = QTransform()
        transform.rotate(rotation)
        transform.scale(*scaling)
        transform.translate(*translation)
        self.setTransform(transform)

        # Calculate new image item geometry
        self._origin = np.multiply(translation, scaling)
        self._rect = map_rect_to_transform(self.boundingRect(),
                                           scaling, self._origin)

    def rect(self):
        if self._rect is None:
            return self.boundingRect()
        return self._rect

    # ---------------------------------------------------------------------
    # Events

    def mouseClickEvent(self, event):
        if (self._viewBox().mouse_mode is MouseMode.Picker
                and event.button() == Qt.LeftButton):
            image_pos, view_pos = self._get_mouse_positions(event.pos())
            self.clicked.emit(image_pos.x(), image_pos.y())
        super(KaraboImageItem, self).mouseClickEvent(event)

    def hoverEvent(self, event):
        if self._viewBox().mouse_mode is MouseMode.Picker:
            self._hover_pointer_mode(event)
        super(KaraboImageItem, self).hoverEvent(event)

    # ---------------------------------------------------------------------
    # Private methods

    def _hover_pointer_mode(self, event):
        if event.isExit():
            self.hovered.emit(None, None)
            return

        event.acceptDrags(Qt.LeftButton)
        event.acceptClicks(Qt.LeftButton)

        (image_pos, view_pos) = self._get_mouse_positions(event.pos())
        self.hovered.emit(image_pos.x(), image_pos.y())

    def _get_mouse_positions(self, pos):
        image_pos = self.mapToParent(*np.floor(pos))
        view_pos = image_pos - QPointF(*self._origin)
        return image_pos, view_pos
