from PyQt5.QtCore import pyqtSlot, Qt, QTimer
from PyQt5.QtGui import QPainter, QPen, QColor, QPixmap
from PyQt5.QtWidgets import QWidget

GRID_STEP = 10
GRID_COLOR = (200, 200, 200, 160)
RESIZE_THROTTLE = 100
PEN_SIZE = 0.5


class GridView(QWidget):
    """An object representing the grid view for a Karabo GUI scene.
    """

    def __init__(self, parent=None):
        super(GridView, self).__init__(parent)
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
        super(GridView, self).resizeEvent(event)

    @pyqtSlot()
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
