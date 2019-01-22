import re
from ast import literal_eval

from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QDialog, QHBoxLayout, QLineEdit, QPalette, QToolButton,
    QValidator, QWidget)
from traits.api import Instance, Int

from karabo.common.scenemodel.api import DisplayListModel, EditableListModel
from karabogui import icons
from karabogui.binding.api import (
    get_editor_value, get_min_max_size, VectorBinding, VectorCharBinding,
    VectorHashBinding, VectorNoneBinding, VectorBoolBinding,
    VectorComplexDoubleBinding, VectorComplexFloatBinding,
    VectorDoubleBinding, VectorFloatBinding, VectorInt8Binding,
    VectorInt16Binding, VectorInt32Binding, VectorInt64Binding,
    VectorStringBinding, VectorUint8Binding, VectorUint16Binding,
    VectorUint32Binding, VectorUint64Binding)

from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller)
from karabogui.controllers.listedit import ListEdit
from karabogui.util import SignalBlocker


class _BaseListController(BaseBindingController):
    last_cursor_position = Int(0)
    _internal_widget = Instance(QLineEdit)
    _validator = Instance(QValidator)
    layout = Instance(QHBoxLayout)
    _normal_palette = Instance(QPalette)
    _error_palette = Instance(QPalette)

    def create_widget(self, parent):
        composite_widget = QWidget(parent)
        self.layout = QHBoxLayout(composite_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self._internal_widget = QLineEdit()
        self.layout.addWidget(self._internal_widget)
        self._normal_palette = self._internal_widget.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)

        return composite_widget

    def binding_update(self, proxy):
        binding = proxy.binding
        min_size, max_size = get_min_max_size(proxy.binding)
        self._validator = ListValidator(binding=binding)
        self._validator.min_size = min_size
        self._validator.max_size = max_size
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

        acceptable_input = self._internal_widget.hasAcceptableInput()
        self.last_cursor_position = self._internal_widget.cursorPosition()
        if acceptable_input:
            cast = (str if isinstance(self.proxy.binding, VectorStringBinding)
                    else literal_eval)
            self.proxy.edit_value = [cast(val)
                                     for val in text.split(',') if val != '']
        else:
            # erase edit value!
            self.proxy.edit_value = None
        # update color after text change!
        palette = (self._normal_palette if acceptable_input
                   else self._error_palette)
        self._internal_widget.setPalette(palette)

    def on_decline(self):
        if self._internal_widget.palette() == self._error_palette:
            self._internal_widget.setPalette(self._normal_palette)

    def _validate_empty(self):
        """This method validates an empty value on the list
        """
        value = []
        state, _, _ = self._validator.validate(value, 0)
        if state == QValidator.Invalid:
            value = None
        return value

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


INT_REGEX = r"^[-+]?\d+$"
UINT_REGEX = r"^[+]?\d+$"
DOUBLE_REGEX = r"^[-+]?\d*[\.]\d*$|^[-+]?\d+$"


REGEX_MAP = {
    VectorBoolBinding: r"(0|1|[T]rue|[F]alse)",
    VectorComplexDoubleBinding: DOUBLE_REGEX,  # XXX
    VectorComplexFloatBinding: DOUBLE_REGEX,  # XXX
    VectorDoubleBinding: DOUBLE_REGEX,
    VectorFloatBinding: DOUBLE_REGEX,
    VectorInt8Binding: INT_REGEX,
    VectorInt16Binding: INT_REGEX,
    VectorInt32Binding: INT_REGEX,
    VectorInt64Binding: INT_REGEX,
    VectorStringBinding: r"^.+$",
    VectorUint8Binding: UINT_REGEX,
    VectorUint16Binding: UINT_REGEX,
    VectorUint32Binding: UINT_REGEX,
    VectorUint64Binding: UINT_REGEX,
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

    def __init__(self, binding=None, min_size=None, max_size=None):
        super(ListValidator, self).__init__()
        self.pattern = re.compile(REGEX_MAP.get(type(binding), ''))
        self.intermediate = MEDIATE_MAP.get(type(binding), ())
        self.cast = (str if isinstance(binding, VectorStringBinding)
                     else literal_eval)
        self.min_size = min_size
        self.max_size = max_size

    def validate(self, input, pos):
        """The main validate function
        """
        # completely empty input might be acceptable!
        if input in ('', []) and (self.min_size is None or self.min_size == 0):
            return self.Acceptable, input, pos
        elif (input in ('', []) and self.min_size is not None
                and self.min_size > 0):
            return self.Intermediate, input, pos
        elif input.startswith(','):
            return self.Intermediate, input, pos

        # check for size first
        values = [val for val in input.split(',')]
        if self.min_size is not None and len(values) < self.min_size:
            return self.Intermediate, input, pos
        if self.max_size is not None and len(values) > self.max_size:
            return self.Intermediate, input, pos

        # intermediate positions
        if input[-1] in self.intermediate:
            return self.Intermediate, input, pos

        # check every single list value if it is valid
        for value in values:
            if not self.pattern.match(value):
                return self.Invalid, input, pos
            # We have to check other behavior, e.g. leading zeros, and see
            # if we can cast it properly
            try:
                self.cast(value)
            except SyntaxError:
                return self.Intermediate, input, pos

        return self.Acceptable, input, pos
