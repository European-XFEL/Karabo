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
from itertools import cycle
from weakref import WeakValueDictionary

from traits.api import Instance

from karabo.common.scenemodel.api import (
    VectorXYGraphModel, build_graph_config, build_model_config)
from karabogui import icons
from karabogui.binding.api import (
    VectorBoolBinding, VectorNumberBinding, get_binding_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import get_pen_cycler
from karabogui.graph.common.toolbar.widgets import create_button
from karabogui.graph.plots.api import (
    KaraboPlotView, generate_down_sample, get_view_range)
from karabogui.graph.plots.dialogs.data_analysis import DataAnalysisDialog


def _is_compatible(binding):
    """Don't allow plotting of boolean vectors"""
    return not isinstance(binding, VectorBoolBinding)


@register_binding_controller(ui_name='Vector XY Graph',
                             klassname='VectorXYGraph',
                             binding_type=VectorNumberBinding,
                             is_compatible=_is_compatible,
                             can_show_nothing=False)
class DisplayVectorXYGraph(BaseBindingController):
    """The vector xy plot graph class

    - First property proxy (dragged) will declare the x axis
    - Other property proxy (added) will declare the y axis curves
    """
    model = Instance(VectorXYGraphModel, args=())
    # Internal traits
    _curves = Instance(WeakValueDictionary, args=())
    _pens = Instance(cycle, allow_none=False)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_legend(visible=False)
        widget.add_cross_target()
        toolbar = widget.add_toolbar()
        widget.enable_data_toggle()
        widget.enable_export()

        widget.restore(build_model_config(self.model))

        button = create_button(
            icon=icons.data_analysis, checkable=False, tooltip="Data Analysis",
            on_clicked=self.show_data_analysis_dialog)
        toolbar.add_button(button)
        return widget

    def show_data_analysis_dialog(self):
        config = build_graph_config(self.model)
        data_analysis_dialog = DataAnalysisDialog(
            proxies=self.proxies[1:], config=config, baseline_proxy=self.proxy,
            parent=self.widget)
        data_analysis_dialog.show()

    def __pens_default(self):
        return get_pen_cycler()

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        name = proxy.key
        curve = self.widget.add_curve_item(name=name, pen=next(self._pens))
        self._curves[proxy] = curve
        if len(self._curves) > 1:
            self.widget.set_legend(True)
        return True

    def value_update(self, proxy):
        value = get_binding_value(proxy.binding, [])
        if len(value) > 0:
            # The x-axis proxy changed!
            if proxy is self.proxy:
                for p, c in self._curves.items():
                    # since proxy is used as key for stored curves, before
                    # getting the previous values from the proxy, we have to
                    # check for Undefined
                    y_value = get_binding_value(p, [])
                    self._plot_data(c, value, y_value)
            else:
                c = self._curves.get(proxy, None)
                if c is None:
                    # Note: This can happen on start up ...
                    return

                x_value = get_binding_value(self.proxy, [])
                self._plot_data(c, x_value, value)

    def _plot_data(self, curve, x, y):
        """Plot the data x and y on the `curve`

        Take into account a size missmatch of the data
        """
        size_x = len(x)
        size_y = len(y)
        size = min(size_x, size_y)
        if size == 0:
            curve.setData([], [])
            return

        if size_x != size_y:
            x = x[:size]
            y = y[:size]

        rect = get_view_range(curve)
        x, y = generate_down_sample(y, x=x, rect=rect, deviation=False)
        curve.setData(x, y)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)
