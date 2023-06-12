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
from datetime import datetime

from pyqtgraph import InfiniteLine, SignalProxy
from qtpy.QtCore import QObject, Slot

from karabogui import icons
from karabogui.graph.common.api import (
    AlarmAxisItem, CoordsLegend, StateAxisItem, TimeAxisItem, create_button,
    float_to_string, get_alarm_string, get_state_string, make_pen)


class CrossTargetController(QObject):
    def __init__(self, plotItem):
        """Controller for showing a cross target on a plot"""
        super().__init__()
        self.plotItem = plotItem

        self.v_line = None
        self.h_line = None
        self.proxy = None
        self.legend = None
        self.action_button = create_button(
            checkable=True,
            icon=icons.target,
            tooltip="Get a CrossTarget for the plot",
            on_clicked=self.toggle)

    @Slot(object)
    def mouseMoved(self, event):
        """Catch the mouseMove event on the plotItem and show coordinates"""
        pos = event[0]
        # using signal proxy turns original arguments into a tuple
        if self.plotItem.sceneBoundingRect().contains(pos):
            mousePoint = self.plotItem.vb.mapSceneToView(pos)
            x, y = mousePoint.x(), mousePoint.y()
            self.legend.set_value(self._get_x_value(x), self._get_y_value(y))
            self.v_line.setPos(x)
            self.h_line.setPos(y)
            self.legend.setVisible(True)
        else:
            self.legend.setVisible(False)

    def _get_x_value(self, value):
        """Get the real x value depending on the axis"""
        plotItem = self.plotItem
        axis = plotItem.getAxis("bottom")
        if isinstance(axis, TimeAxisItem):
            x_min, x_max = axis.range
            difference = x_max - x_min
            if difference < 60:
                fmt = "%H:%M:%S,%f"
            elif difference < 3600 * 24:
                fmt = "%H:%M:%S"
            elif difference < 3600 * 24 * 30:
                fmt = "%d/%m, %H:%M:%S"
            elif difference < 3600 * 24 * 30 * 365:
                fmt = "%d/%m, %H:%M"
            else:
                fmt = "%d/%m/%Y"
            timestamp = datetime.fromtimestamp(value)
            return timestamp.strftime(fmt)
        elif axis.logMode:
            return convert_log(value)
        return float_to_string(value, precision=3)

    def _get_y_value(self, value):
        """Get the real y value depending on the axis"""
        plotItem = self.plotItem
        axis = plotItem.getAxis("left")
        if isinstance(axis, StateAxisItem):
            return get_state_string(value)
        elif isinstance(axis, AlarmAxisItem):
            return get_alarm_string(value)
        elif axis.logMode:
            return convert_log(value)
        return float_to_string(value, precision=3)

    # -----------------------------------------------------------------------
    # Public methods

    @Slot(bool)
    def toggle(self, state):
        if state:
            self.activate()
        else:
            self.deactivate()

    def activate(self):
        pen = make_pen("a")
        self.v_line = InfiniteLine(angle=90, pen=pen, movable=False)
        self.h_line = InfiniteLine(angle=0, pen=pen, movable=False)
        self.plotItem.addItem(self.v_line, ignoreBounds=True)
        self.plotItem.addItem(self.h_line, ignoreBounds=True)
        self.legend = CoordsLegend(color='k')
        self.legend.setParentItem(self.plotItem.vb)
        self.legend.anchor(itemPos=(1, 0), parentPos=(1, 0), offset=(-5, 5))
        self.proxy = SignalProxy(self.plotItem.scene().sigMouseMoved,
                                 rateLimit=50, slot=self.mouseMoved)

    def deactivate(self):
        self.proxy.disconnect()
        self.proxy = None
        for item in [self.h_line, self.v_line]:
            self.plotItem.removeItem(item)
            item.deleteLater()
        self.v_line = None
        self.h_line = None
        self.legend.setParentItem(None)
        self.legend.deleteLater()
        self.legend = None


def convert_log(value):
    """Return the e-annotation of the given value with 3 precisions."""
    try:
        return f"{10 ** value:.3e}"
    except Exception:
        return str(value)
