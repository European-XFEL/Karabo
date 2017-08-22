#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on August 8, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from enum import Enum

from PyQt4.QtCore import pyqtSlot, QSize, Qt
from PyQt4.QtGui import (
    QComboBox, QDoubleValidator, QHBoxLayout, QLineEdit, QPalette,
    QStyledItemDelegate, QValidator, QWidget)

from karabo.middlelayer import Integer, MetricPrefix, Unit
from karabo_gui.attributeediting.api import EDITABLE_ATTRIBUTE_NAMES
from karabo_gui.schema import EditableAttributeInfo, VectorHashCellInfo
from karabo_gui.widget import EditableWidget
from .utils import get_attribute_data, get_box_value, get_vector_col_value

FIXED_ROW_HEIGHT = 30


class ValueDelegate(QStyledItemDelegate):
    """A QStyledItemDelegate for configurator values
    """
    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        model = index.model()
        obj = model.index_ref(index)
        if obj is None:
            return None

        return EditWidgetWrapper(obj, index, parent=parent)

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        model = index.model()
        obj = model.index_ref(index)
        if obj is None:
            return

        if isinstance(obj, EditableAttributeInfo):
            _, _, value = get_attribute_data(obj, index.row())
        elif isinstance(obj, VectorHashCellInfo):
            value = get_vector_col_value(obj, is_edit_col=(index.column() == 2))
        else:
            value = get_box_value(obj, is_edit_col=(index.column() == 2))

        editor.editable_widget.valueChanged(obj, value)

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate.
        """
        column = index.column()
        # Set the data via the model
        model = index.model()
        if column == 2:
            model.setData(index, editor.editable_widget.value, Qt.EditRole)

    def sizeHint(self, option, index):
        """Reimplemented function of QStyledItemDelegate.

        XXX: I don't like this, but it's not clear how I would access the
        editor widget for a row (_if_ it exists) to return the correct height
        here.
        """
        return QSize(option.rect.width(), FIXED_ROW_HEIGHT)


class EditWidgetWrapper(QWidget):
    """A QWidget container which can be returned as an item's editor.
    """
    def __init__(self, obj, index, parent=None):
        super(EditWidgetWrapper, self).__init__(parent)
        self.setAutoFillBackground(True)

        if isinstance(obj, EditableAttributeInfo):
            name = obj.names[index.row()]
            klass = _ATTR_EDITOR_FACTORIES[name]
            box = obj.parent()
            self.editable_widget = klass(box, parent=self)
        else:
            klass = EditableWidget.getClass(obj)
            self.editable_widget = klass(obj, self)
            # XXX: Enable editing for that widget - this should be revisited
            # since all widgets are editable now
            self.editable_widget.setReadOnly(False)
            self.setFocusProxy(self.editable_widget.widget)

        # Introduce layout to have some border to show
        layout = QHBoxLayout(self)
        layout.setContentsMargins(2, 2, 2, 2)
        layout.addWidget(self.editable_widget.widget)


# -----------------------------------------------------------------------------
# Attribute editors stolen from karabo_gui.attributeediting (then modified)

class EnumAttributeEditor(object):
    """An editor for attribute values defined by an Enum class.
    """
    # Subclasses should define this
    enumClass = Enum

    def __init__(self, box, parent=None):
        instMsg = "Don't instantiate EnumAttributeEditor directly!"
        assert self.enumClass is not Enum, instMsg

        self.widget = QComboBox(parent)
        self.widget.setFrame(False)

        self._populateWidget()

    @property
    def value(self):
        return self.widget.itemData(self.widget.currentIndex())

    def valueChanged(self, attr_info, value):
        if value is None:
            return

        try:
            enum_value = self.enumClass(value)
        except ValueError:
            return

        index = self.widget.findData(enum_value.value)
        if index >= 0:
            self.widget.setCurrentIndex(index)

    def _populateWidget(self):
        def _get_item_text(enum_val):
            # Normalize the ENUM_VALUE_NAME -> Enum value name
            name = enum_val.name.replace('_', ' ').lower().capitalize()
            return "{} ({})".format(enum_val.value, name)

        for e in self.enumClass:
            text = _get_item_text(e)
            self.widget.addItem(text, e.value)


class UnitAttributeEditor(EnumAttributeEditor):
    """An EnumAttributeEditor for Unit values.
    """
    enumClass = Unit


class MetricPrefixAttributeEditor(EnumAttributeEditor):
    """An EnumAttributeEditor for MetricPrefix values.
    """
    enumClass = MetricPrefix


class IntValidator(QValidator):
    def __init__(self, parent=None):
        super(IntValidator, self).__init__(parent)

    def validate(self, input, pos):
        if input in ('+', '-', ''):
            return self.Intermediate, input, pos

        if not (input.isdigit() or input[0] in '+-' and input[1:].isdigit()):
            return self.Invalid, input, pos

        return self.Acceptable, input, pos


class NumberAttributeEditor(object):
    """An editor for numerical attribute values
    """
    def __init__(self, box, parent=None):
        self.widget = QLineEdit(parent)

        if isinstance(box.descriptor, Integer):
            self.validator = IntValidator(parent=self.widget)
            self._value_cast = int
        else:
            self.validator = QDoubleValidator(parent=self.widget)
            self._value_cast = float

        self.widget.setValidator(self.validator)
        self.widget.textChanged.connect(self.onTextChanged)

        self.normalPalette = self.widget.palette()
        self.errorPalette = QPalette(self.normalPalette)
        self.errorPalette.setColor(QPalette.Text, Qt.red)

    @pyqtSlot(str)
    def onTextChanged(self, text):
        self.widget.setPalette(self.normalPalette
                               if self.widget.hasAcceptableInput()
                               else self.errorPalette)

    def valueChanged(self, attr_info, value):
        if value is None:
            value = 0
        self.widget.setText("{}".format(value))

    def validate_value(self):
        if not self.widget.text():
            return 0

        value = self.widget.text()
        state, _, _ = self.validator.validate(value, 0)
        if state == QValidator.Invalid or state == QValidator.Intermediate:
            value = 0
        return value

    @property
    def value(self):
        return self._value_cast(self.validate_value())


_ATTR_EDITOR_FACTORIES = {name: NumberAttributeEditor
                          for name in EDITABLE_ATTRIBUTE_NAMES}
_ATTR_EDITOR_FACTORIES['metricPrefixSymbol'] = MetricPrefixAttributeEditor
_ATTR_EDITOR_FACTORIES['unitSymbol'] = UnitAttributeEditor
