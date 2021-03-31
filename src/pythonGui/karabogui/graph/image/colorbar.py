import numpy as np
from pyqtgraph import AxisItem, ColorMap, GraphicsWidget, ImageItem, ViewBox
from PyQt5.QtCore import pyqtSignal, pyqtSlot, QPoint, Qt
from PyQt5.QtGui import QTransform
from PyQt5.QtWidgets import QDialog, QGraphicsGridLayout, QMenu

from karabogui.fonts import get_qfont
from karabogui.graph.common.api import COLORMAPS
from karabogui.util import move_to_cursor

from .dialogs.levels import LevelsDialog
from .utils import levels_almost_equal

NUM_SAMPLES = 256


class ColorBarWidget(GraphicsWidget):
    levelsChanged = pyqtSignal(object)

    def __init__(self, imageItem, parent=None):
        super().__init__(parent=parent)
        self.imageItem = imageItem

        self.levels = min_level, max_level = [0, NUM_SAMPLES - 1]
        data = np.linspace(min_level, max_level, NUM_SAMPLES)[None, :]

        self.grid_layout = QGraphicsGridLayout(self)
        self.grid_layout.setSpacing(0)
        self.grid_layout.setContentsMargins(0, 40, 0, 0)

        self.vb = ColorViewBox(parent=self)
        self.vb.menu = self._create_menu()

        self.barItem = ImageItem(parent=self)
        self.barItem.setImage(data)
        self.vb.addItem(self.barItem)
        self.grid_layout.addItem(self.vb, 0, 0)
        self.vb.setYRange(*self.levels, padding=0)

        font = get_qfont()
        font.setPointSize(8)

        self.axisItem = AxisItem(orientation='right', parent=self)
        self.axisItem.setStyle(tickFont=font)
        self.axisItem.linkToView(self.vb)
        self.grid_layout.addItem(self.axisItem, 0, 1)

        self.setLayout(self.grid_layout)

    # ---------------------------------------------------------------------
    # PyQt slots

    @pyqtSlot()
    def _show_levels_dialog(self):
        image_range = self.imageItem.image.min(), self.imageItem.image.max()
        dialog = LevelsDialog(self.imageItem.levels,
                              image_range,
                              self.imageItem.auto_levels, self.parent())
        move_to_cursor(dialog)
        if dialog.exec_() == QDialog.Accepted:
            levels = dialog.levels
            self.set_levels(levels or image_range)

            # Request changing of levels
            self.levelsChanged.emit(levels)

    # ---------------------------------------------------------------------
    # Qt Events

    def mouseDoubleClickEvent(self, event):
        if event.button() == Qt.LeftButton:
            self._show_levels_dialog()
            event.accept()
            return

        super().mouseDoubleClickEvent(event)

    # ---------------------------------------------------------------------
    # Public methods

    def set_colormap(self, cmap):
        lut = (ColorMap(*zip(*COLORMAPS[cmap]), mode="RGB")
               .getLookupTable(alpha=False, mode="RGB"))
        self.barItem.setLookupTable(lut)

    def set_margins(self, top=None, bottom=None):
        """Sets top and bottom margins of the colorbar. This depend on the
        axis size of the main plot."""
        if top is None:
            _, top, _, _ = self.grid_layout.getContentsMargins()
        if bottom is None:
            _, _, _, bottom = self.grid_layout.getContentsMargins()

        self.grid_layout.setContentsMargins(0, top, 0, bottom)

    # ---------------------------------------------------------------------
    # Private methods

    def _create_menu(self):
        menu = QMenu()
        menu.addAction("Set levels...", self._show_levels_dialog)
        return menu

    def set_levels(self, image_range):
        # Check if levels and range are almost equal.
        # Only change y-range values if not.
        if levels_almost_equal(self.levels, image_range):
            return

        # Transform the bar item according to the range
        image_min, image_max = image_range
        scale = (image_max - image_min) / NUM_SAMPLES
        translate = image_min / scale
        transform = QTransform()
        transform.scale(1, scale)
        transform.translate(0, translate)
        self.barItem.setTransform(transform)
        self.vb.setYRange(image_min, image_max, padding=0)
        self.levels = image_range


class ColorViewBox(ViewBox):

    def __init__(self, parent=None):
        super(ColorViewBox, self).__init__(
            parent=parent, enableMenu=False, enableMouse=False)
        self.setFixedWidth(15)
        self.setLimits(xMin=0, xMax=1)
        self.enableAutoRange(enable=False)

        # Menu placeholder!
        self.menu = None

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def menuEnabled(self):
        return self.menu is not None

    def raiseContextMenu(self, event):
        if self.menu is None:
            return
        pos = event.screenPos()
        self.menu.popup(QPoint(pos.x(), pos.y()))
