#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on March 2, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from itertools import cycle

from guiqwt.plot import CurveDialog
from guiqwt.builder import make
from numpy import arange
from traits.api import Dict, Instance, on_trait_change

from karabo.common.scenemodel.api import DisplayPlotModel
from karabogui.binding.api import (
    BaseBindingController, VectorBinding, register_binding_controller,
    KARABO_SCHEMA_DISPLAYED_NAME
)

COLORS = ("red", "green", "blue", "gray", "violet", "orange", "lightgreen",
          "black")
COLOR_GEN = cycle(COLORS)


@register_binding_controller(ui_name='Plot', read_only=True,
                             binding_type=VectorBinding)
class DisplayPlot(BaseBindingController):
    # The scene data model class for this controller
    model = Instance(DisplayPlotModel)
    # Internal traits
    _plot = Instance(object)  # some Qwt bullshit
    _curves = Dict

    def create_widget(self, parent):
        widget = CurveDialog(edit=False, toolbar=True, wintitle="Plot")
        self._plot = widget.get_plot()
        self.add_proxy(self.proxy)
        return widget

    def add_proxy(self, proxy):
        attrs = proxy.binding.attributes
        title = attrs.get(KARABO_SCHEMA_DISPLAYED_NAME, proxy.path)
        curve = make.curve([0, 1], [0, 1], title, next(COLOR_GEN))
        self._curves[proxy] = curve
        self._plot.add_item(curve)
        return True

    @on_trait_change('proxies:value')
    def _replot(self, obj, name, new):
        if self._plot is None:
            return
        self._curves[obj].set_data(arange(len(new)), new)
        self._plot.replot()
