#############################################################################
# Author: <chen.xu@xfel.eu>
# Created on November 1, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from collections import deque
from functools import partial
from itertools import product

import numpy as np
from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QPushButton, QHBoxLayout, QVBoxLayout, QWidget
from traits.api import Bool, Callable, Dict, Instance, List

from karabo.common.scenemodel.api import MultiCurvePlotModel
from karabogui.binding.api import (
    PropertyProxy, BoolBinding, FloatBinding, IntBinding)
from karabogui.const import MAXNUMPOINTS
from karabogui.indicators import ALL_OK_COLOR
from karabogui.controllers.api import (
    BaseBindingController, axis_label, register_binding_controller)
from karabogui.mplwidget.mplplotwidgets import MplCurvePlot

BUTTON_SIZE = (52, 32)
NUMERICAL_BINDINGS = (BoolBinding, FloatBinding, IntBinding)


@register_binding_controller(ui_name='Multi-Curve Plot',
                             klassname='MultiCurvePlot',
                             binding_type=NUMERICAL_BINDINGS,
                             can_show_nothing=False)
class MultiCurvePlot(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(MultiCurvePlotModel, args=())
    # Internal traits
    _mpl_widget = Instance(QWidget)
    _reset_proxy = Instance(PropertyProxy)
    _x_proxy = Instance(PropertyProxy)
    _y_proxies = List(Instance(PropertyProxy))
    _x_values = Instance(deque, kw={'maxlen': MAXNUMPOINTS})
    _y_values = Dict()  # {box.key(): deque of values}
    _last_values = Dict()  # used to synchronize x and y values
    _draw_start = Bool(False)  # indicate there is at least one x-y pair
    _resetbox_linked = Callable

    def create_widget(self, parent):
        widget = QWidget(parent=parent)
        vbox = QVBoxLayout(widget)
        self._mpl_widget = MplCurvePlot(parent=widget, legend=True)
        vbox.addWidget(self._mpl_widget)

        hbox = QHBoxLayout()
        hbox.addStretch(1)  # push the button to the right side
        _btn_reset = QPushButton('Reset')
        _btn_reset.setFixedSize(*BUTTON_SIZE)
        _btn_reset.setFocusPolicy(Qt.NoFocus)
        _btn_reset.clicked.connect(partial(self._reset_plot, reset=True))
        hbox.addWidget(_btn_reset)
        vbox.addLayout(hbox)

        # A closure to change the button sytle once a boolean is linked
        def _changestyle():
            objname = str(id(self))
            _btn_reset.setObjectName(objname)
            sheet = ("QPushButton#{} {{ background-color : rgba{}; }}"
                     "".format(objname, ALL_OK_COLOR))
            _btn_reset.setStyleSheet(sheet)
        self._resetbox_linked = _changestyle

        self.add_proxy(self.proxy)
        return widget

    def add_proxy(self, proxy):
        binding = proxy.binding
        if self._reset_proxy is None and isinstance(binding, BoolBinding):
            # if the resetbox is set, new boolean box goes to yvalues
            self._reset_proxy = proxy
            self._resetbox_linked()
            return True

        if proxy in self._last_values:
            return False

        self._last_values[proxy] = None
        # Don't allow boolean type in x axis
        if self._x_proxy is None and not isinstance(binding, BoolBinding):
            self._x_proxy = proxy
            self._mpl_widget.axes_call('set_xlabel', axis_label(proxy))
        else:
            self._y_proxies.append(proxy)
            # request a new curve for each new ybox, use proxy.key for the
            # curve name as this is unique
            name = proxy.key
            self._mpl_widget.new_curve(label=name)
            self._y_values[name] = deque(maxlen=MAXNUMPOINTS)
            self._draw_start = True
        return True

    def value_update(self, proxy):
        value = proxy.value
        if proxy is self._reset_proxy:
            self._reset_plot(reset=value)
            return

        synchronizer = self._last_values
        synchronizer[proxy] = value
        if self._draw_start and None not in synchronizer.values():
            self._update_curve()

    def destroy_widget(self):
        """Ask MPL to clean up"""
        self._mpl_widget.destroy()

    # -------------------------------------------------------------------
    # private functions

    def _reset_plot(self, reset=True):
        """clear the plot"""
        if not reset:
            # when adding a boolean as resetbox, it may send a False value
            return

        self._last_values = {k: None for k in self._last_values.keys()}
        self._x_values.clear()
        for name, arr in self._y_values.items():
            arr.clear()
            self._mpl_widget.update_curve(self._x_values, arr, label=name)

    def _update_curve(self):
        """Update all curves, clear lastvalues buffer (set all values to None).
        This is called when xbox and all y boxes produce a not None value.
        """
        lastvalues = self._last_values
        for proxy, value in lastvalues.items():
            if proxy is self._x_proxy:
                self._x_values.append(value)
            else:
                self._y_values[proxy.key].append(value)

        # product pairs xvalues to each ('name', yvalues)
        for xdata, (label, ydata) in product((self._x_values,),
                                             self._y_values.items()):
            # later added y may have shorter length
            npx, npy = _convert_data(xdata, ydata)
            self._mpl_widget.update_curve(npx, npy, label=label)
        self._last_values = {k: None for k in lastvalues.keys()}


def _convert_data(xdata, ydata):
    """Convert x y deque array to numpy array for plotting.
    """
    # There might be np.nan in the array
    npx = np.array(xdata, dtype=np.float)
    npy = np.array(ydata, dtype=np.float)
    if npx.size != npy.size:
        # trim npx if necessary, len(npx) >= len(npy) always True
        npx = npx[-npy.size:]
    # get a mask to get rid of np.nan in y
    mask = np.isfinite(npy)
    return npx[mask], npy[mask]
