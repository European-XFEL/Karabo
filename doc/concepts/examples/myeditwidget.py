from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QDoubleSpinBox

from karabogui.binding.api import IntBinding, FloatBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.util import SignalBlocker


# ui_name => shown in the widget selection context menu
# can_edit => True if the controller can work as an editor
# binding_type => What types of data can this widget handle
@register_binding_controller(ui_name='My cool widget', can_edit=True,
                             klassname='MyDisplay',
                             binding_type=(IntBinding, FloatBinding))
class MyDisplayWidget(BaseBindingController):
    def create_widget(self, parent):
        widget = QDoubleSpinBox(parent)
        widget.valueChanged.connect(self._on_value_edited)
        return widget

    def binding_update(self, proxy):
        # Do some initialization for a new value type
        pass

    def value_update(self, proxy):
        # Typically something like:
        with SignalBlocker(self.widget):
            self.widget.setValue(proxy.value)

    @pyqtSlot(float)
    def _on_value_edited(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value
