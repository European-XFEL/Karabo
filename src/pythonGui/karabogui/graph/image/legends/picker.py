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
from pyqtgraph import LabelItem
from pyqtgraph.graphicsItems.LegendItem import ItemSample
from qtpy.QtCore import Qt
from qtpy.QtGui import QBrush, QColor, QPen

from karabogui.graph.common.api import KaraboLegend, float_to_string


class PickerLegend(KaraboLegend):

    def __init__(self):
        super().__init__()
        self.layout.setContentsMargins(2, 2, 2, 2)
        self.layout.setHorizontalSpacing(15)

        self._color_box = ColorSample(None)
        self.layout.addItem(self._color_box, 0, 0)

        self._label = LabelItem(color='w', size="8pt", justify="left")
        self.layout.addItem(self._label, 0, 1)

        self.layout.setColumnFixedWidth(0, self._color_box.width())
        self.layout.setRowAlignment(0, Qt.AlignVCenter)
        self.layout.setColumnAlignment(1, Qt.AlignLeft)
        self.hide()

    def set_value(self, x, y, value, color):
        # Note: The value of a picked pixel can be either an rgb (array) or
        # a lut value (scalar).
        x = float_to_string(x)
        y = float_to_string(y)
        value = float_to_string(value) if type(value) is float else str(value)
        self._label.setText(f"x: {x}<br>y: {y}<br>value: {value}")
        self._color_box.setBrush(QBrush(color))
        self.updateSize()

    def updateSize(self):
        if self.size is not None:
            return

        self.prepareGeometryChange()
        width = (self._color_box.width() + self._label.width())
        self.setGeometry(0, 0, width, self._label.height())


class ColorSample(ItemSample):
    """The color item in the legend that shows the selected pixel color"""

    def __init__(self, item):
        super().__init__(item)

        self._brush = Qt.NoBrush
        self._pen = QPen(QColor(196, 197, 193, 200), 1)

    def paint(self, painter, *args):
        painter.setPen(self._pen)
        painter.setBrush(self._brush)
        painter.drawRect(0, 0, 12, 45)

    def setBrush(self, brush):
        self._brush = brush
        self.update()
