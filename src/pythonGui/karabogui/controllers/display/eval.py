import traceback

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QFrame, QInputDialog, QLabel
from traits.api import Callable, Dict, Instance

from karabo.common.scenemodel.api import EvaluatorModel
from karabogui import messagebox
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, get_binding_value, IntBinding,
    StringBinding
)
from karabogui.const import WIDGET_MIN_HEIGHT, FINE_COLOR
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
        label = QLabel(parent)
        label.setMinimumHeight(WIDGET_MIN_HEIGHT)
        label.setFocusPolicy(Qt.NoFocus)
        self._internal_widget = label
        widget = add_unit_label(self.proxy, label, parent=parent)
        widget.setFrameStyle(QFrame.Box | QFrame.Plain)

        action = QAction('Change expression...', widget)
        widget.addAction(action)
        action.triggered.connect(self._change_expression)

        objectName = generateObjectName(self)
        widget.setObjectName(objectName)
        style_sheet = ("QWidget#{}".format(objectName) +
                       " {{ background-color : rgba{}; }}")
        widget.setStyleSheet(style_sheet.format(FINE_COLOR))
        return widget

    def value_update(self, proxy):
        # update unit label
        self.widget.update_label(proxy)

        try:
            disp_value = "{}".format(self.function(proxy.value))
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
            messagebox.show_warning(msg, title='Error in expression')
            return

        self.model.expression = text
        if get_binding_value(self.proxy) is not None:
            self.value_update(self.proxy)
