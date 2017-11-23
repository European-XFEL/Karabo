#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import pyqtSlot, Qt
from PyQt4.QtGui import (
    QFileDialog, QHBoxLayout, QIcon, QLineEdit, QToolButton, QWidget)
from traits.api import Constant, Instance, Int, on_trait_change

from karabo.common.scenemodel.api import (
    DirectoryModel, FileInModel, FileOutModel)
from karabogui import icons
from karabogui.binding.api import (
    BaseBindingController, StringBinding, register_binding_controller
)
from karabogui.controllers.util import with_display_type
from karabogui.util import getOpenFileName, getSaveFileName


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
        return widget

    @on_trait_change('proxy:value')
    def valueChanged(self, value):
        if self.widget is not None:
            self._path.setText(value)
            self._path.setCursorPosition(self._last_cursor_pos)

    @pyqtSlot(str)
    def _on_user_edit(self, value):
        self._last_cursor_pos = self._path.cursorPosition()
        self.proxy.value = value

    @pyqtSlot()
    def _on_button_click(self):
        path = self.picker()
        if path:
            self._path.setText(path)


# XXX: priority = 20
@register_binding_controller(ui_name='Directory', binding_type=StringBinding,
                             is_compatible=with_display_type('directory'))
class EditableDirectory(_FileSystemPicker):
    # The scene model for this controller
    model = Instance(DirectoryModel)
    # Internal details
    pickerText = Constant('Select directory')

    def _button_icon_default(self):
        return icons.load

    def picker(self):
        return QFileDialog.getExistingDirectory(None, self.pickerText)


# XXX: priority = 20
@register_binding_controller(ui_name='File In', binding_type=StringBinding,
                             is_compatible=with_display_type('fileIn'))
class EditableFileIn(_FileSystemPicker):
    # The scene model for this controller
    model = Instance(FileInModel)
    pickerText = Constant('Select input file')

    def _button_icon_default(self):
        return icons.filein

    def picker(self):
        return getOpenFileName(caption=self.pickerText)


# XXX: priority = 20
@register_binding_controller(ui_name='File Out', binding_type=StringBinding,
                             is_compatible=with_display_type('fileOut'))
class EditableFileOut(_FileSystemPicker):
    # The scene model for this controller
    model = Instance(FileOutModel)
    # Internal details
    pickerText = Constant('Select output file')

    def _button_icon_default(self):
        return icons.fileout

    def picker(self):
        return getSaveFileName(caption=self.pickerText)
