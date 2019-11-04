#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import Instance

from karabogui.binding.api import VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabo.common.scenemodel.api import (
    build_model_config, VectorFillGraphModel)
from karabogui.graph.plots.api import generate_down_sample, KaraboPlotView


@register_binding_controller(ui_name='Vector Fill Graph',
                             klassname='VectorFillGraph',
                             binding_type=VectorNumberBinding,
                             can_show_nothing=False)
class DisplayVectorFillGraph(BaseBindingController):
    """The VectorFill Graph controller for display of vector data

    This widget fills the background between curve and axis.
    """
    model = Instance(VectorFillGraphModel, args=())
    _plot = Instance(object)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.stateChanged.connect(self._change_model)
        self._plot = widget.add_curve_fill()
        widget.restore(build_model_config(self.model))

        return widget

    # ----------------------------------------------------------------

    def value_update(self, proxy):
        value = proxy.value
        # This plotItem is special!
        _, y = generate_down_sample(value, half_samples=700, rect=None)
        self._plot.setData(y)

    # ----------------------------------------------------------------
    # Qt Slots

    # @pyqtSlot(object)
    def _change_model(self, content):
        self.model.trait_set(**content)
