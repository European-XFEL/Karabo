#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from itertools import cycle

import numpy as np
from PyQt5.QtWidgets import QAction
from traits.api import Dict, Instance

from karabogui.binding.api import VectorNumberBinding
from karabo.common.scenemodel.api import (
    build_graph_config, restore_graph_config, VectorGraphModel)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import get_pen_cycler
from karabogui.graph.plots.api import (
    KaraboPlotView, generate_down_sample, generate_baseline, get_view_range,
    TransformDialog)


@register_binding_controller(ui_name='Vector Graph', klassname='VectorGraph',
                             binding_type=VectorNumberBinding,
                             priority=90,
                             can_show_nothing=False)
class DisplayVectorGraph(BaseBindingController):
    """The VectorGraph controller for display basic vector data
    """
    model = Instance(VectorGraphModel, args=())
    _curves = Dict
    _pens = Instance(cycle, allow_none=False)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_legend(visible=False)
        widget.add_cross_target()
        widget.add_roi()
        widget.add_toolbar()
        widget.restore(build_graph_config(self.model))
        widget.enable_data_toggle()
        widget.enable_export()

        trans_action = QAction("X-Transformation", widget)
        trans_action.triggered.connect(self.configure_transformation)

        widget.addAction(trans_action)

        # Add first curve for the main proxy
        self._add_curve(self.proxy, widget=widget)

        return widget

    def __pens_default(self):
        return get_pen_cycler()

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        if proxy.binding is not None:
            self._add_curve(proxy)
            if len(self._curves) > 1:
                self.widget.set_legend(True)
        return True

    def _add_curve(self, proxy, widget=None):
        """The widget is passed as an argument in create_widget as it is not
           yet bound to self.widget then"""
        if widget is None:
            widget = self.widget

        name = proxy.key
        curve = widget.add_curve_item(name=name, pen=next(self._pens))
        self._curves[proxy] = curve

    def value_update(self, proxy):
        if not self._curves:
            return

        y = proxy.value
        plot = self._curves[proxy]
        # NOTE: With empty data or only inf we clear as NaN will clear as well!
        if not len(y) or np.isinf(y).all():
            plot.setData([], [])
            return

        # NOTE: WE cast boolean as int, as numpy method is deprecated
        if y.dtype == np.bool:
            y = y.astype(np.int)

        model = self.model
        # Generate the baseline for the x-axis
        x = generate_baseline(y, offset=model.offset, step=model.step)

        rect = get_view_range(plot)
        x, y = generate_down_sample(y, x=x, rect=rect, deviation=True)
        plot.setData(x, y)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    def configure_transformation(self):
        content, ok = TransformDialog.get(build_graph_config(self.model),
                                          parent=self.widget)
        if ok:
            self.model.trait_set(**content)
            for proxy in self.proxies:
                self.value_update(proxy)
