#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 8, 2017
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
#############################################################################
from enum import Enum

from qtpy.QtCore import QEvent, QRect, QSize, Qt
from qtpy.QtGui import QBrush, QColor, QDoubleValidator, QPalette, QValidator
from qtpy.QtWidgets import (
    QAbstractItemDelegate, QApplication, QComboBox, QDialog, QHBoxLayout,
    QLineEdit, QStyle, QStyledItemDelegate, QStyleOptionButton, QWidget)

from karabo.common.api import State
from karabogui.binding.api import IntBinding, PropertyProxy, VectorHashBinding
from karabogui.controllers.api import get_compatible_controllers
from karabogui.indicators import STATE_COLORS

from .dialog.table_view import TableDialog
from .utils import (
    FIXED_ROW_HEIGHT, ButtonState, handle_default_state, set_fill_rect)

TABLE_BUTTON_TEXT = 'Table Element'
BUTTON_LABEL_PADDING = 5


def _get_table_button_rect(option):
    """Calculate the rectangle of the table edit button based on the given
    `QStyleOptionViewItem` instance
    """
    rect = QRect(option.rect)
    font_metrics = option.fontMetrics
    width = font_metrics.size(Qt.TextSingleLine, TABLE_BUTTON_TEXT).width()
    rect.setWidth(width + BUTTON_LABEL_PADDING * 2)
    return rect


