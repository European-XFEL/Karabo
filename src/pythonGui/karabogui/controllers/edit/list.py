import re
from ast import literal_eval

import numpy as np
from PyQt5.QtCore import Qt
from PyQt5.QtGui import QPalette, QValidator
from PyQt5.QtWidgets import (
    QDialog, QHBoxLayout, QLineEdit, QToolButton, QWidget)
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
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, is_proxy_allowed, register_binding_controller)
from karabogui.dialogs.listedit import ListEditDialog
from karabogui.util import SignalBlocker
from karabogui.widgets.hints import LineEdit


class _BaseListController(BaseBindingController):
    last_cursor_position = Int(0)
    _internal_widget = Instance(QLineEdit)
    _validator = Instance(QValidator)
    layout = Instance(QHBoxLayout)
    _normal_palette = Instance(QPalette)
    _error_palette = Instance(QPalette)

    def create_widget(self, parent):
        composite_widget = QWidget(parent)
        composite_widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.layout = QHBoxLayout(composite_widget)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self._internal_widget = LineEdit(parent)
        self.layout.addWidget(self._internal_widget)
        self._normal_palette = self._internal_widget.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        composite_widget.setFocusProxy(self._internal_widget)

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
        tbEdit.setMinimumSize(WIDGET_MIN_HEIGHT, WIDGET_MIN_HEIGHT)
        tbEdit.setMaximumSize(25, 25)
        tbEdit.setFocusPolicy(Qt.NoFocus)
        tbEdit.clicked.connect(self._on_edit_clicked)
        self.layout.addWidget(tbEdit)

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        self._set_edit_field_text(value)

    def state_update(self, proxy):
        # NOTE: Only the editable widget will be disabled depending on state!
        if self._internal_widget.isReadOnly():
            return

        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)

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

    def _set_edit_field_text(self, value):
        with SignalBlocker(self._internal_widget):
            self._internal_widget.setText(','.join(str(v) for v in value))
        self._internal_widget.setCursorPosition(self.last_cursor_position)

    def _on_edit_clicked(self):
        if self.proxy.binding is None:
            return

        list_edit = ListEditDialog(self.proxy, duplicates_ok=True,
                                   parent=self.widget)
        list_edit.set_texts("Add", "&Value", "Edit")
        if list_edit.exec_() == QDialog.Accepted:
            self.proxy.edit_value = list_edit.values
            self._set_edit_field_text(list_edit.values)

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


CAST_MAP = {
    VectorInt8Binding: np.int8,
    VectorInt16Binding: np.int16,
    VectorInt32Binding: np.int32,
    VectorInt64Binding: np.int64,
    VectorUint8Binding: np.uint8,
    VectorUint16Binding: np.uint16,
    VectorUint32Binding: np.uint32,
    VectorUint64Binding: np.uint64,
}


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
        bind_type = type(binding)
        self.pattern = re.compile(REGEX_MAP.get(bind_type, ''))
        self.intermediate = MEDIATE_MAP.get(bind_type, ())
        self.literal = (str if bind_type == VectorStringBinding
                        else literal_eval)
        # vector of integers are downcasted, here we get a function
        # to validate that the user input matches the casted value
        self.cast = CAST_MAP.get(bind_type, self.literal)
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
            # if we can cast it properly (SyntaxError)
            # Check for changes in between Vectors (ValueError)
            # or if self.cast casts down integers (AssertionError)
            try:
                assert self.literal(value) == self.cast(value)
            except (ValueError, SyntaxError, AssertionError):
                return self.Intermediate, input, pos

        return self.Acceptable, input, pos
