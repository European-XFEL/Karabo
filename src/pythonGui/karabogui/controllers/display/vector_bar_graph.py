#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, WeakRef

from karabogui.binding.api import (NDArrayBinding, VectorNumberBinding)
from karabogui.controllers.api import (
    BaseBindingController, get_array_data, register_binding_controller)

from karabo.common.scenemodel.api import (
    build_model_config, VectorBarGraphModel)
from karabogui.graph.plots.api import (
    generate_down_sample, get_view_range, KaraboPlotView, VectorBarGraphPlot)

MAX_WIDTH = 100
MAX_BARS = 3000


@register_binding_controller(
    ui_name='Vector Bar Graph',
    klassname='VectorBarGraph',
    binding_type=(NDArrayBinding, VectorNumberBinding),
    can_show_nothing=False)
class DisplayVectorBarGraph(BaseBindingController):
    """The BarGraph controller for display of pulse data in a histogram format
    """
    model = Instance(VectorBarGraphModel, args=())
    _plot = WeakRef(VectorBarGraphPlot)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.stateChanged.connect(self._change_model)
        self._plot = widget.add_bar_item()
        self._plot.set_width(self.model.bar_width)

        widget.restore(build_model_config(self.model))

        width_action = QAction("Bar Width", widget)
        width_action.triggered.connect(self.configure_bar_width)
        widget.addAction(width_action)

        return widget

    def value_update(self, proxy):
        value, _ = get_array_data(proxy)
        if value is None or not len(value):
            self._plot.setData([], [])
            return

        rect = get_view_range(self._plot)
        x, y = generate_down_sample(value, threshold=MAX_BARS, rect=rect)
        self._plot.setData(x, y)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)

    def configure_bar_width(self):
        bar_width, ok = QInputDialog.getDouble(
            self.widget, 'Bar Width', 'Bar Width:',
            self.model.bar_width, 0.1, MAX_WIDTH)
        if ok:
            self.model.bar_width = bar_width
            self._plot.set_width(bar_width)
