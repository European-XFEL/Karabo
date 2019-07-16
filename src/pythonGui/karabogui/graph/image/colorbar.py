import numpy as np
from pyqtgraph import AxisItem, ColorMap, GraphicsWidget, ImageItem, ViewBox
from PyQt4.QtCore import pyqtSignal, pyqtSlot, QPoint
from PyQt4.QtGui import QDialog, QFont, QGraphicsGridLayout, QMenu, QTransform

from karabogui.graph.common.api import COLORMAPS

from .dialogs.levels import LevelsDialog
from .utils import levels_almost_equal

NUM_SAMPLES = 256


class ColorBarWidget(GraphicsWidget):
    levelsChanged = pyqtSignal(object)

    def __init__(self, imageItem, label=None, parent=None):
        super(ColorBarWidget, self).__init__(parent)

        self._colorbar_levels = min_level, max_level = [0, NUM_SAMPLES - 1]
        data = np.linspace(min_level, max_level, NUM_SAMPLES)[None, :]

        self.grid_layout = QGraphicsGridLayout()
        self.grid_layout.setHorizontalSpacing(0)
        self.grid_layout.setVerticalSpacing(0)
        self.grid_layout.setContentsMargins(0, 40, 0, 0)

        self.vb = ColorViewBox()
        self.vb.menu = self._create_menu()

        self.barItem = ImageItem()
        self.barItem.setImage(data)
        self.vb.addItem(self.barItem)
        self.grid_layout.addItem(self.vb, 0, 0)
        self.vb.setYRange(*self._colorbar_levels, padding=0)

        font = QFont()
        font.setPixelSize(8)

        self.axisItem = AxisItem(orientation='right')
        self.axisItem.tickFont = font
        self.axisItem.linkToView(self.vb)
        self.grid_layout.addItem(self.axisItem, 0, 1)

        self.setLayout(self.grid_layout)

        self.imageItem = imageItem
        self.imageItem.sigImageChanged.connect(self._image_changed)
        if label is not None:
            self.set_label(label)

    # ---------------------------------------------------------------------
    # PyQt slots

    @pyqtSlot()
    def _image_changed(self):
        self._set_colorbar_range(self.imageItem.levels)

    @pyqtSlot()
    def _show_levels_dialog(self):
        image_range = self.imageItem.image.min(), self.imageItem.image.max()
        dialog = LevelsDialog(self.imageItem.levels,
                              image_range,
                              self.imageItem.auto_levels,
                              self.parent())

        if dialog.exec_() == QDialog.Accepted:
            levels = dialog.levels

            # Request changing of levels
            self.levelsChanged.emit(levels)

            # If autolevel, use recorded level
            if levels is None:
                levels = image_range

            self._set_colorbar_range(levels)

    # ---------------------------------------------------------------------
    # Public methods

    def set_label(self, label):
        self.axisItem.setLabel(label)

    def set_colormap(self, cmap):
        lut = (ColorMap(*zip(*COLORMAPS[cmap]), mode="RGB")
               .getLookupTable(alpha=False, mode="RGB"))
        self.barItem.setLookupTable(lut)

    def set_margins(self, top=0, bottom=0):
        """Sets top and bottom margins of the colorbar. This depend on the
        axis size of the main plot."""
        self.grid_layout.setContentsMargins(0, top, 0, bottom)

    # ---------------------------------------------------------------------
    # Private methods

    def _create_menu(self):
        menu = QMenu()
        menu.addAction("Set levels...", self._show_levels_dialog)
        return menu

    def _set_colorbar_range(self, image_range):
        # Check if levels and range are almost equal.
        # Only change y-range values if not.
        if levels_almost_equal(self._colorbar_levels, image_range):
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
        self._colorbar_levels = image_range


class ColorViewBox(ViewBox):

    def __init__(self):
        super(ColorViewBox, self).__init__()
        self.setFixedWidth(15)
        self.setLimits(xMin=0, xMax=1)
        self.enableAutoRange(enable=False)
        self.setMouseEnabled(x=False, y=False)

    # ---------------------------------------------------------------------
    # PyQtGraph methods

    def raiseContextMenu(self, event):
        if self.menu is None or not self.menuEnabled():
            return
        pos = event.screenPos()
        self.menu.popup(QPoint(pos.x(), pos.y()))
