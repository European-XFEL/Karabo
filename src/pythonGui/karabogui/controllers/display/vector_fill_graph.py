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

from traits.api import Instance, WeakRef

from karabo.common.scenemodel.api import (
    VectorFillGraphModel, build_model_config)
from karabogui.binding.api import VectorNumberBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.plots.api import KaraboPlotView, generate_down_sample


@register_binding_controller(ui_name='Vector Fill Graph',
                             klassname='VectorFillGraph',
                             binding_type=VectorNumberBinding,
                             can_show_nothing=False)
class DisplayVectorFillGraph(BaseBindingController):
    """The VectorFill Graph controller for display of vector data

    This widget fills the background between curve and axis.
    """
    model = Instance(VectorFillGraphModel, args=())
    _plot = WeakRef(object)

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
        _, y = generate_down_sample(value, threshold=2000, rect=None)
        self._plot.setData(y)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)
