from PyQt4.QtCore import pyqtSignal, QPointF, QRectF, QSizeF, Qt
from PyQt4.QtGui import QColor, QGraphicsObject, QPen


class BaseCanvas(QGraphicsObject):
    """A transparent graphics object for painting items. It is
    instantiated by supplying its geometry (rect) and should be then added to
    the scene (by some addItem()). The paint and mouse interactions should be
    reimplemented. Once the user has finished drawing the
    item using the mouse interactions, the canvas supplies the
    drawn geometry using editingFinished.

    One has to redefine the values:
        self._drawn_geometry -  the emitted geometry (e.g. QRect() for rects)
        self._default_geometry - the emitted geometry when the user cancels
            the operations. (e.g. empty QRect())

    """

    editingFinished = pyqtSignal(object)

    def __init__(self, rect):
        super(BaseCanvas, self).__init__()
        self._drawn_geometry = None
        self._default_geometry = QPointF()
        self._bounding_rect = rect
        self._pen = QPen(QColor(255, 255, 255))
        self.setZValue(1000)
        self.setCursor(Qt.CrossCursor)

    def paint(self, painter, *args):
        pass

    def mousePressEvent(self, event):
        """When canvas is pressed by left button, the subclass should do
           desired bookeeping. Base class will only repaint.
           When right button is pressed, finished editing by emitting
           default geometry."""
        if event.button() == Qt.LeftButton:
            self.update()
            event.accept()

    def mouseReleaseEvent(self, event):
        """When left button is released, it usually signals the end of the
           dragging motion. The corresponding geometry will be then emitted.
        """
        if event.button() == Qt.LeftButton:
            self.editingFinished.emit(self._drawn_geometry)
            event.accept()
        elif event.button() == Qt.RightButton:
            self.editingFinished.emit(self._default_geometry)
            event.ignore()

    def destroy(self):
        self.editingFinished.disconnect()
        self.unsetCursor()
        self.deleteLater()

    def boundingRect(self):
        return self._bounding_rect


class RectCanvas(BaseCanvas):
    """Canvas for drawing rectangles."""

    def __init__(self, rect):
        super(RectCanvas, self).__init__(rect)
        self._drawn_geometry = QRectF(0, 0, 0, 0)
        self._default_geometry = QRectF()
        self._start_pos = None

    def paint(self, painter, *args):
        """Reimplemented because we want to draw rectangles"""
        painter.setPen(self._pen)
        painter.drawRect(self._drawn_geometry)

    def mousePressEvent(self, event):
        """Reimplemented because we want to record starting position"""
        if event.button() == Qt.LeftButton:
            self._start_pos = event.pos()
            rect = QRectF(self._start_pos, self._start_pos).normalized()
            self._drawn_geometry = rect
        super(RectCanvas, self).mousePressEvent(event)

    def mouseMoveEvent(self, event):
        """Reimplemented because we want to record geometry and draw
           rectangles when dragging"""
        if event.buttons() & Qt.LeftButton:
            pos = event.pos()
            if self.contains(pos):
                rect = QRectF(self._start_pos, pos).normalized()
                self._drawn_geometry = rect
                self.update()
            event.accept()


class PointCanvas(BaseCanvas):

    def __init__(self, rect):
        super(PointCanvas, self).__init__(rect)
        self._drawn_geometry = QRectF(0, 0, 0, 0)
        self._default_geometry = QRectF()

    def mousePressEvent(self, event):
        """Reimplemented because we only care about mouse release"""
        if event.button() == Qt.LeftButton:
            event.accept()
            return
        super(PointCanvas, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        """Reimplemented because we want to record the self._drawn_geometry
           first before emitting it in base class."""
        self._drawn_geometry = QRectF(event.pos(), QSizeF(1, 1))
        super(PointCanvas, self).mouseReleaseEvent(event)
