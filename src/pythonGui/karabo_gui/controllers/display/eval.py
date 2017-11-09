
import traceback

from PyQt4.QtCore import Qt
from PyQt4.QtGui import QAction, QInputDialog, QLineEdit
from traits.api import Callable, Dict, Instance, on_trait_change

from karabo.common.scenemodel.api import EvaluatorModel
from karabo_gui import messagebox
from karabo_gui.binding.api import (
    BaseBindingController, register_binding_controller,
    CharBinding, ComplexBinding, FloatBinding,
    Int8Binding, Int16Binding, Int32Binding, Int64Binding, StringBinding,
    Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding
)
from karabo_gui.const import WIDGET_MIN_HEIGHT
from karabo_gui.controllers.unitlabel import add_unit_label

BINDING_TYPES = (CharBinding, ComplexBinding, FloatBinding, StringBinding,
                 Int8Binding, Int16Binding, Int32Binding, Int64Binding,
                 Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding)


@register_binding_controller(ui_name='Evaluate Expression', read_only=True,
                             binding_type=BINDING_TYPES)
class Evaluator(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(EvaluatorModel)
    # Security holes (evaluation of values)
    globals_ns = Dict
    function = Callable
    # Storage for the main widget
    _internal_widget = Instance(QLineEdit)

    def _globals_ns_default(self):
        ns = {}
        exec('from numpy import *', ns)
        return ns

    def _function_default(self):
        return eval('lambda x: {}'.format(self.model.expression),
                    self.globals_ns)

    def create_widget(self, parent):
        line_edit = QLineEdit(parent)
        line_edit.setMinimumHeight(WIDGET_MIN_HEIGHT)
        line_edit.setReadOnly(True)
        line_edit.setFocusPolicy(Qt.NoFocus)
        self._internal_widget = line_edit
        widget = add_unit_label(self.proxy, line_edit, parent=parent)

        action = QAction('Change expression...', widget)
        widget.addAction(action)
        action.triggered.connect(self._change_expression)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.update_label(self.proxy)

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
            messagebox.show_warning(msg, title='Error in expression')
            return

        self.model.expression = text
        self._value_update(self.proxy.value)
