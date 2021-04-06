#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from qtpy.QtWidgets import QAction
from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import (
    build_model_config, VectorHistGraphModel)
from karabogui.binding.api import NDArrayBinding, VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, get_array_data, register_binding_controller)
from karabogui.graph.plots.api import HistogramDialog, KaraboPlotView
from karabogui import icons


@register_binding_controller(
    ui_name='Vector HistoGram Graph',
    klassname='VectorHistGraph',
    binding_type=(VectorNumberBinding, NDArrayBinding),
    priority=-10,
    can_show_nothing=False)
class VectorHistogramGraph(BaseBindingController):
    """The controller for display of data in a histogram format"""
    model = Instance(VectorHistGraphModel, args=())

    _plot = WeakRef(object)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.enable_export()
        widget.stateChanged.connect(self._change_model)
        self._plot = widget.add_curve_item()

        widget.restore(build_model_config(self.model))

        hist_action = QAction("Histogram settings ...", widget)
        hist_action.triggered.connect(self.configure_histogram)
        hist_action.setIcon(icons.histogram)
        widget.addAction(hist_action)

        return widget

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)

    def configure_histogram(self):
        content, ok = HistogramDialog.get(build_model_config(self.model),
                                          parent=self.widget)
        if ok:
            self.model.trait_set(**content)
            if len(self.proxy.value):
                self.value_update(self.proxy)

    def _plot_histogram(self, value):
        """The plot function for the histogram called by `value_update`"""
        # XXX: Do we still need this?
        finite = np.isfinite(value)
        if not np.all(finite):
            value = value.copy()
            value[~finite] = 0

        if self.model.auto:
            start, stop = value.min(), value.max()
        else:
            start = min([self.model.start, self.model.stop])
            stop = max([self.model.start, self.model.stop])
        bins = np.linspace(start=start, stop=stop, num=self.model.bins + 1,
                           endpoint=True)
        hist, edges = np.histogram(value, bins=bins)
        if len(edges) > 1:
            bin_w = (stop - start) / (self.model.bins)
            self._plot.setData(edges - bin_w / 2, hist, fillLevel=0,
                               stepMode=True)

    def value_update(self, proxy):
        value, _ = get_array_data(proxy)
        if value is None or not len(value):
            self._plot.setData([], stepMode=None)
            return

        # NOTE: WE cast boolean as int!
        if value.dtype == np.bool:
            value = value.astype(np.int)

        self._plot_histogram(value)
