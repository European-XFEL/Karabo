#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from itertools import cycle

from PyQt4.QtGui import QAction, QInputDialog
from PyQt4.QtCore import pyqtSlot
from traits.api import Dict, Instance

from karabogui.binding.api import VectorNumberBinding
from karabo.common.scenemodel.api import (
    build_graph_config, restore_graph_config, VectorGraphModel)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import get_pen_cycler
from karabogui.graph.common.const import MIN_DOWNSAMPLE, MAX_DOWNSAMPLE
from karabogui.graph.plots.api import (
    KaraboPlotView, generate_down_sample, get_view_range)
from karabogui import icons


@register_binding_controller(ui_name='Vector Graph', klassname='VectorGraph',
                             binding_type=VectorNumberBinding,
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

        downsample_action = QAction("Downsample", widget)
        downsample_action.triggered.connect(self.configure_downsample)
        downsample_action.setIcon(icons.downsample)
        widget.addAction(downsample_action)

        return widget

    def __pens_default(self):
        return get_pen_cycler()

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        if proxy.binding is not None:
            self.binding_update(proxy)
        return True

    def binding_update(self, proxy):
        if proxy in self._curves:
            return

        name = proxy.key
        curve = self.widget.add_curve_item(name=name, pen=next(self._pens))
        self._curves[proxy] = curve
        if len(self._curves) > 1:
            self.widget.set_legend(True)

    def value_update(self, proxy):
        if not self._curves:
            return

        y = proxy.value
        plot = self._curves[proxy]
        rect = get_view_range(plot)
        x, y = generate_down_sample(y, rect=rect,
                                    half_samples=self.model.half_samples,
                                    deviation=True)
        plot.setData(x, y)

    # ----------------------------------------------------------------
    # Qt Slots

    @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    @pyqtSlot()
    def configure_downsample(self):
        sample, ok = QInputDialog.getInt(
            self.widget, 'Downsample', 'Samples half:',
            self.model.half_samples,
            MIN_DOWNSAMPLE, MAX_DOWNSAMPLE)
        if ok:
            self.model.half_samples = sample
