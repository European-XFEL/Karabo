from qtpy.QtCore import QSize, Qt, Signal
from qtpy.QtGui import QPainter
from qtpy.QtWidgets import QWidget
from traits.api import Instance

from karabo.common.scenemodel.api import BitfieldModel
from karabogui.binding.api import (
    Uint8Binding, Uint16Binding, Uint32Binding, Uint64Binding,
    UnsignedIntBinding, get_editor_value
)
from karabogui.controllers.api import (
    add_unit_label, BaseBindingController, register_binding_controller)


class BitfieldWidget(QWidget):
    valueChanged = Signal(int)

    def __init__(self, parent):
        QWidget.__init__(self, parent)
        self.size = 16
        self.value = 0
        self.readonly = False

    def sizeHint(self):
        return QSize(self.size * 5 // 4, 20)

    def mousePressEvent(self, event):
        if self.readonly:
            return

        w, h = self._getwh()
        bit = (event.pos().y() // h) + 4 * (event.pos().x() // w)
        self.value ^= 1 << bit
        self.valueChanged.emit(self.value)
        self.update()

    def _getwh(self):
        w = self.geometry().width() * 4 // self.size
        h = self.geometry().height() // 4
        return w, h

    def paintEvent(self, event):
        w, h = self._getwh()
        with QPainter(self) as p:
            p.fillRect(0, 0, w * self.size // 4 + 1, h * 4 + 1, Qt.gray)
            for pos in range(self.size):
                color = Qt.white if self.value & 1 << pos > 0 else Qt.black
                p.fillRect(pos // 4 * w + 1, pos % 4 * h + 1,
                           w - 1, h - 1, color)


@register_binding_controller(ui_name='Bit Field', klassname='Bitfield',
                             binding_type=UnsignedIntBinding, can_edit=True)
class Bitfield(BaseBindingController):
    model = Instance(BitfieldModel, args=())
    # Internal traits
    _internal_widget = Instance(BitfieldWidget)

    def binding_update(self, proxy):
        if self.widget is None:
            return
        self._internal_widget.size = self._get_binding_bitcount()
        self.widget.update()

    def create_widget(self, parent):
        self._internal_widget = BitfieldWidget(parent)
        self._internal_widget.size = self._get_binding_bitcount()
        return add_unit_label(self.proxy, self._internal_widget, parent=parent)

    def set_read_only(self, ro):
        self._internal_widget.readonly = ro
        if not ro:
            self._internal_widget.valueChanged.connect(self._on_user_edit)
        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)

    def value_update(self, proxy):
        self._internal_widget.value = int(get_editor_value(proxy))
        self.widget.update_label(proxy)
        self.widget.update()

    def _on_user_edit(self, value):
        if self.proxy.binding is None:
            return
        self.proxy.edit_value = value

    def _get_binding_bitcount(self):
        bytecount_map = {
            Uint8Binding: 1, Uint16Binding: 2, Uint32Binding: 4,
            Uint64Binding: 8
        }
        binding_type = type(self.proxy.binding)
        return bytecount_map.get(binding_type, 1) * 8
