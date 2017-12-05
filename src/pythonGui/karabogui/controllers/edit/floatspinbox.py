from numpy import log10
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QAction, QDoubleSpinBox, QInputDialog
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import FloatSpinBoxModel
from karabogui.binding.api import (
    FloatBinding, get_editor_value, get_min_max, KARABO_SCHEMA_ABSOLUTE_ERROR)
from karabogui.const import WIDGET_MIN_HEIGHT, WIDGET_MIN_WIDTH
from karabogui.controllers.base import BaseBindingController
from karabogui.controllers.registry import register_binding_controller
from karabogui.controllers.unitlabel import add_unit_label
from karabogui.util import SignalBlocker


@register_binding_controller(ui_name='SpinBox (real)', can_edit=True,
                             klassname='FloatSpinBox',
                             binding_type=FloatBinding)
class FloatSpinBox(BaseBindingController):
    # The scene model class for this controller
    model = Instance(FloatSpinBoxModel)
    # Internal traits
    _internal_widget = Instance(QDoubleSpinBox)

    def create_widget(self, parent):
        self._internal_widget = QDoubleSpinBox(parent)
        self._internal_widget.setMinimumSize(WIDGET_MIN_WIDTH,
                                             WIDGET_MIN_HEIGHT)
        self._internal_widget.setDecimals(self.model.decimals)
        self._internal_widget.setSingleStep(self.model.step)

        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)

        # add actions
        step_action = QAction('Change Step...', widget)
        step_action.triggered.connect(self._change_step)
        widget.addAction(step_action)
        decimal_action = QAction('Change Decimals...', widget)
        decimal_action.triggered.connect(self._change_decimals)
        widget.addAction(decimal_action)
        return widget

    def set_read_only(self, ro):
        self._internal_widget.setReadOnly(ro)
        if not ro:
            widget = self._internal_widget
            widget.valueChanged[float].connect(self._on_user_edit)

        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        self._internal_widget.setRange(low, high)

    def value_update(self, proxy):
        self.widget.update_label(proxy)
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setValue(get_editor_value(proxy))

    @on_trait_change('model:decimals')
    def _set_decimals(self, value):
        if self._internal_widget is not None:
            self._internal_widget.setDecimals(value)

    @on_trait_change('model:step')
    def _set_step(self, value):
        if self._internal_widget is not None:
            self._internal_widget.setSingleStep(value)

    @pyqtSlot()
    def _change_step(self):
        step = self._internal_widget.singleStep()
        step, ok = QInputDialog.getDouble(
            self.widget, 'Single Step', 'Enter size of a single step', step)
        if ok:
            self.model.step = step

    @pyqtSlot()
    def _change_decimals(self):
        # Override the starting value with something from the binding
        decimals = self.model.decimals
        binding = self.proxy.binding
        if binding:
            abs_error = binding.attributes.get(KARABO_SCHEMA_ABSOLUTE_ERROR)
            if abs_error is not None and abs_error < 1:
                decimals = -log10(abs_error)

        # Then ask the user
        decimals, ok = QInputDialog.getInt(
            self.widget, 'Decimals', 'Enter number of decimals',
            value=decimals, min=0, max=15)
        if ok:
            self.model.decimals = decimals

    @pyqtSlot(float)
    def _on_user_edit(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value
