import re
from ast import literal_eval

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QDialog, QHBoxLayout, QLineEdit, QToolButton, QValidator, QWidget)
from traits.api import Instance, Int

from karabo.common.scenemodel.api import DisplayListModel, EditableListModel
from karabogui import icons
from karabogui.binding.api import (
    VectorBinding, VectorCharBinding, VectorHashBinding, VectorNoneBinding,
    VectorBoolBinding, VectorComplexDoubleBinding, VectorComplexFloatBinding,
    VectorDoubleBinding, VectorFloatBinding, VectorInt8Binding,
    VectorInt16Binding, VectorInt32Binding, VectorInt64Binding,
    VectorStringBinding, VectorUint8Binding, VectorUint16Binding,
    VectorUint32Binding, VectorUint64Binding, get_editor_value)

from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.listedit import ListEdit
from karabogui.util import SignalBlocker


class _BaseListController(BaseBindingController):
    last_cursor_position = Int(0)
    _internal_widget = Instance(QLineEdit)
    _validator = Instance(QValidator)
    layout = Instance(QHBoxLayout)

    def create_widget(self, parent):
        composite_widget = QWidget(parent)
        self.layout = QHBoxLayout(composite_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)

        self._internal_widget = QLineEdit()
        self.layout.addWidget(self._internal_widget)

        return composite_widget

    def binding_update(self, proxy):
        binding = proxy.binding
        self._validator = ListValidator(binding=binding)
        self._internal_widget.setValidator(self._validator)

    def set_read_only(self, ro):
        focus_policy = Qt.NoFocus if ro else Qt.StrongFocus
        self._internal_widget.setFocusPolicy(focus_policy)
        self._internal_widget.setReadOnly(ro)
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

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setText(','.join(str(v) for v in value))
        self._internal_widget.setCursorPosition(self.last_cursor_position)

    @pyqtSlot(str)
    def _on_user_edit(self, text):
        if self.proxy.binding is None:
            return

        if not text:
            self.proxy.edit_value = []
            return

        acceptable_input = self._internal_widget.hasAcceptableInput()
        self.last_cursor_position = self._internal_widget.cursorPosition()
        if acceptable_input:
            cast = (str if isinstance(self.proxy.binding, VectorStringBinding)
                    else literal_eval)
            self.proxy.edit_value = [cast(val)
                                     for val in text.split(',') if val != '']

    @pyqtSlot()
    def _on_edit_clicked(self):
        if self.proxy.binding is None:
            return

        list_edit = ListEdit(self.proxy, True)
        list_edit.set_texts("Add", "&Value", "Edit")
        if list_edit.exec_() == QDialog.Accepted:
            self.proxy.edit_value = list_edit.values

        # Give back the focus!
        self._internal_widget.setFocus(Qt.PopupFocusReason)


def _is_compatible(binding):
    """Don't allow editing of goofy vectors"""
    prohibited = (VectorHashBinding, VectorNoneBinding, VectorCharBinding)
    return not isinstance(binding, prohibited)


@register_binding_controller(ui_name='Edit List', is_compatible=_is_compatible,
                             klassname='EditableList', can_edit=True,
                             binding_type=VectorBinding, priority=10)
class EditableList(_BaseListController):
    """The editable version of the list widget"""
    model = Instance(EditableListModel, args=())


@register_binding_controller(ui_name='List', is_compatible=_is_compatible,
                             klassname='DisplayList', priority=10,
                             binding_type=VectorBinding)
class DisplayList(_BaseListController):
    """The display version of the list widget"""
    model = Instance(DisplayListModel, args=())


REGEX_MAP = {
    VectorBoolBinding: r"(0|1|[T]rue|[F]alse)",
    VectorComplexDoubleBinding: r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$",  # X
    VectorComplexFloatBinding: r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$",  # X
    VectorDoubleBinding: r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$",
    VectorFloatBinding: r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$",
    VectorInt8Binding: r"^[-+]?\d+$",
    VectorInt16Binding: r"^[-+]?\d+$",
    VectorInt32Binding: r"^[-+]?\d+$",
    VectorInt64Binding: r"^[-+]?\d+$",
    VectorStringBinding: r"^.+$",
    VectorUint8Binding: r"^[+]?\d+$",
    VectorUint16Binding: r"^[+]?\d+$",
    VectorUint32Binding: r"^[+]?\d+$",
    VectorUint64Binding: r"^[+]?\d+$",
}

MEDIATE_MAP = {
    VectorBoolBinding: ('T', 't', 'r', 'u', 'F', 'a', 'l', 's', ','),
    VectorComplexDoubleBinding: ('+', '-', 'j', ','),  # X
    VectorComplexFloatBinding: ('+', '-', 'j', ','),  # X
    VectorDoubleBinding: ('+', '-', ','),
    VectorFloatBinding: ('+', '-', ',', '.'),
    VectorInt8Binding: ('+', '-', ','),
    VectorInt16Binding: ('+', '-', ','),
    VectorInt32Binding: ('+', '-', ','),
    VectorInt64Binding: ('+', '-', ','),
    VectorStringBinding: (','),
    VectorUint8Binding: ('+', ','),
    VectorUint16Binding: ('+', ','),
    VectorUint32Binding: ('+', ','),
    VectorUint64Binding: ('+', ','),
}


class ListValidator(QValidator):
    pattern = None
    intermediate = ()

    def __init__(self, binding=None):
        super(ListValidator, self).__init__()
        self.pattern = re.compile(REGEX_MAP.get(type(binding), ''))
        self.intermediate = MEDIATE_MAP.get(type(binding), ())

    def validate(self, input, pos):
        """The main validate function
        """
        # completely empty input is acceptable!
        if input == '':
            return self.Acceptable, input, pos

        # intermediate positions
        if input[-1] in self.intermediate:
            return self.Intermediate, input, pos

        values = [val for val in input.split(',')]
        for value in values:
            if not self.pattern.match(value):
                return self.Invalid, input, pos

        return self.Acceptable, input, pos
