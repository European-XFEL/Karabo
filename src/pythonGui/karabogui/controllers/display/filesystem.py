#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QLineEdit
from traits.api import Instance, on_trait_change

from karabo.common.scenemodel.api import (
    DirectoryModel, FileInModel, FileOutModel)
from karabogui.binding.api import (
    BaseBindingController, StringBinding, register_binding_controller
)
from karabogui.controllers.util import with_display_type


class _FilesystemDisplay(BaseBindingController):
    def create_widget(self, parent):
        widget = QLineEdit(parent)
        widget.setReadOnly(True)
        widget.setFocusPolicy(Qt.NoFocus)
        return widget

    @on_trait_change('proxy:value')
    def _value_update(self, value):
        self.widget.setText(value)


@register_binding_controller(ui_name='Directory', read_only=True,
                             binding_type=StringBinding,
                             is_compatible=with_display_type('directory'))
class DisplayDirectory(_FilesystemDisplay):
    model = Instance(DirectoryModel)


@register_binding_controller(ui_name='File In', read_only=True,
                             binding_type=StringBinding,
                             is_compatible=with_display_type('fileIn'))
class DisplayFileIn(_FilesystemDisplay):
    model = Instance(FileInModel)


@register_binding_controller(ui_name='File Out', read_only=True,
                             binding_type=StringBinding,
                             is_compatible=with_display_type('fileOut'))
class DisplayFileOut(_FilesystemDisplay):
    model = Instance(FileOutModel)