class EditDelegate(QStyledItemDelegate):
    """A QStyledItemDelegate for configurator values
    """

    def __init__(self, parent=None):
        super().__init__(parent)
        self._button_states = {}
        self._selection_changed = False

    # ----------------------------------------------------------------------
    # Public interface

    def close_editor(self, editor, hint):
        """Workaround for shitty PyQt behavior.
        """
        self.update_model_data(editor.index, hint)

    def current_changed(self, index):
        """The view is telling us that the selection changed
        """
        self._selection_changed = True

    def update_model_data(self, index, hint):
        model = self.parent().model()
        if hint == QAbstractItemDelegate.SubmitModelCache:
            obj = model.index_ref(index)
            # XXX: some extra handling for VectorHashes since they are not
            # editable via an editor
            editable = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
            is_table = isinstance(getattr(obj, 'binding', None),
                                  VectorHashBinding)
            if not self._selection_changed or (editable and is_table):
                model.flush_index_modification(index)
        elif hint == QAbstractItemDelegate.RevertModelCache:
            model.clear_index_modification(index)

    # ----------------------------------------------------------------------
    # Qt interface

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        model = index.model()
        obj = model.index_ref(index)
        if isinstance(getattr(obj, 'binding', None), VectorHashBinding):
            return None

        self._selection_changed = False
        return EditWidgetWrapper(obj, index, parent=parent)

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        if editor.initialized:
            # Don't mess with editors which already received a value!
            # This avoids resetting cursor positions against the user's will.
            return

        model = index.model()
        obj = model.index_ref(index)
        if obj is None:
            return

        # Handle attribute editors specially
        if isinstance(obj, PropertyProxy):
            editor.controller.value_update(obj)
        else:
            editor.controller.value = index.data(Qt.EditRole)
        editor.initialized = True

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        column = index.column()
        if column != 2:
            return

        model = index.model()
        obj = model.index_ref(index)
        if obj is None:
            return

        if isinstance(obj, PropertyProxy):
            value = editor.controller.proxy.edit_value
        else:
            value = editor.controller.value
        # Set the data via the model
        model.setData(index, value, Qt.EditRole)

    def sizeHint(self, option, index):
        """Reimplemented function of QStyledItemDelegate.

        XXX: I don't like this, but it's not clear how I would access the
        editor widget for a row (_if_ it exists) to return the correct height
        here.
        """
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)

    def paint(self, painter, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        model = index.model()
        obj = model.index_ref(index)
        if not isinstance(getattr(obj, 'binding', None), VectorHashBinding):
            super().paint(painter, option, index)
            return

        set_fill_rect(painter, option, index)

        self._draw_button(painter, option, index, obj)

    def editorEvent(self, event, model, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        handled_types = (QEvent.MouseButtonPress, QEvent.MouseButtonRelease)
        if event.type() in handled_types:
            proxy = model.index_ref(index)
            if isinstance(getattr(proxy, 'binding', None), VectorHashBinding):
                self._handle_event_state(proxy, event, model, option, index)

        return super().editorEvent(
            event, model, option, index)

    # ----------------------------------------------------------------------
    # Private interface

    def _draw_button(self, painter, option, index, proxy):
        """Draw a table button"""
        key = proxy.key
        state = self._button_states.get(key, ButtonState.DISABLED)
        # always allow table button click!
        button_state = handle_default_state(True, state)
        self._button_states[key] = state
        button = QStyleOptionButton()
        if proxy.edit_value is not None:
            palette = QPalette(button.palette)
            color = QColor(*STATE_COLORS[State.CHANGING])
            color.setAlpha(128)
            palette.setBrush(QPalette.Button, QBrush(color))
            button.palette = palette

        button.state = button_state
        button.rect = _get_table_button_rect(option)
        button.text = TABLE_BUTTON_TEXT
        button.features = QStyleOptionButton.AutoDefaultButton
        QApplication.style().drawControl(QStyle.CE_PushButton, button, painter)

    def _handle_event_state(self, proxy, event, model, option, index):
        """Determine the state of a given box's button during user interaction.
        """
        key = proxy.key
        state = self._button_states.get(key, ButtonState.DISABLED)
        allowed = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
        rect = _get_table_button_rect(option)
        if rect.contains(event.pos()):
            if event.type() == QEvent.MouseButtonPress:
                state = ButtonState.PRESSED
            elif state == ButtonState.PRESSED:
                dialog = TableDialog(proxy, allowed, parent=self.parent())
                result = dialog.exec()
                # Only for editable table elements we do actions!
                if allowed:
                    if result == QDialog.Accepted:
                        # XXX: Note that the dialog is passed as an editor
                        # Ensure that the dialog has a member called
                        # `controller` to enable fetching the data of it
                        self.setModelData(dialog, model, index)
                    else:
                        proxy.revert_edit()
        if (state == ButtonState.PRESSED and
                event.type() == QEvent.MouseButtonRelease):
            state = ButtonState.ENABLED
        self._button_states[key] = state
        return state


class EditWidgetWrapper(QWidget):
    """A QWidget container which can be returned as an item's editor."""

    def __init__(self, obj, index, parent=None):
        super().__init__(parent)
        self.setAutoFillBackground(True)

        assert isinstance(obj, PropertyProxy)
        klass = get_compatible_controllers(obj.binding, can_edit=True)[0]
        self.controller = klass(proxy=obj)
        self.controller.create(self)
        # Enable editing for the widget
        self.controller.set_read_only(False)
        self.setFocusProxy(self.controller.widget)

        # Introduce layout to have some border to show
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.controller.widget)

        # Keep a model index reference for use when the editor is closed by Qt
        self.index = index
        self.initialized = False


# -----------------------------------------------------------------------------
# Attribute editors aren't typical BaseBindingControllers

class EnumAttributeEditor:
    """An editor for attribute values defined by an Enum class."""
    # Subclasses should define this
    enum_klass = Enum

    def __init__(self, binding, parent=None):
        instMsg = "Don't instantiate EnumAttributeEditor directly!"
        assert self.enum_klass is not Enum, instMsg

        self.widget = QComboBox(parent)
        self.widget.setFrame(False)
        self._populate_widget()

    @property
    def value(self):
        return self.widget.itemData(self.widget.currentIndex())

    @value.setter
    def value(self, value):
        try:
            enum_value = self.enum_klass(value)
        except ValueError:
            return
        index = self.widget.findData(enum_value.value)
        if index >= 0:
            self.widget.setCurrentIndex(index)

    def _populate_widget(self):
        def _get_item_text(enum_val):
            # Normalize the ENUM_VALUE_NAME -> Enum value name
            name = enum_val.name.replace('_', ' ').lower().capitalize()
            return f"{enum_val.value} ({name})"

        for e in self.enum_klass:
            text = _get_item_text(e)
            self.widget.addItem(text, e.value)


class IntValidator(QValidator):
    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


class NumberAttributeEditor:
    """An editor for numerical attribute values
    """

    def __init__(self, binding, parent=None):
        self.widget = QLineEdit(parent)

        if isinstance(binding, IntBinding):
            self.validator = IntValidator(self.widget)
            self._value_cast = int
        else:
            self.validator = QDoubleValidator(self.widget)
            self._value_cast = float

        self.widget.setValidator(self.validator)
        self.widget.textChanged.connect(self._on_text_changed)
        self._normal_palette = self.widget.palette()
        self._error_palette = QPalette(self._normal_palette)
        self._error_palette.setColor(QPalette.Text, Qt.red)

    @property
    def value(self):
        return self._value_cast(self._validate_value())

    @value.setter
    def value(self, value):
        if value is None:
            value = 0
        self.widget.setText(f"{value}")

    def _on_text_changed(self, text):
        self.widget.setPalette(self._normal_palette
                               if self.widget.hasAcceptableInput()
                               else self._error_palette)

    def _validate_value(self):
        if not self.widget.text():
            return 0

        value = self.widget.text()
        state, _, _ = self.validator.validate(value, 0)
        if state == QValidator.Invalid or state == QValidator.Intermediate:
            value = 0
        return value
