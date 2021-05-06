import numpy as np
from pyqtgraph import GraphicsWidget, ViewBox
from qtpy.QtCore import QPoint, Qt, Signal
from qtpy.QtWidgets import QMenu

from .enums import MouseMode

ZOOM_IN = 1
ZOOM_OUT = -1


class KaraboViewBox(ViewBox):
    middleButtonClicked = Signal()

    def __init__(self, parent=None):
        super(KaraboViewBox, self).__init__(parent, enableMenu=False)
        self.mouse_mode = MouseMode.Pointer
        self.setBackgroundColor('w')

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

        if self.mouse_mode is MouseMode.Zoom:
            if event.button() == Qt.LeftButton:
                self._click_zoom_mode(event, ZOOM_IN)
            elif event.button() == Qt.RightButton:
                self._click_zoom_mode(event, ZOOM_OUT)
        else:
            super(KaraboViewBox, self).mouseClickEvent(event)

    def mouseDragEvent(self, event, axis=None):
        button = event.buttons()

        # Restore original MouseMode (and cursor) when dragEvent is finished
        if event.isFinish():
            super(KaraboViewBox, self).mouseDragEvent(event, axis)
            self.set_mouse_mode(self.mouse_mode)
            return

        # Check on how the drag will commence
        if self.mouse_mode is MouseMode.Pointer and button == Qt.LeftButton:
            event.ignore()
            return
        elif self.mouse_mode is MouseMode.Move or button == Qt.MiddleButton:
            if event.isStart():
                # Enable panning regardless of MouseMode
                self.state['mouseMode'] = ViewBox.PanMode
                self.setCursor(Qt.ClosedHandCursor)
        elif button == Qt.RightButton:
            if event.isStart():
                # Change cursor on right button as well
                self.setCursor(Qt.ClosedHandCursor)

        super(KaraboViewBox, self).mouseDragEvent(event, axis)

    def wheelEvent(self, event, axis=None):
        """Ignore mouse scroll since it also catches scene scroll"""
        event.ignore()

    # ---------------------------------------------------------------------
    # Public methods

    def set_mouse_mode(self, mode):
        if mode is MouseMode.Pointer:
            vb_mode = ViewBox.PanMode
            cursor = Qt.ArrowCursor
        elif mode is MouseMode.Zoom:
            vb_mode = ViewBox.RectMode
            cursor = Qt.CrossCursor
        elif mode is MouseMode.Move:
            vb_mode = ViewBox.PanMode
            cursor = Qt.OpenHandCursor
        else:
            raise LookupError("Invalid mouse mode.")

        # Using self.state to assign vb mouse mode to avoid emitting
        # sigStateChanged
        self.state['mouseMode'] = vb_mode
        self.setCursor(cursor)
        self.mouse_mode = mode

    def add_action(self, action, separator=True):
        if self.menu is None:
            return

        if separator:
            self.menu.addSeparator()
        self.menu.addAction(action)

    # ---------------------------------------------------------------------
    # Reimplemented `PyQtGraph` methods

    def removeItem(self, item):
        if item in self.allChildItems():
            super(KaraboViewBox, self).removeItem(item)

    def menuEnabled(self):
        return self.menu is not None

    def raiseContextMenu(self, event):
        if self.menu is None:
            return
        pos = event.screenPos()
        self.menu.popup(QPoint(pos.x(), pos.y()))

    # ---------------------------------------------------------------------
    # private methods

    def _click_zoom_mode(self, event, direction):
        # Lifted from the PyQtGraph wheelEvent(). wheelScaleFactor can be
        # increased for larger scale/zoom
        mask = np.array([1, 1])
        s = ((mask * 0.02) + 1) ** \
            (120 * direction * self.state['wheelScaleFactor'])

        center = self.mapToView(event.pos())
        self._resetTarget()
        self.scaleBy(s, center)
        self.sigRangeChangedManually.emit(self.state['mouseEnabled'])
        event.accept()

    # ------------ Patch PyQtGraph > 0.11.0 ----------------

    # The following lines revert changes done in PR!1435
    # https://github.com/pyqtgraph/pyqtgraph/pull/1435/
    # These changes lead to a peformance decrease of a factor of two (2)
    # which are reverted here, until pyqtgraph comes up with a different
    # fix.
    # Issue - https://github.com/pyqtgraph/pyqtgraph/issues/1602
    # Failed fix: https://github.com/pyqtgraph/pyqtgraph/pull/1610

    def paint(self, p, opt, widget):
        if self.border is not None:
            bounds = self.shape()
            p.setPen(self.border)
            p.drawPath(bounds)

    def update(self, *args, **kwargs):
        self.prepareForPaint()
        GraphicsWidget.update(self, *args, **kwargs)
