#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 8, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from PyQt4.QtCore import QEvent, QRect, QSize, Qt, pyqtSlot
from PyQt4.QtGui import (
    QApplication, QAbstractItemDelegate, QComboBox, QDialog, QDoubleValidator,
    QHBoxLayout, QLineEdit, QPalette, QStyle, QStyleOptionButton,
    QStyledItemDelegate, QValidator, QWidget
)

from karabo.middlelayer import MetricPrefix, Unit
from karabogui.binding.api import IntBinding, PropertyProxy, VectorHashBinding
from karabogui.controllers.registry import get_compatible_controllers
from .dialog.table_edit import TableEditDialog
from .utils import (
    ButtonState, handle_default_state, set_fill_rect,
    EDITABLE_ATTRIBUTES, FIXED_ROW_HEIGHT)

TABLE_BUTTON_TEXT = 'Edit Table'
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
        super(EditDelegate, self).__init__(parent)
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
            super(EditDelegate, self).paint(painter, option, index)
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

        return super(EditDelegate, self).editorEvent(
            event, model, option, index)

    # ----------------------------------------------------------------------
    # Private interface

    def _draw_button(self, painter, option, index, proxy):
        """Draw a button
        """
        key = proxy.key
        state = self._button_states.get(key, ButtonState.DISABLED)
        allowed = index.flags() & Qt.ItemIsEditable == Qt.ItemIsEditable
        button_state = handle_default_state(allowed, state)
        self._button_states[key] = state
        button = QStyleOptionButton()
        button.state = button_state.value
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
        if allowed:
            rect = _get_table_button_rect(option)
            if rect.contains(event.pos()):
                if event.type() == QEvent.MouseButtonPress:
                    state = ButtonState.PRESSED
                elif state == ButtonState.PRESSED:
                    dialog = TableEditDialog(proxy)
                    result = dialog.exec()
                    if result == QDialog.Accepted:
                        # XXX: Note that the dialog is passed as an editor here
                        # Ensure that the dialog has a member called
                        # `controller` to enable fetching the data of it
                        self.setModelData(dialog, model, index)
            if (state == ButtonState.PRESSED and
                    event.type() == QEvent.MouseButtonRelease):
                state = ButtonState.ENABLED
        self._button_states[key] = state
        return state


class EditWidgetWrapper(QWidget):
    """A QWidget container which can be returned as an item's editor."""
    def __init__(self, obj, index, parent=None):
        super(EditWidgetWrapper, self).__init__(parent)
        self.setAutoFillBackground(True)

        if isinstance(obj, PropertyProxy):
            klass = get_compatible_controllers(obj.binding, can_edit=True)[0]
            self.controller = klass(proxy=obj)
            self.controller.create(self)
            # Enable editing for the widget
            self.controller.set_read_only(False)
        else:
            # The data of the 0th column is the attribute name!
            row = index.row()
            model = index.model()
            index = model.index(row, 0, parent=index.parent())
            attr_name = index.data(Qt.DisplayRole)
            klass = _ATTR_EDITOR_FACTORIES[attr_name]
            self.controller = klass(obj, parent=self)

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

class EnumAttributeEditor(object):
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
            return "{} ({})".format(enum_val.value, name)

        for e in self.enum_klass:
            text = _get_item_text(e)
            self.widget.addItem(text, e.value)


class UnitAttributeEditor(EnumAttributeEditor):
    """An EnumAttributeEditor for Unit values."""
    enum_klass = Unit


class MetricPrefixAttributeEditor(EnumAttributeEditor):
    """An EnumAttributeEditor for MetricPrefix values."""
    enum_klass = MetricPrefix


class IntValidator(QValidator):
    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


class NumberAttributeEditor(object):
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
        self.widget.setText("{}".format(value))

    @pyqtSlot(str)
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


_ATTR_EDITOR_FACTORIES = {name: NumberAttributeEditor
                          for name in EDITABLE_ATTRIBUTES}
_ATTR_EDITOR_FACTORIES['metricPrefixSymbol'] = MetricPrefixAttributeEditor
_ATTR_EDITOR_FACTORIES['unitSymbol'] = UnitAttributeEditor
