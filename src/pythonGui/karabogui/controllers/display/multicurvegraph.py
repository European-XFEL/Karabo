#############################################################################
# Author: <chen.xu@xfel.eu> & <dennis.goeries@xfel.eu>
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
from collections import deque
from functools import partial
from itertools import cycle, product
from weakref import WeakValueDictionary

import numpy as np
from qtpy.QtWidgets import QWidget
from traits.api import Bool, Callable, Dict, Instance, List

import karabogui.icons as icons
from karabo.common.scenemodel.api import (
    MultiCurveGraphModel, build_model_config)
from karabogui.binding.api import (
    BoolBinding, FloatBinding, IntBinding, PropertyProxy)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.graph.common.api import create_button, get_pen_cycler
from karabogui.graph.plots.api import KaraboPlotView

NUMERICAL_BINDINGS = (BoolBinding, FloatBinding, IntBinding)
MAXNUMPOINTS = 1000


def is_compatible(binding):
    """Reject a boolean for the x-axis"""
    return not isinstance(binding, BoolBinding)


@register_binding_controller(ui_name='Multi-Curve Graph',
                             klassname='MultiCurveGraph',
                             binding_type=NUMERICAL_BINDINGS,
                             is_compatible=is_compatible,
                             can_show_nothing=False)
class DisplayMultiCurveGraph(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(MultiCurveGraphModel, args=())
    # Internal traits
    _mpl_widget = Instance(QWidget)
    _reset_proxy = Instance(PropertyProxy)
    _y_proxies = List(Instance(PropertyProxy))
    _x_values = Instance(deque, kw={'maxlen': MAXNUMPOINTS})
    _y_values = Dict()  # {box.key(): deque of values}
    _last_values = Dict()  # used to synchronize x and y values
    _draw_start = Bool(False)  # indicate there is at least one x-y pair
    _resetbox_linked = Callable

    # The associated plot curves
    _curves = Instance(WeakValueDictionary, args=())
    _pens = Instance(cycle, allow_none=False)

    def __pens_default(self):
        return get_pen_cycler()

    def create_widget(self, parent):
        widget = KaraboPlotView(parent=parent)
        widget.add_legend(visible=False)
        widget.add_cross_target()
        widget.enable_data_toggle(True)
        toolbar = widget.add_toolbar()

        _btn_reset = create_button(checkable=False,
                                   icon=icons.reset,
                                   tooltip="Reset the plot",
                                   on_clicked=partial(self._reset_plot, True))
        toolbar.add_button(_btn_reset)
        widget.stateChanged.connect(self._change_model)

        # A closure to change the button style once a boolean is linked
        def _changestyle(default=False):
            icon = icons.reset if default else icons.resetTilted
            _btn_reset.setIcon(icon)

        self._resetbox_linked = _changestyle

        self._last_values[self.proxy] = None
        self._draw_start = True
        widget.restore(build_model_config(self.model))
        return widget

    def binding_update(self, proxy):
        self.add_proxy(proxy)

    def add_proxy(self, proxy):
        binding = proxy.binding
        if binding is None:
            # Wait for a new cycle
            return True

        if self._reset_proxy is None and isinstance(binding, BoolBinding):
            # if the resetbox is set, new boolean box goes to yvalues
            self._reset_proxy = proxy
            self._resetbox_linked()
            return True

        if proxy in self._last_values:
            return False

        self._last_values[proxy] = None
        self._y_proxies.append(proxy)
        # request a new curve for each new ybox, use proxy.key for the
        # curve name as this is unique
        name = proxy.key
        self._curves[proxy] = self.widget.add_curve_item(
            name=name, pen=next(self._pens))
        self._y_values[proxy] = deque(maxlen=MAXNUMPOINTS)

        # We can show the legend already here!
        self.widget.set_legend(True)

        return True

    def remove_proxy(self, proxy):
        if proxy is self._reset_proxy:
            self._reset_proxy = None
            self._resetbox_linked(default=True)
            return True
        if proxy in self._y_proxies:
            self._y_proxies.remove(proxy)

            item = self._curves.pop(proxy)
            self.widget.remove_item(item)
            self._y_values.pop(proxy)
            self._last_values.pop(proxy)
            if not self._curves:
                self.widget.set_legend(False)
            return True
        return False

    def value_update(self, proxy):
        value = proxy.value
        if proxy is self._reset_proxy:
            self._reset_plot(reset=value)
            return

        synchronizer = self._last_values
        synchronizer[proxy] = value
        if self._draw_start and None not in synchronizer.values():
            self._update_curve()

    # -------------------------------------------------------------------
    # private functions

    def _reset_plot(self, reset=True):
        """clear the plot"""
        if not reset:
            # when adding a boolean as resetbox, it may send a False value
            return

        self._last_values = {k: None for k in self._last_values.keys()}
        self._x_values.clear()
        for proxy, arr in self._y_values.items():
            arr.clear()
            self._curves[proxy].setData(self._x_values, arr)

    def _update_curve(self):
        """Update all curves, clear lastvalues buffer (set all values to None).
        This is called when xbox and all y boxes produce a not None value.
        """
        lastvalues = self._last_values
        for proxy, value in lastvalues.items():
            if proxy is self.proxy:
                self._x_values.append(value)
            else:
                self._y_values[proxy].append(value)

        # product pairs xvalues to each ('proxy', yvalues)
        for xdata, (proxy, ydata) in product((self._x_values,),
                                             self._y_values.items()):
            # later added y may have shorter length
            npx, npy = _convert_data(xdata, ydata)
            self._curves[proxy].setData(npx, npy)
        self._last_values = {k: None for k in lastvalues.keys()}

    # ----------------------------------------------------------------
    # Qt Slots

    def _change_model(self, content):
        self.model.trait_set(**content)


def _convert_data(xdata, ydata):
    """Convert x y deque array to numpy array for plotting.
    """
    # There might be np.nan in the array
    npx = np.array(xdata, dtype=np.float64)
    npy = np.array(ydata, dtype=np.float64)
    if npx.size != npy.size:
        # trim npx if necessary, len(npx) >= len(npy) always True
        npx = npx[-npy.size:]
    # get a mask to get rid of np.nan in y
    mask = np.isfinite(npy)
    return npx[mask], npy[mask]
