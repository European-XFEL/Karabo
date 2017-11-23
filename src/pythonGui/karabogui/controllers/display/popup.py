import os.path as op

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QMessageBox
from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import PopUpModel
from karabogui.binding.api import (
    BaseBindingController, PropertyProxy, SlotBinding, StringBinding,
    register_binding_controller, KARABO_SCHEMA_DISPLAYED_NAME
)


def _is_compatible(binding):
    # This controller must always have a StringBinding for its first proxy
    return isinstance(binding, StringBinding)


@register_binding_controller(ui_name='PopUp', read_only=True,
                             binding_type=(SlotBinding, StringBinding),
                             is_compatible=_is_compatible)
class PopUp(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(PopUpModel)
    # Internal traits
    _dialog = Instance(QMessageBox)
    _ok = Instance(PropertyProxy)
    _cancel = Instance(PropertyProxy)

    def add_proxy(self, proxy):
        if not isinstance(proxy.binding, SlotBinding):
            return False

        elif self._ok is None:
            self._ok = proxy
            return True

        elif self._cancel is None:
            self._cancel = proxy
            buttons = QMessageBox.Ok | QMessageBox.Cancel
            self._dialog.setStandardButtons(buttons)
            return True

        return False

    def create_widget(self, parent):
        background_svg = op.join(op.dirname(__file__), 'speech-balloon.svg')
        widget = QSvgWidget(background_svg, parent)
        self._dialog = QMessageBox(parent)
        self._dialog.setStandardButtons(QMessageBox.Ok)
        self._dialog.setModal(False)
        self._dialog.finished.connect(self._on_finished)
        return widget

    def _widget_changed(self):
        """Finish widget initialization when the binding is alive"""
        binding = self.proxy.binding
        if binding is not None:
            self._binding_update(binding)

    @on_trait_change('proxy:binding')
    def _binding_update(self, binding):
        name = binding.attributes.get(KARABO_SCHEMA_DISPLAYED_NAME)
        self._dialog.setWindowTitle(name)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self._dialog.setText(value)
        self._dialog.setVisible(value != '')

    @pyqtSlot(object)
    def _on_finished(self, result):
        if result == QMessageBox.Ok and self._ok is not None:
            self._ok.execute()
        elif result == QMessageBox.Cancel and self._cancel is not None:
            self._cancel.execute()
