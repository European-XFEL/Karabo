#############################################################################
# Author: <dennis.goeries@xfel.eu>
# Created on March 6, 2021
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
import webbrowser

from qtpy.QtCore import Qt, Slot
from qtpy.QtWidgets import (
    QApplication, QComboBox, QMessageBox, QStyle, QStyledItemDelegate,
    QStyleOptionFrame, QStyleOptionProgressBar)

from karabo.common.api import KARABO_SCHEMA_OPTIONS
from karabo.native import Hash, is_equal
from karabogui import messagebox
from karabogui.binding.api import (
    BoolBinding, FloatBinding, IntBinding, StringBinding, VectorBinding,
    VectorBoolBinding, get_default_value, get_min_max)
from karabogui.controllers.validators import (
    BindingValidator as GenericValidator, ListValidator, SimpleValidator)
from karabogui.logger import get_logger
from karabogui.request import (
    call_device_slot, get_scene_from_server, retrieve_default_scene)
from karabogui.topology.api import is_device_online
from karabogui.util import SignalBlocker, get_reason_parts
from karabogui.widgets.edits import LineEditEditor

from .button_delegate import TableButtonDelegate
from .utils import (
    create_brushes, get_button_attributes, has_confirmation, parse_table_link)
from .widgets import TableVectorEdit


def get_display_delegate(proxy, binding, parent, read_only=True):
    """Retrieve a display (readonly) delegate for the column with binding

    :param proxy: The property proxy of the table element
    :param binding: The column binding
    :param parent: The parent widget
    :param read_only: If only read_only display delegates are considered
    """
    displayType = binding.displayType.split("|")[0]
    if not displayType:
        return None
    if (displayType == "TableBoolButton" and read_only and
            isinstance(binding, BoolBinding)):
        return BoolButtonDelegate(proxy, binding, parent)
    elif (displayType == "TableStringButton" and read_only and
          isinstance(binding, StringBinding)):
        return StringButtonDelegate(proxy, binding, parent)
    elif (displayType == "TableProgressBar"
          and isinstance(binding, (FloatBinding, IntBinding))):
        return ProgressBarDelegate(binding, parent)
    elif displayType == "TableColor":
        if isinstance(binding, (FloatBinding, IntBinding)):
            return ColorNumberDelegate(binding, parent)
        elif isinstance(binding, StringBinding):
            return ColorBindingDelegate(binding, parent)


def get_table_delegate(proxy, binding, parent):
    """Retrieve a (writable) table delegate for the column with binding

    :param proxy: The property proxy of the table element
    :param binding: The column binding
    :param parent: The parent widget
    """
    # For a writable table we might find other delegates allowed
    delegate = get_display_delegate(proxy, binding, parent,
                                    read_only=False)
    if delegate is not None:
        return delegate

    options = binding.attributes.get(KARABO_SCHEMA_OPTIONS, None)
    if options is not None:
        return ComboBoxDelegate(options, parent)
    elif isinstance(binding, (FloatBinding, IntBinding)):
        return NumberDelegate(binding, parent)
    elif isinstance(binding, VectorBinding):
        if (binding.displayType.split("|")[0] == "TableVector"
                and not isinstance(binding, VectorBoolBinding)):
            return VectorButtonDelegate(binding, parent)
        return VectorDelegate(binding, parent)
    else:
        return BindingDelegate(binding, parent)


