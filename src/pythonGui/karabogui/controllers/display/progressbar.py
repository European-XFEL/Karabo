#############################################################################
# Author: <alessandro.silenzi@xfel.eu>
# Created on October 26, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QProgressBar
from traits.api import Bool, Float, Instance, Tuple

from karabo.common.api import (
    KARABO_SCHEMA_MAX_EXC, KARABO_SCHEMA_MAX_INC, KARABO_SCHEMA_MIN_EXC,
    KARABO_SCHEMA_MIN_INC)
from karabo.common.scenemodel.api import DisplayProgressBarModel
from karabogui import messagebox
from karabogui.binding.api import FloatBinding, IntBinding
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)

NULL_RANGE = (0, 0)
PROGRESS_MAX = 200


def _orientation(is_vertical):
    return Qt.Vertical if is_vertical else Qt.Horizontal


def _scale(val, min_val, max_val):
    clamped = min(max_val, max(min_val, val))
    return int((clamped - min_val) / (max_val - min_val) * PROGRESS_MAX)


@register_binding_controller(ui_name='Progress Bar',
                             klassname='DisplayProgressBar',
                             binding_type=(FloatBinding, IntBinding),
                             can_show_nothing=False)
class DisplayProgressBar(BaseBindingController):
    # The specific scene model class used by this widget
    model = Instance(DisplayProgressBarModel, args=())
    # Value scaling params
    _error_shown = Bool(False)
    _value_factors = Tuple(Float(-1), Float(-1))

    def create_widget(self, parent):
        widget = QProgressBar(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        orient_action = QAction("Change Orientation", widget)
        orient_action.triggered.connect(self._orientation_action)
        widget.addAction(orient_action)
        widget.setOrientation(_orientation(self.model.is_vertical))
        # Use a fixed integer range. Minimum defaults to 0.
        widget.setMaximum(PROGRESS_MAX)
        # update boolean context menu
        orient_action.setCheckable(True)
        orient_action.setChecked(self.model.is_vertical)

        return widget

    def binding_update(self, proxy):
        binding = proxy.binding
        self._eval_limits(binding)
        if self._value_factors == NULL_RANGE:
            self.error_box(proxy.key)
        else:
            self._error_shown = False

    def error_box(self, keyname):
        if not self._error_shown:
            # make sure you bother the user only once per widget
            msg = ('No proper configuration detected.\n'
                   'Please define min and max thresholds for '
                   '{}.'.format(keyname))
            messagebox.show_warning(msg, title='Wrong property configuration')
            self._error_shown = True

    def value_update(self, proxy):
        if self._value_factors == NULL_RANGE:
            return
        value = _scale(proxy.value, *self._value_factors)
        self.widget.setValue(value)

    def _eval_limits(self, binding):
        def _set_limits(low, high):
            """Avoid unset widget traits"""
            if self.widget is None:
                return
            self.widget.setMinimum(low)
            self.widget.setMaximum(high)

        attrs = binding.attributes
        min_val = attrs.get(KARABO_SCHEMA_MIN_INC,
                            attrs.get(KARABO_SCHEMA_MIN_EXC, 0))
        max_val = attrs.get(KARABO_SCHEMA_MAX_INC,
                            attrs.get(KARABO_SCHEMA_MAX_EXC, 0))

        self._value_factors = (min_val, max_val)
        if self._value_factors == NULL_RANGE:
            _set_limits(0, 0)
        else:
            _set_limits(0, PROGRESS_MAX)

    @pyqtSlot()
    def _orientation_action(self):
        self._set_orientation(not self.model.is_vertical)

    def _set_orientation(self, is_vertical):
        self.model.is_vertical = is_vertical
        self.widget.setOrientation(_orientation(is_vertical))
