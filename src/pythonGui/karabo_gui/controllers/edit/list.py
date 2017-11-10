from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import QDialog, QHBoxLayout, QLineEdit, QToolButton, QWidget
from traits.api import Instance, Int, on_trait_change

from karabo.common.scenemodel.api import EditableListModel
from karabo_gui import icons
from karabo_gui.binding.api import (
    BaseBindingController, VectorBoolBinding, VectorCharBinding,
    VectorComplexDoubleBinding,
    VectorComplexFloatBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
    VectorInt64Binding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding,
    register_binding_controller
)
from karabo_gui.listedit import ListEdit
from karabo_gui.util import SignalBlocker

BINDING_TYPES = (
    VectorBoolBinding, VectorCharBinding, VectorComplexDoubleBinding,
    VectorComplexFloatBinding, VectorDoubleBinding, VectorFloatBinding,
    VectorInt8Binding, VectorInt16Binding, VectorInt32Binding,
    VectorInt64Binding, VectorStringBinding, VectorUint8Binding,
    VectorUint16Binding, VectorUint32Binding, VectorUint64Binding)


# XXX: priority = 10
@register_binding_controller(ui_name='List', binding_type=BINDING_TYPES)
class EditableList(BaseBindingController):
    model = Instance(EditableListModel)
    last_cursor_position = Int(0)
    _internal_widget = Instance(QLineEdit)
    layout = Instance(QHBoxLayout)

    def create_widget(self, parent):
        composite_widget = QWidget(parent)
        self.layout = QHBoxLayout(composite_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)

        self._internal_widget = QLineEdit()

        self.layout.addWidget(self._internal_widget)

        return composite_widget

    def set_read_only(self, ro):
        self._internal_widget.setReadOnly(ro)
        self._internal_widget.setFocusPolicy(Qt.NoFocus if ro else Qt.StrongFocus)
        if ro:
            return
        self._internal_widget.textChanged.connect(self._on_user_edit)
        text = "Edit"
        tbEdit = QToolButton()
        tbEdit.setStatusTip(text)
        tbEdit.setToolTip(text)
        tbEdit.setIcon(icons.edit)
        tbEdit.setMaximumSize(25, 25)
        tbEdit.setFocusPolicy(Qt.NoFocus)
        tbEdit.clicked.connect(self._on_edit_clicked)
        self.layout.addWidget(tbEdit)

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setText(','.join(str(v) for v in value))
        self._internal_widget.setCursorPosition(self.last_cursor_position)

    @pyqtSlot(str)
    def _on_user_edit(self, text):
        self.last_cursor_position = self._internal_widget.cursorPosition()
        if text:
            # don't need to convert here 'cause the traits does it foh ya.
            self.proxy.value = [val for val in text.split(',') if val != '']
        else:
            self.proxy.value = []

    @pyqtSlot()
    def _on_edit_clicked(self):
        list_edit = ListEdit(self.proxy, True)
        list_edit.set_texts("Add", "&Value", "Edit")

        if list_edit.exec_() == QDialog.Accepted:
            self.proxy.value = list_edit.values
