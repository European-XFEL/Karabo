#############################################################################
# Author: <john.wiggins@xfel.eu>
# Created on September 21, 2017
# Copyright (C) European XFEL GmbH Hamburg. All rights reserved.
#############################################################################
from qtpy.QtCore import Qt
from qtpy.QtWidgets import QLineEdit
from traits.api import Instance

from karabo.common.scenemodel.api import (
    DirectoryModel, FileInModel, FileOutModel)
from karabogui.binding.api import get_binding_value, StringBinding
from karabogui.controllers.api import (
    BaseBindingController, register_binding_controller, with_display_type)


class _FilesystemDisplay(BaseBindingController):
    def create_widget(self, parent):
        widget = QLineEdit(parent)
        widget.setReadOnly(True)
        widget.setFocusPolicy(Qt.NoFocus)
        return widget

    def value_update(self, proxy):
        self.widget.setText(get_binding_value(proxy, ''))


@register_binding_controller(ui_name='Directory', binding_type=StringBinding,
                             klassname='DisplayDirectory',
                             is_compatible=with_display_type('directory'))
class DisplayDirectory(_FilesystemDisplay):
    model = Instance(DirectoryModel, args=())


@register_binding_controller(ui_name='File In', binding_type=StringBinding,
                             klassname='DisplayFileIn',
                             is_compatible=with_display_type('fileIn'))
class DisplayFileIn(_FilesystemDisplay):
    model = Instance(FileInModel, args=())


@register_binding_controller(ui_name='File Out', binding_type=StringBinding,
                             klassname='DisplayFileOut',
                             is_compatible=with_display_type('fileOut'))
class DisplayFileOut(_FilesystemDisplay):
    model = Instance(FileOutModel, args=())
