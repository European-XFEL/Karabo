from guiqwt.plot import CurveDialog
from guiqwt.builder import make
from PyQt4.Qwt5.Qwt import QwtPlot
from traits.api import Dict, Instance, on_trait_change

from karabo.common.scenemodel.api import LinePlotModel
from karabogui.binding.api import VectorBinding
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.controllers.util import axis_label


@register_binding_controller(ui_name='XY-Plot', binding_type=VectorBinding)
class XYVector(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(LinePlotModel)
    # Internal traits
    _plot = Instance(object)  # Some Qwt bullshit
    _curves = Dict

    def add_proxy(self, proxy):
        curve = make.curve([], [], axis_label(proxy), "r")
        self._add_curve(proxy, curve)
        return True

    def create_widget(self, parent):
        widget = CurveDialog(edit=False, toolbar=True, wintitle="XY-Plot")
        self._plot = widget.get_plot()
        self._plot.set_antialiasing(True)
        self._plot.setAxisAutoScale(QwtPlot.yLeft)
        self._plot.setAxisAutoScale(QwtPlot.xBottom)
        return widget

    @on_trait_change('proxies.binding')
    def _binding_update(self, proxy, name, binding):
        if name != 'binding':
            return

        pos = QwtPlot.xBottom if proxy is self.proxy else QwtPlot.yLeft
        self._plot.setAxisTitle(pos, axis_label(proxy))

    @on_trait_change('proxies.value')
    def _value_update(self, proxy, name, value):
        if name != 'value':
            return
        if self.proxy.binding is None:
            return

        if proxy is self.proxy:
            for p, c in self._curves.items():
                if len(value) == len(p.value):
                    c.set_data(value, p.value)
        elif len(self.proxy.value) == len(value):
            self._curves[proxy].set_data(self.proxy.value, value)
        self._plot.replot()

    def _add_curve(self, proxy, curve):
        if proxy in self._curves:
            old_curve = self._curves[proxy]
            self._plot.del_item(old_curve)

        self._curves[proxy] = curve
        self._plot.add_item(curve)
