#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from itertools import cycle

from guiqwt.plot import CurveDialog
from guiqwt.builder import make
from numpy import arange
from traits.api import Dict, Instance

from karabo.common.scenemodel.api import DisplayPlotModel
from karabogui.binding.api import VectorBinding, KARABO_SCHEMA_DISPLAYED_NAME
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)

COLORS = ("red", "green", "blue", "gray", "violet", "orange", "lightgreen",
          "black")


@register_binding_controller(ui_name='Plot', klassname='DisplayPlot',
                             binding_type=VectorBinding)
class DisplayPlot(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(DisplayPlotModel)
    # Internal traits
    _plot = Instance(object)  # some Qwt bullshit
    _curves = Dict

    _line_colors = Instance(cycle, allow_none=False)

    def create_widget(self, parent):
        widget = CurveDialog(edit=False, toolbar=True, wintitle="Plot")
        self._plot = widget.get_plot()
        self.add_proxy(self.proxy)
        return widget

    def add_proxy(self, proxy):
        if proxy.binding is not None:
            self.binding_update(proxy)
        return True

    def binding_update(self, proxy):
        curve = self._curves.pop(proxy, None)
        if curve is not None:
            self._plot.del_item(curve)
        attrs = proxy.binding.attributes
        title = attrs.get(KARABO_SCHEMA_DISPLAYED_NAME, proxy.path)
        curve = make.curve([0, 1], [0, 1], title, next(self._line_colors))
        self._curves[proxy] = curve
        self._plot.add_item(curve)

    def value_update(self, proxy):
        if self._plot is None:
            return
        value = proxy.value
        self._curves[proxy].set_data(arange(len(value)), value)
        self._plot.replot()

    def __line_colors_default(self):
        return cycle(COLORS)
