from PyQt4.QtGui import QDoubleSpinBox

from karabogui.binding.api import IntBinding, FloatBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)


# ui_name => shown in the widget selection context menu
# binding_type => What types of data can this widget handle
@register_binding_controller(ui_name='My cool widget',
                             klassname='MyDisplay',
                             binding_type=(IntBinding, FloatBinding))
class MyDisplayWidget(BaseBindingController):
    def create_widget(self, parent):
        return QDoubleSpinBox(parent)

    def binding_update(self, proxy):
        # Do some initialization for a new value type
        pass

    def value_update(self, proxy):
        # Typically something like:
        self.widget.setValue(proxy.value)
