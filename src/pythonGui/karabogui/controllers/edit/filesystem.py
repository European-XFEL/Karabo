#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import Qt
from qtpy.QtGui import QIcon
from qtpy.QtWidgets import (
    QFileDialog, QHBoxLayout, QLineEdit, QToolButton, QWidget)
from traits.api import Constant, Instance, Int

from karabo.common.scenemodel.api import (
    DirectoryModel, FileInModel, FileOutModel)
from karabogui import icons
from karabogui.binding.api import StringBinding, get_editor_value
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)
from karabogui.util import SignalBlocker, getOpenFileName, getSaveFileName


class _FileSystemPicker(BaseBindingController):
    button_icon = Instance(QIcon)

    _last_cursor_pos = Int(0)
    _path = Instance(QLineEdit)

    def create_widget(self, parent):
        self._path = QLineEdit(parent)
        self._path.textChanged.connect(self._on_user_edit)
        self._path.setFocusPolicy(Qt.StrongFocus)

        button = QToolButton(parent)
        button.setStatusTip(self.pickerText)
        button.setToolTip(self.pickerText)
        button.setIcon(self.button_icon)
        button.setMaximumSize(25, 25)
        button.setFocusPolicy(Qt.NoFocus)
        button.clicked.connect(self._on_button_click)

        widget = QWidget(parent)
        hLayout = QHBoxLayout(widget)
        hLayout.setContentsMargins(0, 0, 0, 0)
        hLayout.addWidget(self._path)
        hLayout.addWidget(button)

        widget.setFocusProxy(self._path)

        return widget

    def value_update(self, proxy):
        with SignalBlocker(self._path):
            self._path.setText(get_editor_value(proxy, ''))
            self._path.setCursorPosition(self._last_cursor_pos)

    def _on_user_edit(self, value):
        if self.proxy.binding is None:
            return
        self._last_cursor_pos = self._path.cursorPosition()
        self.proxy.edit_value = value

    def _on_button_click(self):
        path = self.picker()
        if path:
            self._path.setText(path)


@register_binding_controller(ui_name='Directory', can_edit=True,
                             klassname='EditableDirectory',
                             binding_type=StringBinding, priority=20,
                             is_compatible=with_display_type('directory'))
class EditableDirectory(_FileSystemPicker):
    # The scene model for this controller
    model = Instance(DirectoryModel, args=())
    # Internal details
    pickerText = Constant('Select directory')

    def _button_icon_default(self):
        return icons.load

    def picker(self):
        return QFileDialog.getExistingDirectory(self.widget, self.pickerText)


@register_binding_controller(ui_name='File In', can_edit=True,
                             klassname='EditableFileIn',
                             binding_type=StringBinding, priority=20,
                             is_compatible=with_display_type('fileIn'))
class EditableFileIn(_FileSystemPicker):
    # The scene model for this controller
    model = Instance(FileInModel, args=())
    # Internal details
    pickerText = Constant('Select input file')

    def _button_icon_default(self):
        return icons.filein

    def picker(self):
        return getOpenFileName(caption=self.pickerText, parent=self.widget)


@register_binding_controller(ui_name='File Out', can_edit=True,
                             klassname='EditableFileOut',
                             binding_type=StringBinding, priority=20,
                             is_compatible=with_display_type('fileOut'))
class EditableFileOut(_FileSystemPicker):
    # The scene model for this controller
    model = Instance(FileOutModel, args=())
    # Internal details
    pickerText = Constant('Select output file')

    def _button_icon_default(self):
        return icons.fileout

    def picker(self):
        return getSaveFileName(caption=self.pickerText, parent=self.widget)
