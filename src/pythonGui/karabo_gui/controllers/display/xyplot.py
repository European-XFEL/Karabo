from collections import deque

from traits.api import Any, Instance, on_trait_change

from karabo.common.scenemodel.api import XYPlotModel
from karabo_gui.binding.api import (
    BaseBindingController, PropertyProxy, register_binding_controller,
    FloatBinding, Int8Binding, Int16Binding, Int32Binding, Int64Binding,
    Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding
)
from karabo_gui.const import MAXNUMPOINTS
from karabo_gui.controllers.util import axis_label
from karabo_gui.mplwidget.mplplotwidgets import MplCurvePlot

NUMERICAL_BINDINGS = (
    FloatBinding, Int8Binding, Int16Binding, Int32Binding, Int64Binding,
    Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding)


@register_binding_controller(ui_name='XY-Plot', read_only=True,
                             binding_type=NUMERICAL_BINDINGS)
class XYPlot(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(XYPlotModel)
    # Internal traits
    _x_proxy = Instance(PropertyProxy)
    _y_proxy = Instance(PropertyProxy)
    _x_values = Instance(deque, kw={'maxlen': MAXNUMPOINTS})
    _y_values = Instance(deque, kw={'maxlen': MAXNUMPOINTS})
    _last_x_value = Any

    def create_widget(self, parent):
        widget = MplCurvePlot(legend=True)
        self._x_proxy = self.proxy
        self._x_values = deque(maxlen=MAXNUMPOINTS)
        self._y_values = deque(maxlen=MAXNUMPOINTS)

        if self.proxy.binding is not None:
            self._last_x_value = self.proxy.value
        else:
            self._last_x_value = None
        return widget

    def add_proxy(self, proxy):
        if self._y_proxy is None:
            self._y_proxy = proxy
            self.widget.new_curve(label='Random values')
            return True
        return False

    @on_trait_change('proxies.binding')
    def _binding_update(self, proxy, name, value):
        if name != 'binding':
            return

        fname = 'set_xlabel' if proxy is self._x_proxy else 'set_ylabel'
        self.widget.axes_call(fname, axis_label(proxy))

    @on_trait_change('proxies.value')
    def _value_update(self, proxy, name, value):
        if name != 'value':
            return

        if proxy is self._x_proxy:
            self._last_x_value = value
        elif proxy is self._y_proxy:
            if self._last_x_value is not None:
                self._x_values.append(self._last_x_value)
                self._y_values.append(value)
                if self.widget is not None:
                    self.widget.update_curve(self._x_values,
                                             self._y_values,
                                             'Random values')

    def destroy_widget(self):
        self.widget.destroy()
