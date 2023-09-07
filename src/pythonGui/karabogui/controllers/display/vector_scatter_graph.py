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

from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import ArrayOrNone, Instance, WeakRef

from karabo.common.scenemodel.api import (
    VectorScatterGraphModel, build_graph_config, build_model_config)
from karabogui import icons
from karabogui.binding.api import (
    PropertyProxy, VectorNumberBinding, get_binding_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.toolbar.widgets import create_button
from karabogui.graph.plots.api import KaraboPlotView, ScatterGraphPlot
from karabogui.graph.plots.dialogs.data_analysis import DataAnalysisDialog

MIN_POINT_SIZE = 0.1
MAX_POINT_SIZE = 10.0


@register_binding_controller(ui_name='Vector XY Scatter',
                             klassname='VectorScatterGraph',
                             binding_type=VectorNumberBinding,
                             can_show_nothing=False)
class DisplayVectorScatterGraph(BaseBindingController):
    """The scatter graph widget provides a scatter plot from property proxies
    """
    model = Instance(VectorScatterGraphModel, args=())

    # Internal traits
    _x_proxy = Instance(PropertyProxy)
    _y_proxy = Instance(PropertyProxy)
    _last_x_value = ArrayOrNone
    _plot = WeakRef(ScatterGraphPlot)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        toolbar = widget.add_toolbar()
        widget.enable_export()
        widget.stateChanged.connect(self._change_model)

        self._plot = widget.add_scatter_item(cycle=False)
        # assigning proxy is safe and wanted here!
        self._x_proxy = self.proxy
        self._last_x_value = get_binding_value(self.proxy)
        widget.restore(build_model_config(self.model))
        self._plot.setSize(self.model.psize)

        point_size_action = QAction("Point Size", widget)
        point_size_action.triggered.connect(self._configure_point_size)
        point_size_action.setIcon(icons.scatter)
        widget.addAction(point_size_action)
        button = create_button(
            icon=icons.data_analysis, checkable=False, tooltip="Data Analysis",
            on_clicked=self.show_data_analysis_dialog)
        toolbar.add_button(button)

        return widget

    # ----------------------------------------------------------------

    def show_data_analysis_dialog(self):
        config = build_graph_config(self.model)
        dialog = DataAnalysisDialog(
            proxies=self.proxies[1:], config=config, baseline_proxy=self.proxy,
            parent=self.widget)
        dialog.show()

    def add_proxy(self, proxy):
        if self._y_proxy is None:
            self._y_proxy = proxy
            return True

        return False

    def remove_proxy(self, proxy):
        if self._y_proxy is not None:
            self._y_proxy = None
            return True

        return False

    def value_update(self, proxy):
        value = proxy.value
        if proxy is self._x_proxy:
            self._last_x_value = value
        elif proxy is self._y_proxy:
            if value is None:
                return
            if self._last_x_value is not None and self.widget is not None:
                size_x = len(self._last_x_value)
                size_y = len(value)
                size = min(size_x, size_y)
                if size == 0:
                    self._plot.setData([], [])
                    return

                x = self._last_x_value
                if size_x != size_y:
                    x = self._last_x_value[:size]
                    value = value[:size]
                self._plot.setData(x, value)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)

    def _configure_point_size(self):
        psize, ok = QInputDialog.getDouble(self.widget, 'Size of points',
                                           'Pointsize:', self.model.psize,
                                           MIN_POINT_SIZE, MAX_POINT_SIZE)
        if ok:
            self._plot.setSize(psize)
            self.model.psize = psize
