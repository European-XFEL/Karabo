#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

import numpy as np
from PyQt5.QtWidgets import QAction
from traits.api import Instance, Undefined

from karabo.common.scenemodel.api import (
    build_graph_config, NDArrayGraphModel, restore_graph_config)
from karabogui.binding.api import NDArrayBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.plots.api import (
    KaraboPlotView, generate_down_sample, generate_baseline, get_view_range,
    TransformDialog)


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
        widget.enable_export()
        widget.restore(build_graph_config(self.model))
        widget.enable_data_toggle()

        trans_action = QAction("X-Transformation", widget)
        trans_action.triggered.connect(self.configure_transformation)

        widget.addAction(trans_action)

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
            model = self.model
            # Generate the baseline for the x-axis
            x = generate_baseline(value, offset=model.offset,
                                  step=model.step)
            rect = get_view_range(self._plot)
            x, y = generate_down_sample(value, rect=rect, x=x, deviation=True)
            self._plot.setData(x, y)

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
