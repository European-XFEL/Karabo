#############################################################################
# Author: <dennis.goeries@xfel.eu>
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
#############################################################################

from collections import deque

from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Any, Instance, WeakRef

from karabo.common.scenemodel.api import ScatterGraphModel, build_model_config
from karabo.native import Timestamp
from karabogui import icons
from karabogui.binding.api import (
    BoolBinding, FloatBinding, IntBinding, PropertyProxy, get_binding_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import create_button
from karabogui.graph.plots.api import KaraboPlotView, ScatterGraphPlot

BUTTON_SIZE = (52, 32)
NUMERICAL_BINDINGS = (BoolBinding, FloatBinding, IntBinding)
MAX_NUM_POINTS = 1000

MIN_POINT_SIZE = 0.1
MAX_POINT_SIZE = 10.0


@register_binding_controller(ui_name="Scatter Graph", klassname="ScatterGraph",
                             binding_type=NUMERICAL_BINDINGS,
                             can_show_nothing=False)
class DisplayScatterGraph(BaseBindingController):
    """The scatter graph widget provides a scatter plot from property proxies

    - First property proxy (dragged) will declare the x axis
    - Second property proxy (added) will declare the y axis
    """
    model = Instance(ScatterGraphModel, args=())

    # Internal traits
    _y_proxy = Instance(PropertyProxy)
    _x_values = Instance(deque)
    _y_values = Instance(deque)
    _last_x_value = Any
    _plot = WeakRef(ScatterGraphPlot)

    _timestamp = Instance(Timestamp, args=())

    def create_widget(self, parent):

        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        toolbar = widget.add_toolbar()

        _btn_reset = create_button(checkable=False,
                                   icon=icons.reset,
                                   tooltip="Reset the plot",
                                   on_clicked=self._reset_plot)
        toolbar.add_button(_btn_reset)
        widget.stateChanged.connect(self._change_model)

        model = self.model
        self._x_values = deque(maxlen=model.maxlen)
        self._y_values = deque(maxlen=model.maxlen)

        self._plot = widget.add_scatter_item()

        # assigning proxy is safe and wanted here!
        self._last_x_value = get_binding_value(self.proxy)

        widget.restore(build_model_config(self.model))
        self._plot.setSize(self.model.psize)

        deque_action = QAction("Queue Size", widget)
        deque_action.triggered.connect(self._configure_deque)
        widget.addAction(deque_action)

        point_size_action = QAction("Point Size", widget)
        point_size_action.triggered.connect(self._configure_point_size)
        point_size_action.setIcon(icons.scatter)
        widget.addAction(point_size_action)

        return widget

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        if self._y_proxy is None:
            self._y_proxy = proxy
            return True

        return False

    def remove_proxy(self, proxy):
        if self._y_proxy is not None:
            self._y_proxy = None
            self._reset_plot()
            return True

        return False

    def value_update(self, proxy):
        value = proxy.value
        if proxy is self.proxy:
            self._last_x_value = value
        elif proxy is self._y_proxy and self._last_x_value is not None:
            timestamp = proxy.binding.timestamp
            if self.widget is not None and timestamp != self._timestamp:
                self._timestamp = timestamp
                self._x_values.append(self._last_x_value)
                self._y_values.append(value)
                self._plot.setData(self._x_values, self._y_values)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)

    def _reset_plot(self):
        self._last_x_value = None
        self._x_values.clear()
        self._y_values.clear()
        self._plot.clear()

    def _configure_deque(self):
        maxlen, ok = QInputDialog.getInt(self.widget, "Number of Values",
                                         "Maxlen:", self.model.maxlen, 5,
                                         MAX_NUM_POINTS)
        if ok:
            self._last_x_value = None
            self._x_values, self._y_values = None, None
            self._x_values = deque(maxlen=maxlen)
            self._y_values = deque(maxlen=maxlen)
            self.model.maxlen = maxlen

    def _configure_point_size(self):
        psize, ok = QInputDialog.getDouble(self.widget, "Size of points",
                                           "Pointsize:", self.model.psize,
                                           MIN_POINT_SIZE, MAX_POINT_SIZE)
        if ok:
            self._plot.setSize(psize)
            self.model.psize = psize
