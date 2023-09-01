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

from qtpy.QtWidgets import QAction, QInputDialog
from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import (
    VectorBarGraphModel, build_model_config)
from karabogui.binding.api import NDArrayBinding, VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, get_array_data, register_binding_controller)
from karabogui.graph.common.const import ACTION_ITEMS
from karabogui.graph.plots.api import (
    KaraboPlotView, VectorBarGraphPlot, generate_down_sample, get_view_range)

MAX_WIDTH = 100
MAX_BARS = 3000

# Do not allow to set logarithmic scale on X-axis.This is to avoid overriding
# methods from upstream as pyqtgraph doesn't support log values on axes, yet
actions = [action for action in ACTION_ITEMS if action != 'x_log']


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
        widget = KaraboPlotView(parent=parent, actions=actions)
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
        value, _ = get_array_data(proxy, default=[])
        if not len(value):
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
