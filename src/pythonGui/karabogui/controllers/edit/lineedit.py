#############################################################################
# Author: <kerstin.weger@xfel.eu>
# Created on February 10, 2012
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPalette, QValidator
from PyQt5.QtWidgets import QAction, QInputDialog, QLineEdit
from traits.api import Instance, Int, on_trait_change, Str

from karabo.common.api import KARABO_SCHEMA_REGEX
from karabo.common.scenemodel.api import (
    DoubleLineEditModel, EditableRegexModel, HexadecimalModel,
    IntLineEditModel)
from karabogui.binding.api import (
    get_editor_value, get_min_max, FloatBinding, IntBinding, StringBinding)
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, is_proxy_allowed,
    register_binding_controller)
from karabogui.util import SignalBlocker
from karabogui.validators import (
    HexValidator, IntValidator, NumberValidator, RegexValidator)
from karabogui.widgets.hints import LineEdit

MAX_FLOATING_PRECISION = 12


class BaseLineEdit(BaseBindingController):
    _validator = Instance(QValidator)
    _internal_widget = Instance(QLineEdit)
    _normal_palette = Instance(QPalette)
    _error_palette = Instance(QPalette)
    _display_value = Str('')
    _internal_value = Str('')
    _last_cursor_pos = Int(0)

    def create_widget(self, parent):
        self._internal_widget = LineEdit(parent)
        self._internal_widget.setValidator(self._validator)
        self._normal_palette = self._internal_widget.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)
        widget.setFocusProxy(self._internal_widget)
        return widget

    def set_read_only(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            self._internal_widget.textChanged.connect(self._on_text_changed)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self._internal_widget.setEnabled(enable)

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        self._validator.setBottom(low)
        self._validator.setTop(high)

    def _on_text_changed(self, text):
        acceptable_input = self._internal_widget.hasAcceptableInput()
        if self.proxy.binding is None:
            self._internal_widget.setPalette(self._normal_palette)
            return
        if acceptable_input:
            self._internal_value = text
            self._last_cursor_pos = self._internal_widget.cursorPosition()
            # proxy.edit_value is set to None if the user input is not valid
            self.proxy.edit_value = self._validate_value()
        else:
            # erase the edit value
            self.proxy.edit_value = None
        # update color after text change!
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self._internal_widget.setPalette(palette)

    def _validate_value(self):
        """This method validates the current value of the widget and returns
        on success the value or in failure None.
        """
        if not self._internal_value:
            return None

        value = self._internal_value
        state, _, _ = self._validator.validate(value, 0)
        if state == QValidator.Invalid:
            value = None
        return value

    def on_decline(self):
        """When the input was declined, this method is executed
        """
        # we know that after we are in valid range, hence we reset the
        # background
        self._internal_widget.setPalette(self._normal_palette)


@register_binding_controller(ui_name='Float Field', can_edit=True,
                             klassname='DoubleLineEdit',
                             binding_type=FloatBinding, priority=10)
class DoubleLineEdit(BaseLineEdit):
    # The scene model class used by this controller
    model = Instance(DoubleLineEditModel, args=())

    def create_widget(self, parent):
        self._validator = NumberValidator()
        self._validator.decimals = self.model.decimals
        widget = super(DoubleLineEdit, self).create_widget(parent)
        decimal_action = QAction('Change number of decimals', widget)
        decimal_action.triggered.connect(self._pick_decimals)
        widget.addAction(decimal_action)
        return widget

    def value_update(self, proxy):
        value = get_editor_value(proxy, '')
        self.widget.update_label(proxy)
        self._internal_value = str(value)

        format_str = ('{}' if self.model.decimals == -1
                      else '{{:.{}f}}'.format(self.model.decimals))
        with SignalBlocker(self._internal_widget):
            displayed = '' if value == '' else format_str.format(value)
            self._display_value = displayed
            self._internal_widget.setText(self._display_value)
        self._internal_widget.setCursorPosition(self._last_cursor_pos)

    @on_trait_change('model.decimals', post_init=True)
    def _decimals_update(self):
        self.value_update(self.proxy)
        self._validator.decimals = self.model.decimals

    def _pick_decimals(self, checked):
        num_decimals, ok = QInputDialog.getInt(
            self.widget, 'Decimal', 'Floating point precision:',
            self.model.decimals, -1, MAX_FLOATING_PRECISION)
        if ok:
            self.model.decimals = num_decimals

    def _validate_value(self):
        ret = super(DoubleLineEdit, self)._validate_value()
        return float(ret) if ret is not None else None


@register_binding_controller(ui_name='Integer Field', can_edit=True,
                             klassname='IntLineEdit', binding_type=IntBinding,
                             priority=10)
class IntLineEdit(BaseLineEdit):
    # The scene model class used by this controller
    model = Instance(IntLineEditModel, args=())

    def create_widget(self, parent):
        self._validator = IntValidator()
        return super(IntLineEdit, self).create_widget(parent)

    def value_update(self, proxy):
        value = get_editor_value(proxy, '')
        self.widget.update_label(proxy)
        self._internal_value = str(value)
        with SignalBlocker(self._internal_widget):
            self._display_value = "{}".format(value)
            self._internal_widget.setText(self._display_value)
        self._internal_widget.setCursorPosition(self._last_cursor_pos)

    def _validate_value(self):
        ret = super(IntLineEdit, self)._validate_value()
        return int(ret) if ret is not None else None


@register_binding_controller(ui_name='Hexadecimal', can_edit=True,
                             klassname='Hexadecimal', binding_type=IntBinding)
class Hexadecimal(BaseLineEdit):
    model = Instance(HexadecimalModel, args=())

    def create_widget(self, parent):
        self._validator = HexValidator()
        return super(Hexadecimal, self).create_widget(parent)

    def value_update(self, proxy):
        value = get_editor_value(proxy, '')
        if value == '':
            return
        self.widget.update_label(proxy)
        self._internal_value = str(value)
        with SignalBlocker(self._internal_widget):
            self._display_value = "{:x}".format(value)
            self._internal_widget.setText(self._display_value)
        self._internal_widget.setCursorPosition(self._last_cursor_pos)

    def _validate_value(self):
        ret = super(Hexadecimal, self)._validate_value()
        return int(ret, base=16) if ret is not None else None


def _is_regex_compatible(binding):
    return binding.attributes.get(KARABO_SCHEMA_REGEX, None) is not None


@register_binding_controller(ui_name='Regex Field', can_edit=True,
                             is_compatible=_is_regex_compatible,
                             klassname='RegexEdit', binding_type=StringBinding,
                             priority=90)
class EditRegex(BaseLineEdit):
    model = Instance(EditableRegexModel, args=())

    def create_widget(self, parent):
        self._validator = RegexValidator()
        return super(EditRegex, self).create_widget(parent)

    def binding_update(self, proxy):
        binding = proxy.binding
        regex = binding.attributes.get(KARABO_SCHEMA_REGEX, '')
        self._validator.setRegex(regex)

    def value_update(self, proxy):
        value = get_editor_value(proxy, '')
        self.widget.update_label(proxy)
        self._internal_value = str(value)
        with SignalBlocker(self._internal_widget):
            self._display_value = "{}".format(value)
            self._internal_widget.setText(self._display_value)
        self._internal_widget.setCursorPosition(self._last_cursor_pos)

    def _validate_value(self):
        value = self._internal_value
        state, _, _ = self._validator.validate(value, 0)
        if state == QValidator.Invalid:
            value = None
        return str(value) if value is not None else None
