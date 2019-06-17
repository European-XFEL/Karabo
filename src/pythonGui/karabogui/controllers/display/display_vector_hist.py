#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QAction
from traits.api import Instance

from karabo.common.scenemodel.api import (
    build_model_config, VectorHistGraphModel)
from karabogui.binding.api import VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.plots.api import HistogramDialog, KaraboPlotView
from karabogui import icons


@register_binding_controller(ui_name='Vector HistoGram Graph',
                             klassname='VectorHistGraph',
                             binding_type=VectorNumberBinding,
                             can_show_nothing=False)
class DisplayHistGraph(BaseBindingController):
    """The BarGraph controller for display of pulse data in a histogram format
    """
    model = Instance(VectorHistGraphModel, args=())
    _plot = Instance(object)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.stateChanged.connect(self._change_model)
        self._plot = widget.add_curve_item()

        widget.restore(build_model_config(self.model))

        hist_action = QAction("Histogram settings ...", widget)
        hist_action.triggered.connect(self.configure_histogram)
        hist_action.setIcon(icons.histogram)
        widget.addAction(hist_action)

        return widget

    # ----------------------------------------------------------------

    def value_update(self, proxy):
        value = proxy.value

        if self.model.auto:
            start, stop = value.min(), value.max()
        else:
            start = min([self.model.start, self.model.stop])
            stop = max([self.model.start, self.model.stop])
        bins = np.linspace(start=start, stop=stop, num=self.model.bins,
                           endpoint=True)
        hist, edges = np.histogram(value, bins=bins)
        if len(edges) > 1:
            self._plot.setData(edges, hist, stepMode=True, fillLevel=0)

    # ----------------------------------------------------------------
    # Qt Slots

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**content)

    @pyqtSlot()
    def configure_histogram(self):
        content, ok = HistogramDialog.get(build_model_config(self.model),
                                          parent=self.widget)
        if ok:
            self.model.trait_set(**content)
