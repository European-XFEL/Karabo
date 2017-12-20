import os.path as op

from PyQt4.QtCore import pyqtSlot
from PyQt4.QtGui import QMessageBox
from PyQt4.QtSvg import QSvgWidget
from traits.api import Instance

from karabo.common.scenemodel.api import PopUpModel
from karabo.middlelayer import Timestamp
from karabogui.binding.api import (
    PropertyProxy, SlotBinding, StringBinding, KARABO_SCHEMA_DISPLAYED_NAME)
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)


def _is_compatible(binding):
    # This controller must always have a StringBinding for its first proxy
    return isinstance(binding, StringBinding)


@register_binding_controller(ui_name='PopUp', klassname='PopUp',
                             binding_type=(SlotBinding, StringBinding),
                             is_compatible=_is_compatible)
class PopUp(BaseBindingController):
    # The scene model class used by this controller
    model = Instance(PopUpModel, args=())
    # Internal traits
    _dialog = Instance(QMessageBox)
    _ok = Instance(PropertyProxy)
    _cancel = Instance(PropertyProxy)
    _timestamp = Instance(Timestamp, args=())

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

    def binding_update(self, proxy):
        attrs = proxy.binding.attributes
        name = attrs.get(KARABO_SCHEMA_DISPLAYED_NAME)
        self._dialog.setWindowTitle(name)

    def create_widget(self, parent):
        background_svg = op.join(op.dirname(__file__), 'speech-balloon.svg')
        widget = QSvgWidget(background_svg, parent)
        self._dialog = QMessageBox(parent)
        self._dialog.setStandardButtons(QMessageBox.Ok)
        self._dialog.setModal(False)
        self._dialog.finished.connect(self._on_finished)
        return widget

    def value_update(self, proxy):
        value = proxy.value
        timestamp = proxy.binding.timestamp
        if timestamp != self._timestamp:
            self._dialog.setText(value)
            self._timestamp = timestamp
            self._dialog.setVisible(value != '')

    @pyqtSlot(object)
    def _on_finished(self, result):
        if result == QMessageBox.Ok and self._ok is not None:
            self._ok.execute()
        elif result == QMessageBox.Cancel and self._cancel is not None:
            self._cancel.execute()
