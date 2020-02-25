#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from traits.api import Any, Instance

from karabo.common.scenemodel.api import build_model_config, VectorXYGraphModel
from karabogui.binding.api import (
    get_binding_value, PropertyProxy, VectorBoolBinding, VectorNumberBinding)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.plots.api import KaraboPlotView


def _is_compatible(binding):
    """Don't allow plotting of boolean vectors"""
    return not isinstance(binding, VectorBoolBinding)


@register_binding_controller(ui_name='Vector XY Graph',
                             klassname='VectorXYGraph',
                             binding_type=VectorNumberBinding,
                             is_compatible=_is_compatible,
                             can_show_nothing=False)
class DisplayVectorXYGraph(BaseBindingController):
    """The vector xy plot graph class

    - First property proxy (dragged) will declare the x axis
    - Second property proxy (added) will declare the y axis
    """
    model = Instance(VectorXYGraphModel, args=())
    # Internal traits
    _x_proxy = Instance(PropertyProxy)
    _y_proxy = Instance(PropertyProxy)
    _last_x_value = Any
    _plot = Instance(object)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.enable_data_toggle()
        widget.enable_export()

        widget.restore(build_model_config(self.model))
        self._plot = widget.add_curve_item()
        # assigning proxy is safe and wanted here!
        self._x_proxy = self.proxy
        self._last_x_value = get_binding_value(self.proxy)

        return widget

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        if self._y_proxy is None:
            self._y_proxy = proxy
            return True
        return False

    def value_update(self, proxy):
        value = get_binding_value(proxy.binding)
        if value is None:
            return
        if proxy is self._x_proxy:
            self._last_x_value = value
        if proxy is self._y_proxy and self._last_x_value is not None:
            if self.widget is not None:
                # Be cautious and ask for the size!
                if len(self._last_x_value) != len(value):
                    self._plot.setData([], [])
                    return
                self._plot.setData(self._last_x_value, value)

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)
