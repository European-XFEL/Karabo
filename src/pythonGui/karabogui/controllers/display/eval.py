import traceback

from PyQt5.QtCore import Qt
from PyQt5.QtWidgets import QAction, QFrame, QInputDialog, QLabel
from traits.api import Callable, Dict, Instance, Str, Tuple

from karabo.common.scenemodel.api import EvaluatorModel
from karabogui import messagebox
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, get_binding_value, IntBinding,
    StringBinding
)
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.indicators import (
    ALL_OK_COLOR, PROPERTY_ALARM_COLOR, PROPERTY_WARN_COLOR)
from karabo.common.api import (
    KARABO_ALARM_LOW, KARABO_ALARM_HIGH, KARABO_WARN_LOW, KARABO_WARN_HIGH)
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, register_binding_controller)
from karabogui.util import generateObjectName

BINDING_TYPES = (CharBinding, ComplexBinding, FloatBinding, StringBinding,
                 IntBinding)


@register_binding_controller(ui_name='Evaluate Expression',
                             klassname='Evaluator',
                             binding_type=BINDING_TYPES)
class Evaluator(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(EvaluatorModel, args=())
    # Security holes (evaluation of values)
    globals_ns = Dict
    function = Callable
    _bg_color = Tuple(ALL_OK_COLOR)
    _style_sheet = Str
    # Storage for the main widget
    _internal_widget = Instance(QLabel)

    def _globals_ns_default(self):
        ns = {}
        exec('from builtins import *', ns)
        exec('from numpy import *', ns)
        return ns

    def _function_default(self):
        return eval('lambda x: {}'.format(self.model.expression),
                    self.globals_ns)

    def create_widget(self, parent):
        self._internal_widget = QLabel(parent)
        self._internal_widget.setAlignment(Qt.AlignCenter)
        self._internal_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self._internal_widget.setWordWrap(True)
        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)

        action = QAction('Change expression...', widget)
        widget.addAction(action)
        action.triggered.connect(self._change_expression)

        objectName = generateObjectName(self)
        self._style_sheet = ("QWidget#{}".format(objectName) +
                             " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)
        sheet = self._style_sheet.format(ALL_OK_COLOR)
        widget.setStyleSheet(sheet)

        return widget

    def clear_widget(self):
        """Clear the internal widget when the device goes offline"""
        self._internal_widget.clear()

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if value is None:
            return

        binding = proxy.binding
        self._check_alarms(binding, value)

        # update unit label
        self.widget.update_label(proxy)
        try:
            disp_value = "{}".format(self.function(value))
        except Exception as e:
            disp_value = traceback.format_exception_only(type(e), e)[0]
        self._internal_widget.setText(disp_value)

    def _change_expression(self):
        text, ok = QInputDialog.getText(self.widget, 'Enter Expression',
                                        'f(x) = ', text=self.model.expression)
        if not ok:
            return

        try:
            self.function = eval('lambda x: {}'.format(text), self.globals_ns)
        except SyntaxError as e:
            err = traceback.format_exception_only(type(e), e)
            msg = '<pre>{1}{2}</pre>{3}'.format(*err)
            messagebox.show_warning(msg, title='Error in expression',
                                    parent=self.widget)
            return

        self.model.expression = text
        if get_binding_value(self.proxy) is not None:
            self.value_update(self.proxy)

    def _check_alarms(self, binding, value):
        attributes = binding.attributes
        alarm_low = attributes.get(KARABO_ALARM_LOW)
        alarm_high = attributes.get(KARABO_ALARM_HIGH)
        warn_low = attributes.get(KARABO_WARN_LOW)
        warn_high = attributes.get(KARABO_WARN_HIGH)
        if ((alarm_low is not None and value < alarm_low) or
                (alarm_high is not None and value > alarm_high)):
            self._bg_color = PROPERTY_ALARM_COLOR
        elif ((warn_low is not None and value < warn_low) or
                (warn_high is not None and value > warn_high)):
            self._bg_color = PROPERTY_WARN_COLOR
        else:
            self._bg_color = ALL_OK_COLOR
        sheet = self._style_sheet.format(self._bg_color)
        self.widget.setStyleSheet(sheet)
