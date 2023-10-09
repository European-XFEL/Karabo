# This file is part of the Karabo Gui.
#
# http://www.karabo.eu
#
# Copyright (C) European XFEL GmbH Schenefeld. All rights reserved.
#
# The Karabo Gui is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 3 or higher.
#
# You should have received a copy of the General Public License, version 3,
# along with the Karabo Gui.
# If not, see <https://www.gnu.org/licenses/gpl-3.0>.
#
# The Karabo Gui is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.
from ast import literal_eval

from qtpy.QtCore import Qt
from qtpy.QtGui import QPalette, QValidator
from qtpy.QtWidgets import (
    QDialog, QHBoxLayout, QLineEdit, QToolButton, QWidget)
from traits.api import Instance, Int

from karabo.common.api import KARABO_SCHEMA_REGEX
from karabo.common.scenemodel.api import (
    EditableListModel, EditableRegexListModel)
from karabogui import icons
from karabogui.binding.api import (
    VectorBinding, VectorCharBinding, VectorHashBinding, VectorNoneBinding,
    VectorStringBinding, get_editor_value, get_min_max_size)
from karabogui.const import WIDGET_MIN_HEIGHT
from karabogui.controllers.api import (
    BaseBindingController, ListValidator, is_proxy_allowed,
    register_binding_controller)
from karabogui.dialogs.listedit import ListEditDialog
from karabogui.util import SignalBlocker
from karabogui.validators import RegexListValidator
from karabogui.widgets.hints import LineEdit


class BaseEditableListController(BaseBindingController):
    _internal_widget = Instance(QLineEdit)
    last_cursor_position = Int(0)
    layout = Instance(QHBoxLayout)

    # other internal traits
    _validator = Instance(QValidator)
    _normal_palette = Instance(QPalette)
    _error_palette = Instance(QPalette)

    def create_widget(self, parent):
        widget = QWidget(parent)
        widget.setMinimumHeight(WIDGET_MIN_HEIGHT)
        self.layout = QHBoxLayout(widget)
        self.layout.setContentsMargins(0, 0, 0, 0)
        self._internal_widget = LineEdit(parent)
        self.layout.addWidget(self._internal_widget)
        self._normal_palette = self._internal_widget.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)
        widget.setFocusProxy(self._internal_widget)

        self._internal_widget.textChanged.connect(self._on_user_edit)
        self._internal_widget.setFocusPolicy(Qt.StrongFocus)

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

        return widget

    def binding_update(self, proxy):
        raise NotImplementedError

    def state_update(self, proxy):
        enable = is_proxy_allowed(proxy)
        self.widget.setEnabled(enable)

    def value_update(self, proxy):
        value = get_editor_value(proxy, [])
        self._set_edit_field_text(value)

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

        list_edit = ListEditDialog(self.proxy.binding, duplicates_ok=True,
                                   parent=self.widget)
        list_edit.set_list(get_editor_value(self.proxy, []))
        if list_edit.exec() == QDialog.Accepted:
            self.proxy.edit_value = list_edit.values
            with SignalBlocker(self._internal_widget):
                display = ','.join(str(v) for v in list_edit.values)
                self._internal_widget.setText(display)
            self._internal_widget.setCursorPosition(self.last_cursor_position)
        # Give back the focus!
        self._internal_widget.setFocus(Qt.PopupFocusReason)


def _is_compatible(binding):
    """Don't allow editing of goofy vectors"""
    prohibited = (VectorHashBinding, VectorNoneBinding, VectorCharBinding)
    return not isinstance(binding, prohibited)


@register_binding_controller(ui_name='Edit List', is_compatible=_is_compatible,
                             klassname='EditableList', can_edit=True,
                             binding_type=VectorBinding, priority=10)
class EditableList(BaseEditableListController):
    """The editable version of the list widget"""
    model = Instance(EditableListModel, args=())

    def binding_update(self, proxy):
        binding = proxy.binding
        self._validator = ListValidator(binding=binding, parent=self.widget)
        self._internal_widget.setValidator(self._validator)


def _has_regex_attribute(binding):
    attrs = binding.attributes
    return attrs.get(KARABO_SCHEMA_REGEX, None) is not None


@register_binding_controller(ui_name='Edit Regex List', can_edit=True,
                             is_compatible=_has_regex_attribute,
                             klassname='EditableRegexList',
                             binding_type=VectorStringBinding,
                             priority=10)
class EditableRegexList(BaseEditableListController):
    model = Instance(EditableRegexListModel, args=())

    def binding_update(self, proxy):
        binding = proxy.binding
        regex = binding.attributes.get(KARABO_SCHEMA_REGEX, "")
        minSize, maxSize = get_min_max_size(binding)
        self._validator = RegexListValidator(
            regex, minSize=minSize, maxSize=maxSize, parent=self.widget)
        self._internal_widget.setValidator(self._validator)
