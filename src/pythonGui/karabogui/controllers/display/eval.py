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
import traceback

from qtpy.QtCore import Qt
from qtpy.QtWidgets import QAction, QDialog, QFrame, QInputDialog, QLabel
from traits.api import Callable, Dict, Instance, Str, Tuple

from karabo.common.scenemodel.api import EvaluatorModel
from karabogui import messagebox
from karabogui.binding.api import (
    CharBinding, ComplexBinding, FloatBinding, IntBinding, StringBinding,
    get_binding_value)
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, register_binding_controller)
from karabogui.dialogs.format_label import FormatLabelDialog
from karabogui.fonts import get_font_size_from_dpi
from karabogui.indicators import ALL_OK_COLOR
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
        return eval(f'lambda x: {self.model.expression}',
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
        self._style_sheet = (f"QWidget#{objectName}" +
                             " {{ background-color : rgba{}; }}")
        widget.setObjectName(objectName)
        sheet = self._style_sheet.format(ALL_OK_COLOR)
        widget.setStyleSheet(sheet)

        # Add an action for formatting options
        format_action = QAction("Format field..", widget)
        format_action.triggered.connect(self._format_field)
        widget.addAction(format_action)
        self._apply_format(widget)

        return widget

    def clear_widget(self):
        """Clear the internal widget when the device goes offline"""
        self._internal_widget.clear()

    def binding_update(self, proxy):
        self.widget.update_unit_label(proxy)

    def value_update(self, proxy):
        value = get_binding_value(proxy)
        if value is None:
            return

        try:
            disp_value = str(self.function(value))
        except Exception as e:
            disp_value = traceback.format_exception_only(type(e), e)[0]
        self._internal_widget.setText(disp_value)

    def _change_expression(self):
        text, ok = QInputDialog.getText(self.widget, 'Enter Expression',
                                        'f(x) = ', text=self.model.expression)
        if not ok:
            return

        try:
            self.function = eval(f'lambda x: {text}', self.globals_ns)
        except Exception:
            msg = f'Error in expression: {traceback.format_exc()}'
            messagebox.show_warning(msg, title='Error in expression',
                                    parent=self.widget)
            return

        self.model.expression = text
        if get_binding_value(self.proxy) is not None:
            self.value_update(self.proxy)

    # -----------------------------------------------------------------------
    # Formatting methods

    def _format_field(self):
        dialog = FormatLabelDialog(font_size=self.model.font_size,
                                   font_weight=self.model.font_weight,
                                   parent=self.widget)
        if dialog.exec() == QDialog.Accepted:
            self.model.trait_set(font_size=dialog.font_size,
                                 font_weight=dialog.font_weight)
            self._apply_format()

    def _apply_format(self, widget=None):
        """The widget is passed as an argument in create_widget as it is not
           yet bound to self.widget then"""
        if widget is None:
            widget = self.widget

        # Apply font formatting
        font = widget.font()
        font.setPointSize(get_font_size_from_dpi(self.model.font_size))
        font.setBold(self.model.font_weight == "bold")
        widget.setFont(font)