class VectorButtonDelegate(QStyledItemDelegate):

    def __init__(self, binding, parent=None):
        super().__init__(parent)
        self.binding = binding

    def createEditor(self, parent, option, index):
        editor = TableVectorEdit(self.binding, parent=parent)
        values = index.model().data(index, Qt.DisplayRole)
        old = _create_string_list(values)
        validator = VectorValidator(self.binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate"""
        values = index.model().data(index, Qt.DisplayRole)
        editor.setText(values)

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)


class BoolButtonDelegate(TableButtonDelegate):
    def __init__(self, proxy, binding, parent=None):
        super().__init__(parent)
        self.text = binding.displayedName or proxy.path
        self.deviceId = proxy.root_proxy.device_id
        self.path = proxy.path
        self.confirm = has_confirmation(binding.displayType)

    def isEnabled(self, index=None):
        """Reimplemented function of TableButtonDelegate"""
        enabled = index.data(role=Qt.CheckStateRole) == Qt.Checked
        return enabled

    def get_button_text(self, index):
        """Reimplemented function of TableButtonDelegate"""
        return self.text

    def click_action(self, index):
        """Reimplemented function of TableButtonDelegate"""
        if not index.isValid() or not is_device_online(self.deviceId):
            return

        if self.confirm:
            message_box = QMessageBox()
            message_box.setModal(False)
            confirm = message_box.question(
                self.parent(), "Confirmation",
                f"Do you really want to proceed with <b>{self.text}</b>?",
                QMessageBox.No, QMessageBox.Yes)
            if confirm == QMessageBox.No:
                return

        model = index.model()
        row = index.row()
        column = index.column()
        header = model.get_header_key(column)

        column_count = model.columnCount()
        h = Hash(model.get_model_data(row, col) for col in range(column_count))
        data = Hash("rowData", h, "row", row, "column", column,
                    "header", header)

        def request_handler(success, reply):
            if not success:
                reason, details = get_reason_parts(reply)
                messagebox.show_error("Request for table action for device "
                                      f"<b>{self.deviceId}</b> failed."
                                      "The reason is:<br>"
                                      f"<i>{reason}</i>", details=details,
                                      parent=self.parent())
            elif not reply.get("payload.success", True):
                reason = reply.get("payload.reason", "")
                messagebox.show_error(
                    "Request for table action for device "
                    f"<b>{self.deviceId}</b> reported an error "
                    f"with the reason: {reason}.", parent=self.parent())
            else:
                get_logger().info("Successfully executed action "
                                  f"<b>{self.text}</b> for device "
                                  f"<b>{self.deviceId}</b>.")

        call_device_slot(request_handler, self.deviceId,
                         "requestAction", action="TableButton",
                         path=self.path, table=data)


class StringButtonDelegate(TableButtonDelegate):
    def __init__(self, proxy, binding, parent=None):
        super().__init__(parent)
        self.text = binding.displayedName or proxy.path
        self.deviceId = proxy.root_proxy.device_id
        self.attributes = get_button_attributes(binding.displayType)

    def get_button_text(self, index):
        """Reimplemented function of TableButtonDelegate"""
        return self.text

    def click_action(self, index):
        """Reimplemented function of TableButtonDelegate"""
        if not index.isValid() or not is_device_online(self.deviceId):
            return

        model = index.model()
        _, data = model.get_model_data(index.row(), index.column())

        action, options = parse_table_link(data)
        if action is None:
            get_logger().debug(
                "Invalid scheme provided for table string button")
        elif action == "deviceScene":
            try:
                deviceId = options["device_id"]
                name = options.get("name")
                if name is None:
                    retrieve_default_scene(deviceId)
                else:
                    get_scene_from_server(deviceId, name)
            except Exception:
                get_logger().warning(
                    f"Invalid deviceScene information: {data}.")
        elif action == "url":
            try:
                webbrowser.open_new(options)
            except Exception:
                get_logger().error(f"No valid url specified {options}")


class ComboBoxDelegate(QStyledItemDelegate):
    def __init__(self, options, parent=None):
        super().__init__(parent)
        # Note: We make sure that the options are a list of strings. For simple
        # types in Karabo they are a coercing array on the binding
        self._options = [str(value) for value in options]

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = QComboBox(parent)
        editor.addItems(self._options)
        editor.currentIndexChanged.connect(self._on_editor_changed)
        return editor

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate"""
        selection = index.model().data(index, Qt.DisplayRole)
        if selection in self._options:
            selection_index = self._options.index(selection)
            with SignalBlocker(editor):
                editor.setCurrentIndex(selection_index)
        else:
            with SignalBlocker(editor):
                editor.setCurrentIndex(-1)
            get_logger().error(
                f"The value {selection} is not in the following "
                f"options: {self._options}")

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        new = self._options[editor.currentIndex()]
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)

    @Slot()
    def _on_editor_changed(self):
        """The current index of the combobox changed. Notify the model.

        This signal MUST be emitted when the editor widget has completed
        editing the data, and wants to write it back into the model.

        XXX: This is in principle a wrong implementation, as it should be
        only emitted when the editor finished.
        """
        self.commitData.emit(self.sender())


