from guiqwt.plot import CurveDialog
from guiqwt.builder import make
from PyQt4.Qwt5.Qwt import QwtPlot
from traits.api import Dict, Instance

from karabo.common.scenemodel.api import LinePlotModel
from karabogui.binding.api import (get_binding_value, VectorBoolBinding,
                                   VectorNumberBinding)
from karabogui.controllers.api import (
    BaseBindingController, axis_label, register_binding_controller)


def _is_compatible(binding):
    """Don't allow plotting of boolean vectors"""
    return not isinstance(binding, VectorBoolBinding)


@register_binding_controller(ui_name='XY-Plot', klassname='XYVector',
                             binding_type=VectorNumberBinding,
                             is_compatible=_is_compatible,
                             can_show_nothing=False)
class XYVector(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(LinePlotModel, args=())
    # Internal traits
    _plot = Instance(object)  # Some Qwt bullshit
    _curves = Dict

    def add_proxy(self, proxy):
        curve = make.curve([], [], axis_label(proxy), "r")
        self._add_curve(proxy, curve)

        # get_binding_value makes sure that proxy.value is not Undefined
        if get_binding_value(proxy) is not None:
            self.binding_update(proxy)
            self.value_update(proxy)
        return True

    def create_widget(self, parent):
        widget = CurveDialog(edit=False, toolbar=True, wintitle="XY-Plot")
        self._plot = widget.get_plot()
        self._plot.set_antialiasing(True)
        self._plot.setAxisAutoScale(QwtPlot.yLeft)
        self._plot.setAxisAutoScale(QwtPlot.xBottom)
        return widget

    def binding_update(self, proxy):
        pos = QwtPlot.xBottom if proxy is self.proxy else QwtPlot.yLeft
        self._plot.setAxisTitle(pos, axis_label(proxy))

    def value_update(self, proxy):
        value = proxy.value
        if len(value) > 0:
            if proxy is self.proxy:
                for p, c in self._curves.items():
                    # since proxy is used as key for stored curves, before
                    # getting the previous values from the proxy, we have to
                    # check for Undefined
                    y_val = get_binding_value(p, [])
                    if len(value) == len(y_val):
                        c.set_data(value, y_val)
            else:
                x_val = (get_binding_value(self.proxy, []))
                if len(value) == len(x_val):
                    self._curves[proxy].set_data(x_val, value)
            self._plot.replot()

    def _add_curve(self, proxy, curve):
        if proxy in self._curves:
            old_curve = self._curves[proxy]
            self._plot.del_item(old_curve)

        self._curves[proxy] = curve
        self._plot.add_item(curve)
