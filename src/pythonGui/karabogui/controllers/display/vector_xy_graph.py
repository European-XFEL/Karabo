#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#############################################################################
from itertools import cycle
from weakref import WeakValueDictionary

from traits.api import Instance

from karabo.common.scenemodel.api import VectorXYGraphModel, build_model_config
from karabogui.binding.api import (
    VectorBoolBinding, VectorNumberBinding, get_binding_value)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import get_pen_cycler
from karabogui.graph.plots.api import (
    KaraboPlotView, generate_down_sample, get_view_range)


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
    - Other property proxy (added) will declare the y axis curves
    """
    model = Instance(VectorXYGraphModel, args=())
    # Internal traits
    _curves = Instance(WeakValueDictionary, args=())
    _pens = Instance(cycle, allow_none=False)

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.stateChanged.connect(self._change_model)
        widget.add_legend(visible=False)
        widget.add_cross_target()
        widget.add_toolbar()
        widget.enable_data_toggle()
        widget.enable_export()

        widget.restore(build_model_config(self.model))

        return widget

    def __pens_default(self):
        return get_pen_cycler()

    # ----------------------------------------------------------------

    def add_proxy(self, proxy):
        name = proxy.key
        curve = self.widget.add_curve_item(name=name, pen=next(self._pens))
        self._curves[proxy] = curve
        if len(self._curves) > 1:
            self.widget.set_legend(True)
        return True

    def value_update(self, proxy):
        value = get_binding_value(proxy.binding, [])
        if len(value) > 0:
            # The x-axis proxy changed!
            if proxy is self.proxy:
                for p, c in self._curves.items():
                    # since proxy is used as key for stored curves, before
                    # getting the previous values from the proxy, we have to
                    # check for Undefined
                    y_val = get_binding_value(p, [])
                    if len(value) == len(y_val):
                        rect = get_view_range(c)
                        x, y = generate_down_sample(y_val, x=value, rect=rect,
                                                    deviation=False)
                        c.setData(x, y)

                    else:
                        c.setData([], [])
            else:
                curve = self._curves.get(proxy, None)
                if curve is None:
                    # Note: This can happen on start up ...
                    return
                x_val = get_binding_value(self.proxy, [])
                if len(value) == len(x_val):
                    rect = get_view_range(curve)
                    x, y = generate_down_sample(value, x=x_val, rect=rect,
                                                deviation=False)
                    curve.setData(x, y)
                else:
                    curve.setData([], [])

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)
