#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################

from PyQt4.QtGui import QAction, QInputDialog, QLabel
from PyQt4.QtCore import pyqtSlot, Qt
from traits.api import Instance, on_trait_change, Undefined

from karabo.common.scenemodel.api import DoubleWheelBoxModel
from karabogui import globals as krb_globals
from karabogui import messagebox
from karabogui.binding.api import (
    FloatBinding, get_editor_value, get_min_max)
from karabogui.controllers.api import (
    add_unit_label, BaseBindingController, DoubleWheelEdit,
    register_binding_controller)

from karabogui.util import SignalBlocker

MAX_FLOATING_PRECISION = 8
MAX_INTEGERS_PRECISION = 6


@register_binding_controller(ui_name='WheelBox Field', can_edit=True,
                             klassname='WheelBox',
                             binding_type=FloatBinding,
                             priority=-10, can_show_nothing=False)
class EditableWheelBox(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(DoubleWheelBoxModel, args=())
    _internal_widget = Instance(QLabel)

    def create_widget(self, parent):
        model = self.model
        self._internal_widget = DoubleWheelEdit(
            integers=model.integers, decimals=model.decimals, parent=parent)

        self._internal_widget.setFocusPolicy(Qt.StrongFocus)
        self._internal_widget.valueChanged.connect(self._on_value_changed)
        self._internal_widget.configurationChanged.connect(
            self._config_changed)
        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)
        decimal_action = QAction('Change number of decimals', widget)
        decimal_action.triggered.connect(self._pick_decimals)
        widget.addAction(decimal_action)
        integers_action = QAction('Change number of integers', widget)
        integers_action.triggered.connect(self._pick_integers)
        widget.addAction(integers_action)
        widget.setFocusProxy(self._internal_widget)

        return widget

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        self._internal_widget.set_value_limits(low, high)

    def value_update(self, proxy):
        """We are updated internally from the device
        """
        if proxy.value is Undefined:
            return
        value = get_editor_value(proxy)
        with SignalBlocker(self._internal_widget):
            # This value is validated against the widget boundaries. If they
            # do not match, the widget will adjust!
            self._internal_widget.set_value(value)

    @pyqtSlot(float)
    def _on_value_changed(self, value):
        """This widget uses for completion a plain QDoubleValidator

        We again validate the value coming from the WheelWidget
        """
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value
        with SignalBlocker(self._internal_widget):
            self._internal_widget.set_value_widget(value)

    def state_update(self, proxy):
        root_proxy = proxy.root_proxy
        value = root_proxy.state_binding.value
        if value is Undefined or not value:
            return

        binding = proxy.binding
        is_allowed = binding.is_allowed(value)
        is_accessible = (krb_globals.GLOBAL_ACCESS_LEVEL >=
                         binding.required_access_level)

        self._internal_widget.setEnabled(is_allowed and is_accessible)

    @on_trait_change('model.decimals', post_init=True)
    def _decimals_update(self):
        self._internal_widget.set_number_decimals(self.model.decimals)

    @pyqtSlot(int)
    def _config_changed(self, value):
        """The configuration of the wheel widget changed. Model update!"""
        self.model.integers = value

    @pyqtSlot()
    def _pick_decimals(self):
        num_decimals, ok = QInputDialog.getInt(
            self.widget, 'Decimal', 'Floating point precision:',
            self.model.decimals, 0, MAX_FLOATING_PRECISION)
        if ok:
            self.model.decimals = num_decimals

    @pyqtSlot()
    def _pick_integers(self):
        num_integers, ok = QInputDialog.getInt(
            self.widget, 'Integers', 'Integers:',
            self.model.integers, 1, MAX_INTEGERS_PRECISION)
        if ok:
            # Validate the integers if they are compliant with the value
            value_string = str(self._internal_widget.value)
            number = len(value_string.split('.')[0])
            if num_integers < number:
                messagebox.show_error("The integer setting is not compliant "
                                      "with the current value")
                return
            self.model.integers = num_integers
            self._internal_widget.set_number_integers(self.model.integers)
