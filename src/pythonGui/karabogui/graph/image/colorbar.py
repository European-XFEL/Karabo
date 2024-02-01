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
from pyqtgraph import (
    AxisItem, ColorMap, GraphicsWidget, ImageItem, SignalProxy, ViewBox)
from qtpy.QtCore import QPoint, Qt, Signal, Slot
from qtpy.QtGui import QTransform
from qtpy.QtWidgets import QDialog, QGraphicsGridLayout, QMenu

from karabogui.fonts import get_qfont
from karabogui.graph.common.api import COLORMAPS
from karabogui.util import generateObjectName, move_to_cursor

from .dialogs.levels import LevelsDialog
from .utils import ensure_finite_levels, get_level_limits, levels_almost_equal

NUM_SAMPLES = 256


class ColorBarWidget(GraphicsWidget):
    levelsChanged = Signal(object)

    def __init__(self, imageItem, parent=None):
        super().__init__(parent=parent)
        self.setObjectName(generateObjectName(self))
        self.imageItem = imageItem

        self.levels = min_level, max_level = [0, NUM_SAMPLES - 1]
        data = np.linspace(min_level, max_level, NUM_SAMPLES)[None, :]

        self.grid_layout = QGraphicsGridLayout(self)
        self.grid_layout.setSpacing(0)
        self.grid_layout.setContentsMargins(0, 40, 0, 0)

        self.vb = ColorViewBox()
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

    @Slot(object)
    def dynamic_event_throttle(self, event):
        levels = event[0]
        if levels is None:
            bar_level = self.imageItem.image.min(), self.imageItem.image.max()
        else:
            bar_level = levels

        # Set the bar levels different, global levels can be `None` for
        # auto scale
        self.set_levels(bar_level)
        self.levelsChanged.emit(levels)

    @Slot()
    def _show_levels_dialog(self):
        levels = self.imageItem.levels
        if levels is None:
            return  # `None` protection
        image_range = self.imageItem.image.min(), self.imageItem.image.max()
        auto_levels = self.imageItem.auto_levels
        default = None if auto_levels else levels
        limits = get_level_limits(self.imageItem.image)

        dialog = LevelsDialog(levels, image_range, auto_levels,
                              limits=limits, parent=self.parent())
        move_to_cursor(dialog)
        proxy = SignalProxy(dialog.levelsPreview, rateLimit=10,
                            slot=self.dynamic_event_throttle)
        accepted = dialog.exec() == QDialog.Accepted
        proxy.disconnect()
        if accepted:
            levels = dialog.levels
        else:
            levels = default

        bar_levels = image_range if levels is None else levels
        self.set_levels(bar_levels)
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
        lut = ColorMap(*zip(*COLORMAPS[cmap])).getLookupTable(
            alpha=False)
        self.barItem.setLookupTable(lut)

    def set_margins(self, top=None, bottom=None):
        """Sets top and bottom margins of the colorbar. This depend on the
        axis size of the main plot."""
        if top is None:
            _, top, _, _ = self.grid_layout.getContentsMargins()
        if bottom is None:
            _, _, _, bottom = self.grid_layout.getContentsMargins()

        self.grid_layout.setContentsMargins(0, top, 0, bottom)

    def set_levels(self, image_range):
        """Set the mininum and maximum levels on the color bar"""
        image_min, image_max = ensure_finite_levels(image_range)
        # Check if levels and range are almost equal.
        # Only change y-range values if not.
        if levels_almost_equal(self.levels, [image_min, image_max]):
            return
        # Transform the bar item according to the range
        if image_min == image_max:
            # Protect against same levels and simply lower the minimum
            # for a functional colorbar
            image_min -= 1

        scale = (image_max - image_min) / NUM_SAMPLES
        translate = image_min / scale
        transform = QTransform()
        transform.scale(1, scale)
        transform.translate(0, translate)
        self.barItem.setTransform(transform)
        self.vb.setYRange(image_min, image_max, padding=0)
        self.levels = image_range

    # ---------------------------------------------------------------------
    # Private methods

    def _create_menu(self):
        menu = QMenu()
        menu.addAction("Set levels...", self._show_levels_dialog)
        return menu


class ColorViewBox(ViewBox):

    def __init__(self, parent=None):
        super().__init__(parent=parent, enableMenu=False, enableMouse=False)
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
        self.menu.popup(QPoint(int(pos.x()), int(pos.y())))
