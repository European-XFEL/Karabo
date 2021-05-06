from numpy import log10
from qtpy.QtCore import QLocale, Qt
from qtpy.QtWidgets import QAction, QDoubleSpinBox, QInputDialog
from traits.api import Instance, on_trait_change

from karabo.common.api import KARABO_SCHEMA_ABSOLUTE_ERROR
from karabo.common.scenemodel.api import FloatSpinBoxModel
from karabogui.binding.api import FloatBinding, get_editor_value, get_min_max
from karabogui.const import WIDGET_MIN_HEIGHT, WIDGET_MIN_WIDTH
from karabogui.controllers.api import (
    BaseBindingController, add_unit_label, is_proxy_allowed,
    register_binding_controller)
from karabogui.util import MouseWheelEventBlocker, SignalBlocker

LOCALE = QLocale('en_US')


@register_binding_controller(ui_name='Double SpinBox', can_edit=True,
                             klassname='FloatSpinBox',
                             binding_type=FloatBinding)
class FloatSpinBox(BaseBindingController):
    model = Instance(FloatSpinBoxModel, args=())

    _internal_widget = Instance(QDoubleSpinBox)
    _blocker = Instance(MouseWheelEventBlocker)

    def create_widget(self, parent):
        self._internal_widget = QDoubleSpinBox(parent)
        self._internal_widget.setLocale(LOCALE)
        self._internal_widget.setMinimumSize(WIDGET_MIN_WIDTH,
                                             WIDGET_MIN_HEIGHT)
        self._internal_widget.setDecimals(self.model.decimals)
        self._internal_widget.setSingleStep(self.model.step)
        self._internal_widget.setFocusPolicy(Qt.StrongFocus)
        self._internal_widget.valueChanged[float].connect(self._on_user_edit)
        self._blocker = MouseWheelEventBlocker(self._internal_widget)
        self._internal_widget.installEventFilter(self._blocker)

        widget = add_unit_label(self.proxy, self._internal_widget,
                                parent=parent)

        # add actions
        step_action = QAction('Change Step...', widget)
        step_action.triggered.connect(self._change_step)
        widget.addAction(step_action)

        decimal_action = QAction('Change Decimals...', widget)
        decimal_action.triggered.connect(self._change_decimals)
        widget.addAction(decimal_action)

        widget.setFocusProxy(self._internal_widget)

        return widget

    def binding_update(self, proxy):
        low, high = get_min_max(proxy.binding)
        self._internal_widget.setRange(low, high)

    def value_update(self, proxy):
        self.widget.update_label(proxy)
        value = get_editor_value(proxy)
        if value is not None:
            with SignalBlocker(self._internal_widget):
                self._internal_widget.setValue(value)

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)

    @on_trait_change('model:decimals')
    def _set_decimals(self, value):
        if self._internal_widget is not None:
            self._internal_widget.setDecimals(value)

    @on_trait_change('model:step')
    def _set_step(self, value):
        if self._internal_widget is not None:
            self._internal_widget.setSingleStep(value)

    def _change_step(self):
        step = self._internal_widget.singleStep()
        step, ok = QInputDialog.getDouble(
            self.widget, 'Single Step', 'Enter size of a single step', step)
        if ok:
            self.model.step = step

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

    def _on_user_edit(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value
