#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt, pyqtSlot
from PyQt4.QtGui import (QAction, QInputDialog, QLineEdit, QDoubleValidator,
                         QPalette, QValidator)
from traits.api import Instance, Str, on_trait_change

from karabo.common.scenemodel.api import DoubleLineEditModel, IntLineEditModel
from karabogui.binding.api import (
    BaseBindingController, register_binding_controller, get_min_max,
    FloatBinding, IntBinding
)
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.util import SignalBlocker

MAX_FLOATING_PRECISION = 12


class NumberLineEdit(BaseBindingController):
    _validator = Instance(QValidator)
    _internal_widget = Instance(QLineEdit)
    _normal_palette = Instance(QPalette)
    _error_palette = Instance(QPalette)
    _display_value = Str('')
    _internal_value = Str('')

    def create_widget(self, parent):
        self._internal_widget = QLineEdit(parent)
        self._internal_widget.setAlignment(Qt.AlignLeft | Qt.AlignAbsolute)
        self._internal_widget.setValidator(self._validator)
        self._normal_palette = self._internal_widget.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        return add_unit_label(self.proxy, self._internal_widget, parent=parent)

    def set_read_only(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.textChanged.connect(self._on_text_changed)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    def _widget_changed(self):
        """Finish intialization of the widget"""
        binding = self.proxy.binding
        if binding is not None:
            self._binding_update(binding)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        low, high = get_min_max(binding)
        self._validator.setBottom(low)
        self._validator.setTop(high)

    @pyqtSlot(str)
    def _on_text_changed(self, text):
        acceptable_input = self._internal_widget.hasAcceptableInput()
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self._internal_widget.setPalette(palette)

        if acceptable_input:
            if isinstance(self._validator, QDoubleValidator):
                intdci = text.split('.')
                if len(intdci) > 1:
                    part2 = intdci[1]
                    if self._internal_value:
                        tail = self._internal_value.split('.')[1]
                        if len(tail) > len(part2):
                            part2 += tail[len(part2):]
                else:
                    part2 = ''
                self._internal_value = '.'.join([intdci[0], part2])
                self._display_value = text
            else:
                self._internal_value = text
            self.proxy.value = self._validate_value()

    def _validate_value(self):
        """This method validates the current value of the widget and returns
        on sucess the value or in failure 0.
        """
        if not self._internal_value:
            return 0

        value = self._internal_value
        state, _, _ = self._validator.validate(value, 0)
        if state == QValidator.Invalid or state == QValidator.Intermediate:
            value = 0
        return value


class IntValidator(QValidator):
    def __init__(self, min=None, max=None, parent=None):
        QValidator.__init__(self, parent)
        self.min = min
        self.max = max

    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        if self.min is not None and self.min >= 0 and input.startswith('-'):
            return self.Invalid, input, pos

        if self.max is not None and self.max < 0 and input.startswith('+'):
            return self.Invalid, input, pos

        if ((self.min is None or self.min <= int(input)) and
                (self.max is None or int(input) <= self.max)):
            return self.Acceptable, input, pos
        else:
            return self.Intermediate, input, pos

    def setBottom(self, min):
        self.min = min

    def setTop(self, max):
        self.max = max


# XXX: priority = 10
@register_binding_controller(ui_name='Float Field', binding_type=FloatBinding)
class DoubleLineEdit(NumberLineEdit):
    # The scene model class used by this controller
    model = Instance(DoubleLineEditModel)

    def create_widget(self, parent):
        self._validator = QDoubleValidator(None)
        widget = super(DoubleLineEdit, self).create_widget(parent)

        decimal_action = QAction('Change number of decimals', widget)
        decimal_action.triggered.connect(self._pick_decimals)
        widget.addAction(decimal_action)
        return widget

    @on_trait_change('proxy:value,model.decimals')
    def _value_update(self, value):
        if self.widget is None:
            return

        self.widget.update_label(self.proxy)
        self._internal_value = str(value)

        format_str = ('{}' if self.model.decimals == -1
                      else '{{:.{}f}}'.format(self.model.decimals))
        with SignalBlocker(self._internal_widget):
            self._display_value = format_str.format(value)
            self._internal_widget.setText(self._display_value)

    @pyqtSlot()
    def _pick_decimals(self):
        num_decimals, ok = QInputDialog.getInt(
            self.widget, 'Decimal', 'Floating point precision:',
            self.model.decimals, -1, MAX_FLOATING_PRECISION)
        if ok:
            self.model.decimals = num_decimals

    def _validate_value(self):
        return float(super(DoubleLineEdit, self)._validate_value())


# XXX: priority = 10
@register_binding_controller(ui_name='Integer Field', binding_type=IntBinding)
class IntLineEdit(NumberLineEdit):
    # The scene model class used by this controller
    model = Instance(IntLineEditModel)

    def create_widget(self, parent):
        self._validator = IntValidator()
        return super(IntLineEdit, self).create_widget(parent)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.update_label(self.proxy)
        self._internal_value = str(value)
        with SignalBlocker(self._internal_widget):
            self._display_value = "{}".format(value)
            self._internal_widget.setText(self._display_value)

    def _validate_value(self):
        return int(super(IntLineEdit, self)._validate_value())
