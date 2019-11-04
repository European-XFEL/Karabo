#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from PyQt5.QtWidgets import QAction, QInputDialog
from traits.api import Instance, Undefined

from karabo.common.scenemodel.api import (
    build_graph_config, NDArrayGraphModel, restore_graph_config)
from karabogui.binding.api import NDArrayBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.const import MIN_DOWNSAMPLE, MAX_DOWNSAMPLE
from karabogui.graph.plots.api import (
    KaraboPlotView, generate_down_sample, get_view_range)
from karabogui import icons


@register_binding_controller(ui_name='NDArray Graph',
                             klassname='NDArrayGraph',
                             binding_type=NDArrayBinding,
                             can_show_nothing=False)
class DisplayNDArrayGraph(BaseBindingController):
    """The NDArray controller for display of pulse data"""
    model = Instance(NDArrayGraphModel, args=())
    _plot = Instance(object)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        self._plot = widget.add_curve_item()
        widget.add_cross_target()
        widget.add_roi()
        widget.add_toolbar()
        widget.restore(build_graph_config(self.model))

        downsample_action = QAction("Downsample", widget)
        downsample_action.triggered.connect(self.configure_downsample)
        downsample_action.setIcon(icons.downsample)
        widget.addAction(downsample_action)

        return widget

    # ----------------------------------------------------------------

    def value_update(self, proxy):
        node = proxy.value
        data = node.data.value
        if data is None or data is Undefined:
            return

        arr_type = REFERENCE_TYPENUM_TO_DTYPE[node.type.value]
        value = np.frombuffer(data, dtype=arr_type)
        if value.ndim == 1:
            rect = get_view_range(self._plot)
            x, y = generate_down_sample(value, rect=rect,
                                        half_samples=self.model.half_samples,
                                        deviation=True)
            self._plot.setData(x, y)

    # ----------------------------------------------------------------
    # Qt Slots

    # @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**restore_graph_config(content))

    # @pyqtSlot()
    def configure_downsample(self, checked):
        sample, ok = QInputDialog.getInt(
            self.widget, 'Downsample', 'Samples half:',
            self.model.half_samples,
            MIN_DOWNSAMPLE, MAX_DOWNSAMPLE)
        if ok:
            self.model.half_samples = sample


REFERENCE_TYPENUM_TO_DTYPE = {
    0: 'bool',
    2: 'char',
    4: 'int8',
    6: 'uint8',
    8: 'int16',
    10: 'uint16',
    12: 'int32',
    14: 'uint32',
    16: 'int64',
    18: 'uint64',
    20: 'float',
    22: 'float64'
}