class NumberDelegate(QStyledItemDelegate):
    def __init__(self, binding, parent=None):
        super().__init__(parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = LineEditEditor(parent)
        old = index.model().data(index, Qt.DisplayRole)
        validator = NumberValidator(self._binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate

        We only recreate the table if we have changes in our value! We can do
        this since the delegate will call `setModelData` once its out of focus.
        """
        old = index.model().data(index, Qt.DisplayRole)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)
            self.commitData.emit(self.sender())


class ProgressBarDelegate(NumberDelegate):

    def __init__(self, binding, parent=None):
        super().__init__(binding, parent)
        self.min_value, self.max_value = get_min_max(binding)
        self.has_limits = (self.min_value is not None
                           and self.max_value is not None)
        self.editing = {}

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate."""
        key = (index.row(), index.column())
        self.editing[key] = True
        return super().createEditor(parent, option, index)

    def destroyEditor(self, editor, index):
        """Reimplemented function of QStyledItemDelegate."""
        key = (index.row(), index.column())
        self.editing.pop(key, None)
        super().destroyEditor(editor, index)

    def paint(self, painter, option, index):
        """Reimplemented function of QStyledItemDelegate."""
        key = (index.row(), index.column())
        if self.editing.get(key):
            super().paint(painter, option, index)
            return

        progress_bar = QStyleOptionProgressBar()
        progress_bar.rect = option.rect
        if self.has_limits:
            progress_bar.minimum = int(self.min_value)
            progress_bar.maximum = int(self.max_value)
            progress_bar.textVisible = True
            progress_bar.text = index.data(role=Qt.DisplayRole)
            progress_bar.progress = int(float(index.data(role=Qt.EditRole)))
        QApplication.style().drawControl(QStyle.CE_ProgressBar, progress_bar,
                                         painter)


class NumberValidator(SimpleValidator):

    def __init__(self, binding, old, parent=None):
        super().__init__(binding=binding, parent=parent)
        self._old_value = old

    def fixup(self, input):
        """Reimplemented function of QValidator

        If the value input has not been validated properly return the previous
        value before editing.
        """
        if self._old_value is None or self._old_value == "":
            # Note: Ideally a table value should always be there. We make sure
            # it stays like this
            return str(get_default_value(self._binding, force=True))

        return str(self._old_value)


class VectorDelegate(QStyledItemDelegate):
    def __init__(self, binding, parent=None):
        super().__init__(parent=parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = LineEditEditor(parent)
        old = index.model().data(index, Qt.DisplayRole)
        old = _create_string_list(old)
        validator = VectorValidator(self._binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        old = _create_string_list(old)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)
            self.commitData.emit(self.sender())

    def setEditorData(self, editor, index):
        """Reimplemented function of QStyledItemDelegate"""
        value = index.model().data(index, Qt.DisplayRole)
        value = _create_string_list(value)
        editor.setText(value)


def _create_string_list(value):
    """Return a string list with stripped white spaces if present"""
    return value if not value else ",".join(
        [v.rstrip().lstrip() for v in value.split(",")])


class VectorValidator(ListValidator):

    def __init__(self, binding, old, parent=None):
        super().__init__(binding=binding, parent=parent)
        self._old_value = old

    def fixup(self, input):
        """Reimplemented function of QValidator"""
        if self._old_value is None:
            # The model will account for an empty string and use a list
            return ""

        return str(self._old_value)


class BindingDelegate(QStyledItemDelegate):
    def __init__(self, binding, parent=None):
        super().__init__(parent)
        self._binding = binding

    def createEditor(self, parent, option, index):
        """Reimplemented function of QStyledItemDelegate"""
        editor = LineEditEditor(parent)
        old = index.model().data(index, Qt.DisplayRole)
        validator = BindingValidator(self._binding, old, parent=editor)
        editor.setValidator(validator)
        return editor

    def setModelData(self, editor, model, index):
        """Reimplemented function of QStyledItemDelegate"""
        old = index.model().data(index, Qt.DisplayRole)
        new = editor.text()
        if not is_equal(old, new):
            model.setData(index, new, Qt.EditRole)
            self.commitData.emit(self.sender())


class BindingValidator(GenericValidator):

    def __init__(self, binding, old, parent=None):
        super().__init__(binding=binding, parent=parent)
        self._old_value = old

    def fixup(self, input):
        """Reimplemented function of QValidator"""
        if self._old_value is None:
            return str(get_default_value(self._binding, force=True))

        return str(self._old_value)


def _color_delegate(name, base):
    """Create a color delegate with a different base class"""

    class ColorDelegate:

        def __init__(self, binding, parent=None):
            super().__init__(binding, parent)
            self.editing = {}
            self.default_brush, self.brushes = create_brushes(
                binding.displayType)

        def createEditor(self, parent, option, index):
            """Reimplemented function of QStyledItemDelegate."""
            key = (index.row(), index.column())
            self.editing[key] = True
            return super().createEditor(parent, option, index)

        def destroyEditor(self, editor, index):
            """Reimplemented function of QStyledItemDelegate."""
            key = (index.row(), index.column())
            self.editing.pop(key, None)
            super().destroyEditor(editor, index)

        def paint(self, painter, option, index):
            """Reimplemented function of QStyledItemDelegate."""
            key = (index.row(), index.column())
            if self.editing.get(key):
                super().paint(painter, option, index)
                return

            frame = QStyleOptionFrame()
            rect = option.rect
            frame.rect = rect
            text = index.data(role=Qt.DisplayRole)
            brush = self.brushes.get(text, self.default_brush)
            if brush is not None:
                painter.fillRect(rect, brush)
            painter.drawText(rect, Qt.AlignCenter, text)
            QApplication.style().drawControl(QStyle.CE_ShapedFrame, frame,
                                             painter)

    return type(name, (ColorDelegate, base), {})


ColorBindingDelegate = _color_delegate("ColorBindingDelegate", BindingDelegate)
ColorNumberDelegate = _color_delegate("ColorNumberDelegate", NumberDelegate)
